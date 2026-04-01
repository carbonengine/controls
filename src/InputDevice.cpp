#include "InputDevice.h"
#include <sstream>

namespace
{
Events::Button Merge( const Events::Button currentState, const Events::Button newState )
{
	auto now = std::chrono::steady_clock::now();
	// A button is considered pressed if it is pressed and released within g_holdTimeInMs, otherwise it is considered held. If it was held and then release then it is considered released.
	Events::Button mergedBtn = currentState;
	if( !newState._pressed ){
		if( currentState.state == Events::ButtonState::Held )
		{
			// it was held and then released, so we consider it released
			mergedBtn.state = Events::ButtonState::Released;
		}
		else if( currentState._pressed )
		{
			// it was just released, so we consider it pressed
			mergedBtn.state = Events::ButtonState::Pressed;
		}
		else
		{
			mergedBtn.state = Events::ButtonState::Up;
		}
		mergedBtn._pressed = false;
		mergedBtn.m_stateChangeTime = now;
	}
	else
	{
		// either it was just pressed, or it is being held
		mergedBtn._pressed = true;
		if( !currentState._pressed ){
			mergedBtn.state = Events::ButtonState::Down;
			mergedBtn.m_stateChangeTime = now;
		}
		else if( std::chrono::duration_cast<std::chrono::milliseconds>( now - currentState.m_stateChangeTime ).count() >= InputDevice::g_holdTimeInMs  )
		{
			mergedBtn.state = Events::ButtonState::Held;
		}
	}
	return mergedBtn;
}

Events::GamePadState Merge(const Events::GamePadState& current, const Events::GamePadState& update)
{
	Events::GamePadState merged{};
	merged.a = Merge( current.a, update.a );
	merged.b = Merge( current.b, update.b );
	merged.x = Merge( current.x, update.x );
	merged.y = Merge( current.y, update.y );
	merged.leftShoulder = Merge( current.leftShoulder, update.leftShoulder );
	merged.rightShoulder = Merge( current.rightShoulder, update.rightShoulder );
	merged.dpadUp = Merge( current.dpadUp, update.dpadUp );
	merged.dpadDown = Merge( current.dpadDown, update.dpadDown );
	merged.dpadLeft = Merge( current.dpadLeft, update.dpadLeft );
	merged.dpadRight = Merge( current.dpadRight, update.dpadRight );
	merged.menu = Merge( current.menu, update.menu );
	merged.view = Merge( current.view, update.view );
	merged.leftTrigger.amountPressed = update.leftTrigger.amountPressed;
	merged.rightTrigger.amountPressed = update.rightTrigger.amountPressed;
	merged.leftThumbstick.x = update.leftThumbstick.x;
	merged.leftThumbstick.y = update.leftThumbstick.y;
	merged.leftThumbstick.button = Merge( current.leftThumbstick.button, update.leftThumbstick.button );
	merged.rightThumbstick.x = update.rightThumbstick.x;
	merged.rightThumbstick.y = update.rightThumbstick.y;
	merged.rightThumbstick.button = Merge( current.rightThumbstick.button, update.rightThumbstick.button );
	return merged;
}

Events::FlightStickState Merge( const Events::FlightStickState& current, const Events::FlightStickState& update )
{
	Events::FlightStickState merged{};
	merged.yaw = update.yaw;
	merged.pitch = update.pitch;
	merged.roll = update.roll;
	merged.firePrimary = Merge( current.firePrimary, update.firePrimary );
	merged.fireSecondary = Merge( current.fireSecondary, update.fireSecondary );

	return merged;
}
Events::ControllerState Merge( const Events::ControllerState& current, const Events::ControllerState& update )
{
	Events::ControllerState merged{};
	if( current.buttons.size() == update.buttons.size() )
	{
		merged.buttons.reserve( current.buttons.size() );
		for( size_t i = 0; i < current.buttons.size(); ++i )
		{
			merged.buttons.push_back( Merge( current.buttons[i], update.buttons[i] ) );
		}
	}
	else 
	{
		CCP_LOGWARN( "Controller button count changed from %zu to %zu, resetting state", current.buttons.size(), update.buttons.size() );
		merged = update;
		return merged;
	}

	if( current.axis.size() == update.axis.size() )
	{
		merged.axis = update.axis;
	}
	else
	{
		CCP_LOGWARN( "Controller axis count changed from %zu to %zu, resetting state", current.axis.size(), update.axis.size() );
		merged = update;
		return merged;
	}

	if( current.switches.size() == update.switches.size() )
	{
		merged.switches = update.switches;
	}
	else
	{
		CCP_LOGWARN( "Controller switch count changed from %zu to %zu, resetting state", current.switches.size(), update.switches.size() );
		merged = update;
		return merged;
	}

	return merged;
}
}

float InputDevice::g_holdTimeInMs = 300.0f; // The time in milliseconds after which a button state changes from Pressed to Held

InputDeviceIdentifier::InputDeviceIdentifier( IRoot* lockobj )
{
}

void InputDeviceIdentifier::SetData( const DeviceEnums::DeviceIdentifier& identifier )
{
	this->identifier = identifier;
}

uint32_t InputDeviceIdentifier::GetDeviceID() const
{
	return identifier.deviceID;
}

InputDevice::InputDevice( IRoot* lockobj ) :
	PARENTLOCK( m_triggers )
{
}

void InputDevice::SetIdentifier( InputDeviceIdentifierPtr identifier )
{
	m_deviceIdentifier = identifier;
}

void InputDevice::ProcessTriggers( Events::State state )
{
	// merge the state with the current state
	m_currentState.batteryState = state.batteryState;
	m_currentState.gamePadState = Merge( m_currentState.gamePadState, state.gamePadState );
	m_currentState.flightStickState = Merge( m_currentState.flightStickState, state.flightStickState );
	m_currentState.controllerState = Merge( m_currentState.controllerState, state.controllerState );

	for( const auto& trigger : m_triggers )
	{
		trigger->Process( m_currentState );
	}
}

namespace
{

const char* ButtonStateToString( Events::ButtonState state )
{
	switch( state )
	{
	case Events::ButtonState::Pressed:
		return "Pressed";
	case Events::ButtonState::Held:
		return "Held";
	case Events::ButtonState::Released:
		return "Released";
	default:
		return "Unknown";
	}
}

void WriteButton( std::ostringstream& os, const char* name, const Events::Button& btn, bool trailing )
{
	os << "\"" << name << "\": {"
	   << "\"state\": \"" << ButtonStateToString( btn.state ) << "\", "
	   << "}";
	if( trailing )
	{
		os << ", ";
	}
}

void WriteTrigger( std::ostringstream& os, const char* name, const Events::Trigger& trg, bool trailing )
{
	os << "\"" << name << "\": {"
	   << "\"amountPressed\": " << trg.amountPressed
	   << "}";
	if( trailing )
	{
		os << ", ";
	}
}

void WriteThumbStick( std::ostringstream& os, const char* name, const Events::ThumbStick& ts, bool trailing )
{
	os << "\"" << name << "\": {"
	   << "\"x\": " << ts.x << ", "
	   << "\"y\": " << ts.y << ", ";
	WriteButton( os, "button", ts.button, false );
	os << "}";
	if( trailing )
	{
		os << ", ";
	}
}

} // anonymous namespace

BlueSharedString InputDevice::GetStateAsJson() const
{
	auto state = m_currentState;
	std::ostringstream os;

	os << "{";

	// gamePadState
	{
		const auto& gp = state.gamePadState;
		os << "\"gamePadState\": {";
		WriteButton( os, "view", gp.view, true );
		WriteButton( os, "menu", gp.menu, true );
		WriteButton( os, "a", gp.a, true );
		WriteButton( os, "b", gp.b, true );
		WriteButton( os, "x", gp.x, true );
		WriteButton( os, "y", gp.y, true );
		WriteButton( os, "leftShoulder", gp.leftShoulder, true );
		WriteButton( os, "rightShoulder", gp.rightShoulder, true );
		WriteButton( os, "dpadUp", gp.dpadUp, true );
		WriteButton( os, "dpadDown", gp.dpadDown, true );
		WriteButton( os, "dpadLeft", gp.dpadLeft, true );
		WriteButton( os, "dpadRight", gp.dpadRight, true );
		WriteTrigger( os, "leftTrigger", gp.leftTrigger, true );
		WriteTrigger( os, "rightTrigger", gp.rightTrigger, true );
		WriteThumbStick( os, "leftThumbstick", gp.leftThumbstick, true );
		WriteThumbStick( os, "rightThumbstick", gp.rightThumbstick, false );
		os << "}, ";
	}

	// flightStickState
	{
		const auto& fs = state.flightStickState;
		os << "\"flightStickState\": {"
		   << "\"yaw\": " << fs.yaw << ", "
		   << "\"pitch\": " << fs.pitch << ", "
		   << "\"roll\": " << fs.roll << ", ";
		WriteButton( os, "firePrimary", fs.firePrimary, true );
		WriteButton( os, "fireSecondary", fs.fireSecondary, false );
		os << "}, ";
	}

	// batteryState
	{
		const auto& bs = state.batteryState;
		os << "\"batteryState\": {"
		   << "\"remainingCapacity\": " << bs.remainingCapacity << ", "
		   << "\"fullChargeCapacity\": " << bs.fullChargeCapacity << ", "
		   << "\"charging\": " << ( bs.charging ? "true" : "false" )
		   << "}";
	}

	os << "}";

	return BlueSharedString( os.str() );
}