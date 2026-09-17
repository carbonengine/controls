#include "InputEventTrigger.h"
#include "InputEvent.h"

BLUE_DEFINE( InputEventTrigger );

const Be::ClassInfo* InputEventTrigger::ExposeToBlue()
{
	EXPOSURE_BEGIN( InputEventTrigger, "Gamepad input trigger event" )
		MAP_INTERFACE( InputEventTrigger )
		MAP_ATTRIBUTE( "events", m_events, "The events that need to be fulfilled for the callback to be executed", Be::READ )
		MAP_ATTRIBUTE( "callback", m_callback, "The callback to be executed when the events are fulfilled", Be::READWRITE )
		MAP_ATTRIBUTE( "repeat", m_repeat, "Whether the callback is executed for every matching state instead of only the first one", Be::READWRITE )
	EXPOSURE_END()
}
