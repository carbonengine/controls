#include "ControllerAxisInputEvent.h"

BLUE_DEFINE( ControllerAxisInputEvent );

const Be::ClassInfo* ControllerAxisInputEvent::ExposeToBlue()
{
	EXPOSURE_BEGIN( ControllerAxisInputEvent, "Controller axis input event" )
		MAP_INTERFACE( ControllerAxisInputEvent )
		MAP_INTERFACE( IInputEvent )

		MAP_ATTRIBUTE( "axisIndex", m_axisIndex, "The axis to listen to", Be::READWRITE )
	EXPOSURE_END()
}
