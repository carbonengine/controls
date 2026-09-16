#include "ControllerSwitchInputEvent.h"

ControllerSwitchInputEvent::ControllerSwitchInputEvent( IRoot* lockobj )
{
}

bool ControllerSwitchInputEvent::Match( const Events::State& state )
{
	m_previousState = m_state;
	m_matched = Evaluate( state );
	return m_matched;
}

bool ControllerSwitchInputEvent::JustMatched()
{
	return m_matched && m_previousState != m_state;
}

bool ControllerSwitchInputEvent::Evaluate( const Events::State& state )
{
	
	if( m_attached && state.switches.size() > 0 )
	{
		auto it = state.switches.find( static_cast<uint32_t>( m_element ) + m_index );
		
		if( it == state.switches.end() )
		{
			CCP_LOGERR( "ControllerSwitchInputEvent::Match: Could not find switch state for element %s index %d", DeviceEnums::ToKeyString( m_element ), m_index );
			return false;
		}

		auto& switchState = it->second;

		if( !switchState.matched && (switchState.position == m_event || m_event == Events::SwitchPosition::Any) )
		{
			m_state = switchState.position;
			return true;
		}
		m_state = Events::SwitchPosition::Center;
	}

	return false;
}

void ControllerSwitchInputEvent::Own( Events::State& state )
{
	if( !m_attached )
	{
		CCP_LOGERR( "ControllerSwitchInputEvent::Own: Cannot own switch state because no input element is attached." );
		return;
	}

	auto it = state.switches.find( static_cast<uint32_t>( m_element ) + m_index );

	if( it == state.switches.end() )
	{
		CCP_LOGERR( "ControllerSwitchInputEvent::Own: Could not find switch state for element %s index %d", DeviceEnums::ToKeyString( m_element ), m_index );
		return;
	}

	auto& switchState = it->second;
	switchState.matched = true;
}

void ControllerSwitchInputEvent::AttachTo( const InputElement* input )
{
	m_attached = false;
	m_element = DeviceEnums::InputElementDescriptor::Unknown;
	m_index = 0;

	// check if the input is valid and is an axis
	if( input )
	{
		auto element = input->GetElement();

		switch( element )
		{
		case DeviceEnums::InputElementDescriptor::DPad:
			break;
		default:
			// everything else is invalid
			CCP_LOGERR( "ControllerSwitchInputEvent::SetInput: Invalid input element for switch: %s. Ignoring the assignment", DeviceEnums::ToKeyString( element ) );
			break;
		}

		m_element = element;
		m_index = input->GetIndex();
		m_attached = true;
	}
}
