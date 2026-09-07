#include "ControllerButtonInputEvent.h"

ControllerButtonInputEvent::ControllerButtonInputEvent( IRoot* lockobj )
{
}

bool ControllerButtonInputEvent::Match( const Events::State& state )
{
	if( m_input )
	{
		auto element = m_input->GetElement();
		auto index = m_input->GetIndex();
		auto it = std::find_if( state.buttons.begin(), state.buttons.end(), [element, index]( const Events::Button& button ) {
			return button.descriptor == element && button.index == index;
		} );

		auto button = *it;
		if( button.matched )
		{
			return false;
		}

		bool matched = false;
		switch( m_event )
		{
		case Events::ButtonState::Up:
			matched = !button.pressed && !m_previouslyPressed;
			break;
		case Events::ButtonState::Down:
			matched = button.pressed && m_previouslyPressed;
			break;
		case Events::ButtonState::Released:
			matched = !button.pressed && m_previouslyPressed && ( state.timestamp - m_previousStateChangeTimestamp >= Events::g_holdTimeInMicroSeconds );
			break;
		case Events::ButtonState::Held:
			matched = button.pressed && m_previouslyPressed && ( state.timestamp - m_previousStateChangeTimestamp >= Events::g_holdTimeInMicroSeconds );
			break;
		case Events::ButtonState::Pressed:
			matched = !button.pressed && m_previouslyPressed && ( state.timestamp - m_previousStateChangeTimestamp < Events::g_holdTimeInMicroSeconds );
			break;
		default:
			break;
		}

		if( m_previouslyPressed != button.pressed )
		{
			m_previousStateChangeTimestamp = state.timestamp;
			m_previouslyPressed = button.pressed;
		}

		return matched;
	}
	return false;
}

void ControllerButtonInputEvent::Own( Events::State& state )
{
	auto element = m_input->GetElement();
	auto index = m_input->GetIndex();
	auto it = std::find_if( state.buttons.begin(), state.buttons.end(), [element, index]( const Events::Button& button ) {
		return button.descriptor == element && button.index == index;
	} );
	auto& button = *it;
	button.matched = true;
}

void ControllerButtonInputEvent::SetInput( InputElement* input )
{
	// check if the input is valid and is an axis
	if( input )
	{
		auto element = input->GetElement();

		switch( element )
		{
		case DeviceEnums::InputElementDescriptor::LeftStickX:
		case DeviceEnums::InputElementDescriptor::LeftStickY:
		case DeviceEnums::InputElementDescriptor::RightStickX:
		case DeviceEnums::InputElementDescriptor::RightStickY:
		case DeviceEnums::InputElementDescriptor::LeftTriggerAxis:
		case DeviceEnums::InputElementDescriptor::RightTriggerAxis:
		case DeviceEnums::InputElementDescriptor::DPad:
			// invalid input element for button
			CCP_LOGERR( "ControllerButtonInputEvent::SetInput: Invalid input element for button: %s. Ignoring the assignment", DeviceEnums::ToKeyString( element ) );
			return;
		default:
			// everything else is valid
			break;
		}
	}
	m_input = input;
}

InputElement* ControllerButtonInputEvent::GetInput() const
{
	return m_input;
}