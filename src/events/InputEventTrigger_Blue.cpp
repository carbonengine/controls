#include "InputEventTrigger.h"
#include "IInputEvent.h"

BLUE_DEFINE( InputEventTrigger );
BLUE_DEFINE_INTERFACE( IInputEvent );

namespace Events
{
const Be::VarChooser ButtonStateChooser[] = {
	{ "Up", BeCast( ButtonState::Up ), "Indicates that a button is not pressed" },
	{ "Down", BeCast( ButtonState::Down ), "Indicates that a button is down (but no time checks are performed)" },
	{ "Pressed", BeCast( ButtonState::Pressed ), "Indicates that a button is pressed (i.e went down and up within the held time)" },
	{ "Released", BeCast( ButtonState::Released ), "Indicates that a button was held and then released" },
	{ "Held", BeCast( ButtonState::Held ), "Indicates that a button is being held down for longer than the held time" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "ButtonState", ButtonState, ButtonStateChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

};

const Be::ClassInfo* InputEventTrigger::ExposeToBlue()
{
	EXPOSURE_BEGIN( InputEventTrigger, "Gamepad callback trigger event" )
		MAP_INTERFACE( InputEventTrigger )
		MAP_ATTRIBUTE( "events", m_events, "The events that need to be fulfilled for the callback to be executed", Be::READ )
		MAP_ATTRIBUTE( "callback", m_callback, "The callback to be executed when the events are fulfilled", Be::READWRITE )
		MAP_ATTRIBUTE( "repeat", m_repeat, "Whether the callback is executed for every matching state instead of only the first one", Be::READWRITE )
		MAP_ATTRIBUTE( "triggered", m_triggered, "Whether the callback has already been executed for the current uninterrupted match", Be::READ )
	EXPOSURE_END()
}