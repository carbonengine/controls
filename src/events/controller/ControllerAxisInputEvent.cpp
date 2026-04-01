#include "ControllerAxisInputEvent.h"

const float AXIS_ZERO_POSITION = 0.0f;

ControllerAxisInputEvent::ControllerAxisInputEvent( IRoot* lockobj )
{
}

bool ControllerAxisInputEvent::Match( Events::State state )
{
	if( m_axisIndex < state.controllerState.axis.size() )
	{
		// Compare the axis value with the expected value
		return state.controllerState.axis[m_axisIndex] != AXIS_ZERO_POSITION;
	}
	return false;
}
