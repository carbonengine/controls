#include "ControllerButtonInputEvent.h"

BLUE_DEFINE( ControllerButtonInputEvent );

const Be::ClassInfo* ControllerButtonInputEvent::ExposeToBlue()
{
	EXPOSURE_BEGIN( ControllerButtonInputEvent, "Controller button input event" )
		MAP_INTERFACE( ControllerButtonInputEvent )
		MAP_INTERFACE( IInputEvent )

		MAP_ATTRIBUTE( "buttonIndex", m_buttonIndex, "The button to listen to", Be::READWRITE )
		MAP_ATTRIBUTE_WITH_CHOOSER( "event", m_position, "The event to listen to", Be::READWRITE |  Be::ENUM, Events::ButtonStateChooser )
	EXPOSURE_END()
}
