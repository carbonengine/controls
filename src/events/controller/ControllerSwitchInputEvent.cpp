#include "ControllerSwitchInputEvent.h"

ControllerSwitchInputEvent::ControllerSwitchInputEvent( IRoot* lockobj )
{
}

bool ControllerSwitchInputEvent::Match( Events::State state )
{
	if( m_switchIndex < state.switches.size() )
	{
		m_state = state.switches[m_switchIndex];
		return m_state == m_event || m_event == Events::SwitchPosition::Any;
	}

	return false;
}
