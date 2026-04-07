#include "ControllerAxisInputEvent.h"

ControllerAxisInputEvent::ControllerAxisInputEvent( IRoot* lockobj )
{
}

bool ControllerAxisInputEvent::Match( Events::State state )
{
	if( m_axisIndex < state.axis.size() )
	{
		float previousValue = m_value;
		m_value = state.axis[m_axisIndex];
		m_delta = m_value - previousValue;

		return m_delta != 0.0f;
	}
	return false;
}
