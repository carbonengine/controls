#include "InputEvent.h"

BLUE_DEFINE( InputEvent );

const Be::ClassInfo* InputEvent::ExposeToBlue()
{
	EXPOSURE_BEGIN( InputEvent, "Gamepad callback input event" )
		MAP_INTERFACE( InputEvent )

		MAP_METHOD_AND_WRAP( "AttachTo", AttachTo, "Attaches the input element to the event" )

		MAP_ATTRIBUTE( "attached", m_attached, "Whether an input element has been attached to a physical input", Be::READ )
		MAP_ATTRIBUTE( "descriptor", m_key.descriptor, "The descriptor of the input element", Be::READ )
		MAP_ATTRIBUTE( "index", m_key.index, "The index of the input element", Be::READ )
	EXPOSURE_END()
}