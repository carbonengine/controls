#include "InputDevice.h"
#include <sstream>

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
	for( const auto& trigger : m_triggers )
	{
		trigger->Process( state );
	}
	m_currentState = state;
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
	   << "\"buttonId\": " << btn.buttonId << ", "
	   << "\"pressed\": " << ( btn._pressed ? "true" : "false" ) << ", "
	   << "\"state\": \"" << ButtonStateToString( btn.state ) << "\", "
	   << "\"stateChangeTime\": " << btn.m_stateChangeTime
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