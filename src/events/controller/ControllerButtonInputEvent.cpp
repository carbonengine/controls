#include "ControllerButtonInputEvent.h"

ControllerButtonInputEvent::ControllerButtonInputEvent( IRoot* lockobj )
{
}

bool ControllerButtonInputEvent::Match( const Events::State& state )
{
	if( m_attached && state.buttons.size() > 0 )
	{
		auto it = std::find_if( state.buttons.begin(), state.buttons.end(), [&]( const Events::Button& button ) {
			return button.descriptor == m_element && button.index == m_index;
		} );

		if( it == state.buttons.end() )
		{
			CCP_LOGERR( "ControllerButtonInputEvent::Match: Could not find button state for element %s index %d", DeviceEnums::ToKeyString( m_element ), m_index );
			return false;
		}

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
	if( !m_attached )
	{
		CCP_LOGERR( "ControllerButtonInputEvent::Own: Cannot own button state because no input element is attached." );
		return;
	}
	auto it = std::find_if( state.buttons.begin(), state.buttons.end(), [&]( const Events::Button& button ) {
		return button.descriptor == m_element && button.index == m_index;
	} );

	if( it == state.buttons.end() )
	{
		CCP_LOGERR( "ControllerButtonInputEvent::Own: Could not find button state for element %s index %d", DeviceEnums::ToKeyString( m_element ), m_index );
		return;
	}

	auto& button = *it;
	button.matched = true;
}

void ControllerButtonInputEvent::AttachTo( const InputElement* input )
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

		m_element = element;
		m_index = input->GetIndex();
		m_attached = true;
	}
}
