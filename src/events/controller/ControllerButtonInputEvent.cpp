#include "ControllerButtonInputEvent.h"

ControllerButtonInputEvent::ControllerButtonInputEvent( IRoot* lockobj )
{
}

bool ControllerButtonInputEvent::Match( const Events::State& state )
{
	if( m_buttonIndex < state.buttons.size() )
	{
		auto button = state.buttons[m_buttonIndex];
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
	state.buttons[m_buttonIndex].matched = true;
}
