#include "ControllerSwitchInputEvent.h"

ControllerSwitchInputEvent::ControllerSwitchInputEvent( IRoot* lockobj )
{
}

bool ControllerSwitchInputEvent::Match( const Events::State& state )
{
	if( m_switchIndex < state.switches.size() )
	{
		auto switchState = state.switches[m_switchIndex];

		return !switchState.matched && (switchState.position == m_event || m_event == Events::SwitchPosition::Any);
	}

	return false;
}

void ControllerSwitchInputEvent::Own( Events::State& state )
{
	if( m_switchIndex < state.switches.size() )
	{
		auto& switchState = state.switches[m_switchIndex];
		switchState.matched = true;
		m_state = switchState.position;
	}
}