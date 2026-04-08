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

Events::State Merge( const Events::State& current, const Events::State& update )
{
	Events::State merged{};
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

InputDevice::InputDevice( IRoot* lockobj ) :
	PARENTLOCK( m_triggers )
{
}

void InputDevice::SetIdentifier( DeviceEnums::DeviceIdentifier identifier )
{
	m_deviceIdentifier = identifier;
}

uint32_t InputDevice::GetDeviceID() const
{
	return m_deviceIdentifier.deviceID;
}

BlueSharedStringW InputDevice::GetName() const
{
	return m_deviceIdentifier.name;
}

void InputDevice::ProcessTriggers( Events::State state )
{
	// merge the state with the current state
	m_currentState = Merge( m_currentState, state );

	for( const auto& trigger : m_triggers )
	{
		trigger->Process( m_currentState );
	}
}

BlueSharedString InputDevice::GetStateAsJson() const
{
	std::stringstream ss;
	ss << "{ \"buttons\": [";
	for( size_t i = 0; i < m_currentState.buttons.size(); ++i )
	{
		const auto& btn = m_currentState.buttons[i];
		ss << "{ \"id\": " << btn.buttonId << ", \"state\": " << static_cast<int>( btn.state ) << "}";
		if( i < m_currentState.buttons.size() - 1 )
		{
			ss << ",";
		}
	}
	ss << "], \"axis\": [";
	for( size_t i = 0; i < m_currentState.axis.size(); ++i )
	{
		ss << m_currentState.axis[i];
		if( i < m_currentState.axis.size() - 1 )
		{
			ss << ",";
		}
	}
	ss << "], \"switches\": [";
	for( size_t i = 0; i < m_currentState.switches.size(); ++i )
	{
		ss << static_cast<int>( m_currentState.switches[i] );
		if( i < m_currentState.switches.size() - 1 )
		{
			ss << ",";
		}
	}
	ss << "] }";
	return BlueSharedString( ss.str().c_str() );
}