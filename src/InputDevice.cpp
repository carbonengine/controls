// Copyright © 2026 CCP ehf.

#include "InputDevice.h"

namespace
{

BlueStructureDefinition RawDeviceIdDef[] = {
	{ "id", Be::UINT32_1, 0 },
	{ 0 }
};
}

InputDevice::InputDevice( IRoot* lockobj ) :
	PARENTLOCK( m_rawDeviceId ),
	PARENTLOCK( m_triggers ),
	PARENTLOCK( m_buttons ),
	PARENTLOCK( m_axes ),
	PARENTLOCK( m_switches )
{
	m_rawDeviceId.SetStructureDefinition( RawDeviceIdDef );
	m_rawDeviceId.SetDefaultValue( 0 );
	m_triggers.SetNotify( this );
}

void InputDevice::OnListModified(
	long event,
	ssize_t key,
	ssize_t key2,
	IRoot* value,
	const struct IList* theList )
{
	if( theList == &m_triggers )
	{
		m_triggersDirty = true;
	}
}

void InputDevice::SetIdentifier( DeviceEnums::DeviceIdentifier identifier )
{
	m_deviceIdentifier = std::move( identifier );

	auto appendElements = []( const std::vector<DeviceEnums::ElementKey>& keys, PInputElementVector& target ) {
		for( const auto& key : keys )
		{
			InputElementPtr element;
			element.CreateInstance();
			element->Initialize( key );
			target.Append( element );
		}
	};

	appendElements( m_deviceIdentifier.buttonElements, m_buttons );
	appendElements( m_deviceIdentifier.axisElements, m_axes );
	appendElements( m_deviceIdentifier.switchElements, m_switches );

	// Seeded neutral so events evaluated before the first hardware reading arrives still find
	// an entry for their element instead of reporting a missing state.
	m_currentState.timestamp = Events::GetTimestamp();
	for( const auto& key : m_deviceIdentifier.buttonElements )
	{
		m_currentState.buttons.emplace( key, Events::Button{} );
	}
	for( const auto& key : m_deviceIdentifier.axisElements )
	{
		m_currentState.axis.emplace( key, Events::Axis{} );
	}
	for( const auto& key : m_deviceIdentifier.switchElements )
	{
		m_currentState.switches.emplace( key, Events::Switch{} );
	}
}

BlueSharedString InputDevice::GetDeviceID() const
{
	return m_deviceIdentifier.deviceID;
}

BlueSharedString InputDevice::GetName() const
{
	return m_deviceIdentifier.name;
}

void InputDevice::Update( IInputHandler* inputHandler )
{
	if( m_triggersDirty )
	{
		m_sortedTriggers.clear();
		for( auto& trigger : m_triggers )
		{
			m_sortedTriggers.push_back( trigger );
		}
		std::sort( m_sortedTriggers.begin(), m_sortedTriggers.end(), []( const InputEventTrigger* a, const InputEventTrigger* b ) {
			return a->GetEventCount() > b->GetEventCount();
		} );
		m_triggersDirty = false;
	}

	auto states = inputHandler->Update( m_deviceIdentifier.deviceID );
	if( !states.empty() )
	{
		for( const auto& state : states )
		{
			UpdateState( state );
		}
	}
	else
	{
		m_currentState.timestamp = Events::GetTimestamp();
		UpdateState( m_currentState );
	}

	if( m_updateRumble )
	{
		inputHandler->Rumble( m_deviceIdentifier.deviceID, m_rumble );
		m_updateRumble = false;
	}
}

void InputDevice::UpdateState( const Events::State& state )
{
	m_currentState = state;

	// ownership is only valid for a single evaluation pass. m_currentState is re-processed when the
	// handler reports no new state, so stale flags would permanently block events on those elements.
	for( auto& button : m_currentState.buttons )
	{
		button.second.matched = false;
		button.second.comboOwned = false;
	}
	for( auto& axis : m_currentState.axis )
	{
		axis.second.matched = false;
	}
	for( auto& switchState : m_currentState.switches )
	{
		switchState.second.matched = false;
	}

	// a button that was part of a combination is spent: its release must not also be reported as a
	// Pressed/Released of that button on its own. While it is still pressed the combination keeps
	// owning it, so it only needs blocking in the state where it is released.
	for( auto it = m_spentButtons.begin(); it != m_spentButtons.end(); )
	{
		auto button = m_currentState.buttons.find( *it );
		if( button == m_currentState.buttons.end() )
		{
			it = m_spentButtons.erase( it );
		}
		else if( !button->second.pressed )
		{
			button->second.matched = true;
			it = m_spentButtons.erase( it );
		}
		else
		{
			++it;
		}
	}

	for( const auto& trigger : m_sortedTriggers )
	{
		trigger->Process( m_currentState );
	}

	for( const auto& button : m_currentState.buttons )
	{
		if( button.second.comboOwned && button.second.pressed )
		{
			m_spentButtons.insert( button.first );
		}
	}
}

float InputDevice::GetHighFrequencyRumble() const
{
	return m_rumble.highFrequency;
}

void InputDevice::SetHighFrequencyRumble( float value )
{
	m_rumble.highFrequency = value;
	m_updateRumble = true;
}

float InputDevice::GetLowFrequencyRumble() const
{
	return m_rumble.lowFrequency;
}

void InputDevice::SetLowFrequencyRumble( float value )
{
	m_rumble.lowFrequency = value;
	m_updateRumble = true;
}

float InputDevice::GetLeftTriggerRumble() const
{
	return m_rumble.leftTrigger;
}

void InputDevice::SetLeftTriggerRumble( float value )
{
	m_rumble.leftTrigger = value;
	m_updateRumble = true;
}

float InputDevice::GetRightTriggerRumble() const
{
	return m_rumble.rightTrigger;
}

void InputDevice::SetRightTriggerRumble( float value )
{
	m_rumble.rightTrigger = value;
	m_updateRumble = true;
}

void InputDevice::ResetRumble()
{
	m_rumble = Events::Rumble{};
	m_updateRumble = false;
}

DeviceEnums::DeviceFamily InputDevice::GetDeviceFamily() const
{
	return m_deviceIdentifier.family;
}