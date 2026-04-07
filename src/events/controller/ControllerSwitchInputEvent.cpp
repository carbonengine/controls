#include "ControllerSwitchInputEvent.h"

ControllerSwitchInputEvent::ControllerSwitchInputEvent( IRoot* lockobj )
{
}

bool ControllerSwitchInputEvent::Match( Events::State state )
{
	if( m_switchIndex < state.switches.size() )
	{
		return state.switches[m_switchIndex] == m_position;
	}

	return false;
}
