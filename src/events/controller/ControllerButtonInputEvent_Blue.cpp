#include "ControllerButtonInputEvent.h"

BLUE_DEFINE( ControllerButtonInputEvent );

const Be::ClassInfo* ControllerButtonInputEvent::ExposeToBlue()
{
	EXPOSURE_BEGIN( ControllerButtonInputEvent, "Controller button input event" )
		MAP_INTERFACE( ControllerButtonInputEvent )
		MAP_INTERFACE( IInputEvent )

		MAP_METHOD_AND_WRAP( "AttachTo", AttachTo, "Attaches the input element to the event" )
		MAP_ATTRIBUTE( "attached", m_attached, "Whether the input element has been attached to a physical input", Be::READ )
		MAP_ATTRIBUTE( "element", m_element, "The input element to monitor for button state changes", Be::READ )
		MAP_ATTRIBUTE( "index", m_index, "The index of the input element in the device's element array", Be::READ )

		MAP_ATTRIBUTE( "previouslyPressed", m_previouslyPressed, "Whether the button was pressed on the previous update", Be::READ )
		MAP_ATTRIBUTE( "previousStateChangeTimestamp", m_previousStateChangeTimestamp, "Timestamp of the last press/release transition", Be::READ )

		MAP_ATTRIBUTE_WITH_CHOOSER( "event", m_event, "The event to listen to", Be::READWRITE | Be::ENUM, Events::ButtonStateChooser )
	EXPOSURE_END()
}
