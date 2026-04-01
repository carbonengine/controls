#include "ControllerButtonInputEvent.h"

ControllerButtonInputEvent::ControllerButtonInputEvent( IRoot* lockobj )
{
}

bool ControllerButtonInputEvent::Match( Events::State state )
{
	if( m_buttonIndex < state.controllerState.buttons.size() )
	{
		return state.controllerState.buttons[m_buttonIndex].state == m_position;
	}
	return false;
}
