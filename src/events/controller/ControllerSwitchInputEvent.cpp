#include "ControllerSwitchInputEvent.h"

ControllerSwitchInputEvent::ControllerSwitchInputEvent( IRoot* lockobj )
{
}

bool ControllerSwitchInputEvent::Match( const Events::State& state )
{
	if( m_input )
	{
		auto element = m_input->GetElement();
		auto index = m_input->GetIndex();
		auto it = std::find_if( state.switches.begin(), state.switches.end(), [element, index]( const Events::Switch& switchState ) {
			return switchState.descriptor == element && switchState.index == index;
		} );
		auto& switchState = *it;

		return !switchState.matched && (switchState.position == m_event || m_event == Events::SwitchPosition::Any);
	}

	return false;
}

void ControllerSwitchInputEvent::Own( Events::State& state )
{
	if( m_input )
	{
		auto element = m_input->GetElement();
		auto index = m_input->GetIndex();
		auto it = std::find_if( state.switches.begin(), state.switches.end(), [element, index]( const Events::Switch& switchState ) {
			return switchState.descriptor == element && switchState.index == index;
		} );
		auto& switchState = *it;
		switchState.matched = true;
		m_state = switchState.position;
	}
}

void ControllerSwitchInputEvent::SetInput( InputElement* input )
{
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
	}
	m_input = input;
}

InputElement* ControllerSwitchInputEvent::GetInput() const
{
	return m_input;
}