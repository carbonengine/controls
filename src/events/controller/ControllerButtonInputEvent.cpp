#include "ControllerButtonInputEvent.h"

ControllerButtonInputEvent::ControllerButtonInputEvent( IRoot* lockobj )
{
}

bool ControllerButtonInputEvent::Match( Events::State state )
{
	if( m_buttonIndex < state.buttons.size() )
	{
		return state.buttons[m_buttonIndex].state == m_position;
	}
	return false;
}
