#include "ControllerSwitchInputEvent.h"

ControllerSwitchInputEvent::ControllerSwitchInputEvent( IRoot* lockobj )
{
}

bool ControllerSwitchInputEvent::Match( Events::State state )
{
	if( m_switchIndex < state.controllerState.switches.size() )
	{
		return state.controllerState.switches[m_switchIndex] == m_position;
	}

	return false;
}
