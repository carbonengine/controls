#include "ControllerAxisInputEvent.h"

ControllerAxisInputEvent::ControllerAxisInputEvent( IRoot* lockobj )
{
}

bool ControllerAxisInputEvent::Match( Events::State state )
{
	if( m_axisIndex < state.axis.size() )
	{
		float previousValue = m_value;
		if( std::abs( state.axis[m_axisIndex] - m_value ) < Events::AXIS_THRESHOLD )
		{
			m_delta = 0.0f;
			return false; // ignore small changes in axis value
		}
		m_value = state.axis[m_axisIndex];
		m_delta = m_value - previousValue;

		if( !m_initialized )
		{
			m_initialized = true;
			// ignore the first update since some axes may not start at a neutral value of 0.0f
			// this prevents unintended input events from being triggered on initialization
			return false; 
		}
		return m_delta != 0.0f;
	}
	return false;
}
