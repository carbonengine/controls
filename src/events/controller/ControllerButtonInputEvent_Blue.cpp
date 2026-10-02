// Copyright © 2026 CCP ehf.

#include "ControllerButtonInputEvent.h"


namespace Events
{
const Be::VarChooser ButtonStateChooser[] = {
	{ "Up", BeCast( ButtonState::Up ), "Indicates that a button is not pressed" },
	{ "Down", BeCast( ButtonState::Down ), "Indicates that a button is down, from the first update it is pressed (no time checks are performed)" },
	{ "Pressed", BeCast( ButtonState::Pressed ), "Indicates that a button was tapped (went down and up within the held time); matches on the release" },
	{ "Released", BeCast( ButtonState::Released ), "Indicates that a button was held for at least the held time and then released; matches on the release" },
	{ "Held", BeCast( ButtonState::Held ), "Indicates that a button is being held down for longer than the held time" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "ButtonState", ButtonState, ButtonStateChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

};

BLUE_DEFINE( ControllerButtonInputEvent );

const Be::ClassInfo* ControllerButtonInputEvent::ExposeToBlue()
{
	EXPOSURE_BEGIN( ControllerButtonInputEvent, "Controller button input event" )
		MAP_INTERFACE( ControllerButtonInputEvent )

		MAP_ATTRIBUTE( "conditionsMet", m_conditionsMet, "Whether the button conditions were met in the most recent evaluation, regardless of ownership", Be::READ )
		MAP_ATTRIBUTE( "previouslyPressed", m_previouslyPressed, "Whether the button was pressed on the previous evaluation", Be::READ )
		MAP_ATTRIBUTE( "previousStateChangeTimestamp", m_previousStateChangeTimestamp, "Timestamp of the last state transition", Be::READ )

		MAP_ATTRIBUTE_WITH_CHOOSER( "event", m_event, "The event to listen to", Be::READWRITE | Be::ENUM, Events::ButtonStateChooser )
	EXPOSURE_CHAINTO( InputEvent )
}
