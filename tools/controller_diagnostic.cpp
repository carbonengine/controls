#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#ifndef _WIN32
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#endif

#ifdef __APPLE__
#import <Foundation/Foundation.h>
#include "../src/handlers/InputHandlerApple.h"
#elif defined(WIN32)
#include "../src/handlers/InputHandlerWin.h"
#else
#include "../src/handlers/InputHandlerStub.h"
#endif

#include "../src/DeviceEnums.h"
#include "../src/events/Events.h"
#include "../src/handlers/IInputHandler.h"

// Required by the shared logging macros pulled in by the handler sources.
const char* g_moduleName = "controller_diagnostic";

namespace
{
std::atomic<bool> g_shouldExit{ false };

void HandleSignal( int )
{
	g_shouldExit.store( true );
}

std::mutex g_deviceListMutex;
std::vector<DeviceEnums::DeviceIdentifier> g_connectedDevices;

void OnDeviceAdded( DeviceEnums::DeviceIdentifier& identifier )
{
	std::lock_guard<std::mutex> lock( g_deviceListMutex );
	g_connectedDevices.push_back( identifier );
	std::printf( "[+] Device connected: %s (id=%s, buttons=%u, axes=%u, switches=%u)\n",
		identifier.name.c_str(),
		identifier.deviceID.c_str(),
		identifier.buttonCount,
		identifier.axisCount,
		identifier.switchCount );
	std::fflush( stdout );
}

void OnDeviceRemoved( DeviceEnums::DeviceIdentifier& identifier )
{
	std::lock_guard<std::mutex> lock( g_deviceListMutex );
	const std::string toRemove( identifier.deviceID.c_str() );
	g_connectedDevices.erase(
		std::remove_if(
			g_connectedDevices.begin(),
			g_connectedDevices.end(),
			[&toRemove]( const DeviceEnums::DeviceIdentifier& d ) {
				return std::string( d.deviceID.c_str() ) == toRemove;
			} ),
		g_connectedDevices.end() );
	std::printf( "[-] Device disconnected: %s\n", identifier.name.c_str() );
	std::fflush( stdout );
}

// Tick the platform run loop so device callbacks and input events are delivered.
void PumpEvents()
{
#ifdef __APPLE__
	@autoreleasepool
	{
		NSDate* limit = [NSDate dateWithTimeIntervalSinceNow:0.005];
		[[NSRunLoop currentRunLoop] runUntilDate:limit];
	}
#else
	std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
#endif
}

std::unique_ptr<IInputHandler> MakeInputHandler()
{
#ifdef __APPLE__
	return std::unique_ptr<IInputHandler>( new InputHandlerApple() );
#elif defined(WIN32)
	return std::unique_ptr<IInputHandler>( new InputHandlerWin() );
#else
	return std::unique_ptr<IInputHandler>( new InputHandlerStub() );
#endif
}

std::string RenderAxisBar( float value )
{
	const int width = 20;
	const int half = width / 2;
	int filled = static_cast<int>( value * half );
	if( filled > half ) filled = half;
	if( filled < -half ) filled = -half;

	std::string bar;
	bar.reserve( width + 2 );
	bar.push_back( '[' );
	for( int i = -half; i < half; ++i )
	{
		if( i == 0 )
		{
			bar.push_back( '|' );
		}
		else if( ( filled >= 0 && i >= 0 && i < filled ) || ( filled < 0 && i < 0 && i >= filled ) )
		{
			bar.push_back( '=' );
		}
		else
		{
			bar.push_back( ' ' );
		}
	}
	bar.push_back( ']' );
	return bar;
}

const char* SwitchPositionName( Events::SwitchPosition p )
{
	switch( p )
	{
	case Events::SwitchPosition::Center: return "Center";
	case Events::SwitchPosition::Up: return "Up";
	case Events::SwitchPosition::UpRight: return "UpRight";
	case Events::SwitchPosition::Right: return "Right";
	case Events::SwitchPosition::DownRight: return "DownRight";
	case Events::SwitchPosition::Down: return "Down";
	case Events::SwitchPosition::DownLeft: return "DownLeft";
	case Events::SwitchPosition::Left: return "Left";
	case Events::SwitchPosition::UpLeft: return "UpLeft";
	case Events::SwitchPosition::Any: return "Any";
	}
	return "?";
}

const char* ButtonStateName( Events::ButtonState s )
{
	switch( s )
	{
	case Events::ButtonState::Down: return "x";
	case Events::ButtonState::Held: return "X";
	default: return "";
	}
}

// Per-button timing needed to derive the logical ButtonState from raw pressed samples.
struct ButtonLogicalTracker
{
	bool prevPressed = false;
	uint64_t pressStartMicros = 0;
	// Keep transient Pressed/Released visible for a short window since we sample at ~60Hz.
	uint64_t transientHoldUntilMicros = 0;
	Events::ButtonState displayed = Events::ButtonState::Up;
};

uint64_t NowMicros()
{
	return static_cast<uint64_t>(
		std::chrono::duration_cast<std::chrono::microseconds>(
			std::chrono::steady_clock::now().time_since_epoch() )
			.count() );
}

void UpdateButtonLogicalState( ButtonLogicalTracker& t, bool pressed, uint64_t nowUs )
{
	// Linger transient states (Pressed / Released) so they don't blink past between frames.
	const uint64_t transientLingerUs = 200 * 1000;

	if( pressed && !t.prevPressed )
	{
		t.pressStartMicros = nowUs;
		t.displayed = Events::ButtonState::Down;
		t.transientHoldUntilMicros = 0;
	}
	else if( pressed && t.prevPressed )
	{
		const uint64_t elapsed = nowUs - t.pressStartMicros;
		t.displayed = ( elapsed >= Events::g_holdTimeInMicroSeconds ) ? Events::ButtonState::Held : Events::ButtonState::Down;
		t.transientHoldUntilMicros = 0;
	}
	else if( !pressed && t.prevPressed )
	{
		const uint64_t elapsed = nowUs - t.pressStartMicros;
		t.displayed = ( elapsed >= Events::g_holdTimeInMicroSeconds ) ? Events::ButtonState::Released : Events::ButtonState::Pressed;
		t.transientHoldUntilMicros = nowUs + transientLingerUs;
	}
	else if( nowUs >= t.transientHoldUntilMicros )
	{
		t.displayed = Events::ButtonState::Up;
	}
	t.prevPressed = pressed;
}

// Puts stdin in non-canonical, non-echoing, non-blocking mode so hotkeys are pollable each frame.
#ifndef _WIN32
struct RawStdinGuard
{
	termios saved{};
	int savedFlags = 0;
	bool active = false;

	void Enter()
	{
		if( !isatty( STDIN_FILENO ) )
		{
			return;
		}
		if( tcgetattr( STDIN_FILENO, &saved ) != 0 )
		{
			return;
		}
		termios raw = saved;
		raw.c_lflag &= ~( ICANON | ECHO );
		raw.c_cc[VMIN] = 0;
		raw.c_cc[VTIME] = 0;
		if( tcsetattr( STDIN_FILENO, TCSANOW, &raw ) != 0 )
		{
			return;
		}
		savedFlags = fcntl( STDIN_FILENO, F_GETFL, 0 );
		fcntl( STDIN_FILENO, F_SETFL, savedFlags | O_NONBLOCK );
		active = true;
	}

	~RawStdinGuard()
	{
		if( active )
		{
			tcsetattr( STDIN_FILENO, TCSANOW, &saved );
			fcntl( STDIN_FILENO, F_SETFL, savedFlags );
		}
	}

	int TryRead()
	{
		if( !active )
		{
			return -1;
		}
		char c = 0;
		ssize_t n = read( STDIN_FILENO, &c, 1 );
		return n == 1 ? static_cast<int>( static_cast<unsigned char>( c ) ) : -1;
	}
};
#else
struct RawStdinGuard
{
	void Enter() {}
	int TryRead() { return -1; }
};
#endif

struct RumblePulse
{
	float value = 0.0f;
	uint64_t expireUs = 0;
};

const char* NameAt( const std::vector<BlueSharedString>& names, size_t index )
{
	if( index < names.size() )
	{
		return names[index].c_str();
	}
	return "(unnamed)";
}

void RenderState(
	const DeviceEnums::DeviceIdentifier& id,
	const Events::State& state,
	const std::vector<ButtonLogicalTracker>& buttonTrackers,
	const std::vector<BlueSharedString>& buttonNames,
	const std::vector<BlueSharedString>& axisNames,
	const std::vector<BlueSharedString>& switchNames,
	const RumblePulse ( &pulses )[4] )
{
	// Move cursor to top-left and clear from cursor down.
	std::printf( "\x1b[H\x1b[J" );
	std::printf( "Controller Diagnostic — %s (id=%s)\n", id.name.c_str(), id.deviceID.c_str() );
	std::printf( "buttons=%zu axes=%zu switches=%zu   (Ctrl+C to quit)\n",
		state.buttons.size(), state.axis.size(), state.switches.size() );
	std::printf( "Rumble motors=%u: [1]low=%s%.2f [2]high=%s%.2f [3]lTrig=%s%.2f [4]rTrig=%s%.2f\n\n",
		id.rumbleCapacity.rumbleMotorCount,
		id.rumbleCapacity.hasLowFrequencyRumble ? "" : "(unsupported)",
		pulses[0].value,
		id.rumbleCapacity.hasHighFrequencyRumble ? "" : "(unsupported)",
		pulses[1].value,
		id.rumbleCapacity.hasLeftTriggerRumble ? "" : "(unsupported)",
		pulses[2].value,
		id.rumbleCapacity.hasRightTriggerRumble ? "" : "(unsupported)",
		pulses[3].value );

	std::printf( "Buttons:\n" );
	for( size_t i = 0; i < state.buttons.size(); ++i )
	{
		const Events::ButtonState logical = ( i < buttonTrackers.size() ) ? buttonTrackers[i].displayed : Events::ButtonState::Up;
		std::printf( "  %2zu %-24s: %-8s\n", i, NameAt( buttonNames, i ), ButtonStateName( logical ) );
	}

	std::printf( "\nAxes:\n" );
	for( size_t i = 0; i < state.axis.size(); ++i )
	{
		std::printf( "  %2zu %-24s: %s %+.3f\n", i, NameAt( axisNames, i ), RenderAxisBar( state.axis[i].value ).c_str(), state.axis[i].value );
	}

	std::printf( "\nSwitches:\n" );
	for( size_t i = 0; i < state.switches.size(); ++i )
	{
		std::printf( "  %2zu %-24s: %s\n", i, NameAt( switchNames, i ), SwitchPositionName( state.switches[i].position ) );
	}

	std::fflush( stdout );
}
}

int main( int /*argc*/, char** /*argv*/ )
{
	std::signal( SIGINT, HandleSignal );
	std::signal( SIGTERM, HandleSignal );

	auto handler = MakeInputHandler();
	
	handler->SetBackgroundEventsEnabled( true );
	handler->RegisterForDeviceAdded( OnDeviceAdded );
	handler->RegisterForDeviceRemoved( OnDeviceRemoved );

	if( !handler->Initialize() )
	{
		std::fprintf( stderr, "Failed to initialize input handler.\n" );
		return 1;
	}

	std::printf( "Waiting for a controller to connect... (press Ctrl+C to quit)\n" );
	std::fflush( stdout );

	DeviceEnums::DeviceIdentifier chosen;
	bool haveChoice = false;
	while( !g_shouldExit.load() && !haveChoice )
	{
		PumpEvents();
		{
			std::lock_guard<std::mutex> lock( g_deviceListMutex );
			if( !g_connectedDevices.empty() )
			{
				chosen = g_connectedDevices.front();
				haveChoice = true;
			}
		}
	}

	if( g_shouldExit.load() )
	{
		std::printf( "\nExiting before device selection.\n" );
		return 0;
	}

	std::printf( "\nSelected device: %s\n", chosen.name.c_str() );
	std::fflush( stdout );

	handler->SetDeviceActivation( chosen.deviceID, true );

	const std::vector<BlueSharedString> buttonNames = chosen.buttonNames;
	const std::vector<BlueSharedString> axisNames = chosen.axisNames;
	const std::vector<BlueSharedString> switchNames = chosen.switchNames;

	Events::State latest;
	// Initialise sizes so an empty poll still renders a stable table.
	latest.buttons.resize( chosen.buttonCount );
	latest.axis.resize( chosen.axisCount );
	latest.switches.resize( chosen.switchCount );

	std::vector<ButtonLogicalTracker> buttonTrackers( chosen.buttonCount );

	RawStdinGuard rawStdin;
	rawStdin.Enter();

	RumblePulse pulses[4] = {};
	const uint64_t pulseDurationUs = 500 * 1000;
	const bool channelSupported[4] = {
		chosen.rumbleCapacity.hasLowFrequencyRumble,
		chosen.rumbleCapacity.hasHighFrequencyRumble,
		chosen.rumbleCapacity.hasLeftTriggerRumble,
		chosen.rumbleCapacity.hasRightTriggerRumble,
	};

	while( !g_shouldExit.load() )
	{
		PumpEvents();

		auto states = handler->Update( chosen.deviceID );
		if( !states.empty() )
		{
			latest = states.back();
		}

		const uint64_t nowUs = NowMicros();

		int key = rawStdin.TryRead();
		while( key >= 0 )
		{
			if( key >= '1' && key <= '4' )
			{
				const int channel = key - '1';
				if( channelSupported[channel] )
				{
					pulses[channel].value = 1.0f;
					pulses[channel].expireUs = nowUs + pulseDurationUs;
				}
			}
			key = rawStdin.TryRead();
		}

		for( int i = 0; i < 4; ++i )
		{
			if( pulses[i].value > 0.0f && nowUs >= pulses[i].expireUs )
			{
				pulses[i].value = 0.0f;
			}
		}

		Events::Rumble rumble = {};
		rumble.lowFrequency = pulses[0].value;
		rumble.highFrequency = pulses[1].value;
		rumble.leftTrigger = pulses[2].value;
		rumble.rightTrigger = pulses[3].value;
		handler->Rumble( chosen.deviceID, rumble );

		if( buttonTrackers.size() < latest.buttons.size() )
		{
			buttonTrackers.resize( latest.buttons.size() );
		}
		for( size_t i = 0; i < latest.buttons.size(); ++i )
		{
			UpdateButtonLogicalState( buttonTrackers[i], latest.buttons[i].pressed, nowUs );
		}

		RenderState( chosen, latest, buttonTrackers, buttonNames, axisNames, switchNames, pulses );
		std::this_thread::sleep_for( std::chrono::milliseconds( 16 ) );
	}

	// Silence motors before we tear down the activation so ShutdownHapticsForSlot has nothing to trail off.
	Events::Rumble zero = {};
	handler->Rumble( chosen.deviceID, zero );
	handler->SetDeviceActivation( chosen.deviceID, false );
	std::printf( "\nShutting down.\n" );
	return 0;
}
