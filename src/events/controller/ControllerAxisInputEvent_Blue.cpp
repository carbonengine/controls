#include "ControllerAxisInputEvent.h"

BLUE_DEFINE( ControllerAxisInputEvent );

const Be::ClassInfo* ControllerAxisInputEvent::ExposeToBlue()
{
	EXPOSURE_BEGIN( ControllerAxisInputEvent, "Controller axis input event" )
		MAP_INTERFACE( ControllerAxisInputEvent )
		MAP_INTERFACE( IInputEvent )

		MAP_METHOD_AND_WRAP( "AttachTo", AttachTo, "Attaches the input element to the event" )
		MAP_ATTRIBUTE( "attached", m_attached, "Whether the input element has been attached to a physical input", Be::READ )
		MAP_ATTRIBUTE( "element", m_element, "The input element to monitor for button state changes", Be::READ )
		MAP_ATTRIBUTE( "index", m_index, "The index of the input element in the device's element array", Be::READ )

		MAP_ATTRIBUTE( "value", m_value, "The value of the axis", Be::READ )
		MAP_ATTRIBUTE( "delta", m_delta, "The change in value since the last update", Be::READ )
	EXPOSURE_END()
}
