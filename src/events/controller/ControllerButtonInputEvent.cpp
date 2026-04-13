#include "ControllerButtonInputEvent.h"

ControllerButtonInputEvent::ControllerButtonInputEvent( IRoot* lockobj )
{
}

bool ControllerButtonInputEvent::Match( const Events::State& state )
{
	if( m_buttonIndex < state.buttons.size() )
	{
		auto button = state.buttons[m_buttonIndex];

		return !button.matched && button.state == m_event;
	}
	return false;
}

void ControllerButtonInputEvent::Own( Events::State& state )
{
	state.buttons[m_buttonIndex].matched = true;
}
