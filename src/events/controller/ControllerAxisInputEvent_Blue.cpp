#include "ControllerAxisInputEvent.h"

BLUE_DEFINE( ControllerAxisInputEvent );

const Be::ClassInfo* ControllerAxisInputEvent::ExposeToBlue()
{
	EXPOSURE_BEGIN( ControllerAxisInputEvent, "Controller axis input event" )
		MAP_INTERFACE( ControllerAxisInputEvent )
		MAP_INTERFACE( IInputEvent )

		MAP_PROPERTY( "input", GetInput, SetInput, "The input element to listen to" )
		MAP_ATTRIBUTE( "value", m_value, "The value of the axis", Be::READ )
		MAP_ATTRIBUTE( "delta", m_delta, "The change in value since the last update", Be::READ )
	EXPOSURE_END()
}
