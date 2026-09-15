#include "ControllerSwitchInputEvent.h"

ControllerSwitchInputEvent::ControllerSwitchInputEvent( IRoot* lockobj )
{
}

bool ControllerSwitchInputEvent::Match( const Events::State& state )
{
	m_previouslyMatched = m_matched;
	m_matched = Evaluate( state );
	return m_matched;
}

bool ControllerSwitchInputEvent::JustMatched()
{
	return m_matched && !m_previouslyMatched;
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

		return !switchState.matched && (switchState.position == m_event || m_event == Events::SwitchPosition::Any);
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
	m_state = switchState.position;
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
		case DeviceEnums::InputElementDescriptor::Unknown:
			// invalid input element for switch
			CCP_LOGERR( "ControllerSwitchInputEvent::SetInput: Invalid input element for switch: %s. Ignoring the assignment", DeviceEnums::ToKeyString( element ) );
			return;
		default:
			// everything else is valid
			break;
		}

		m_element = element;
		m_index = input->GetIndex();
		m_attached = true;
	}
}