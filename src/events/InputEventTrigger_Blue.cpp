#include "InputEventTrigger.h"
#include "IInputEvent.h"

BLUE_DEFINE( InputEventTrigger );
BLUE_DEFINE_INTERFACE( IInputEvent );

namespace Events
{
const Be::VarChooser ButtonStateChooser[] = {
	{ "Pressed", BeCast( ButtonState::Pressed ), "Indicates that a button was pressed" },
	{ "Released", BeCast( ButtonState::Released ), "Indicates that a button was released" },
	{ "Held", BeCast( ButtonState::Held ), "Indicates that a button is being held down" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "ButtonState", ButtonState, ButtonStateChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

const Be::VarChooser SideChooser[] = {
	{ "Left", BeCast( Side::Left ), "Indicates the left side of the controller" },
	{ "Right", BeCast( Side::Right ), "Indicates the right side of the controller" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "Side", Side, SideChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE);
};

const Be::ClassInfo* InputEventTrigger::ExposeToBlue()
{
	EXPOSURE_BEGIN( InputEventTrigger, "Gamepad callback trigger event" )
		MAP_INTERFACE( InputEventTrigger )
		MAP_ATTRIBUTE( "events", m_events, "The events that need to be fulfilled for the callback to be executed", Be::READ )
		MAP_ATTRIBUTE( "callback", m_callback, "The callback to be executed when the events are fulfilled", Be::READWRITE )
	EXPOSURE_END()
}