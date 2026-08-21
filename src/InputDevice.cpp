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
	PARENTLOCK( m_triggers )
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
	m_deviceIdentifier = identifier;
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
		for( auto& trigger: m_triggers )
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
	for( const auto& trigger : m_sortedTriggers )
	{
		trigger->Process( m_currentState );
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