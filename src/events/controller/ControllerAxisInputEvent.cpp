#include "ControllerAxisInputEvent.h"

const float AXIS_EPSILON = 0.005f;

ControllerAxisInputEvent::ControllerAxisInputEvent( IRoot* lockobj )
{
}

bool ControllerAxisInputEvent::Match( Events::State state )
{
	if( m_axisIndex < state.axis.size() )
	{
		float previousValue = m_value;
		if( std::abs( state.axis[m_axisIndex] - m_value ) < AXIS_EPSILON )
		{
			return false; // ignore small changes in axis value
		}
		m_value = state.axis[m_axisIndex];
		m_delta = m_value - previousValue;

		return m_delta != 0.0f;
	}
	return false;
}
