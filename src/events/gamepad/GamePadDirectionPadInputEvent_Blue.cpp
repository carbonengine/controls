#include "GamePadDirectionPadInputEvent.h"


BLUE_DEFINE( GamePadDirectionPadInputEvent );

namespace
{
	Be::VarChooser DirectionPadButtonTypeChooser[] = {
	{ "Up", BeCast( GamePadDirectionPadInputEvent::DirectionPadButtonType::Up ), "The up button on the dpad" },
	{ "Down", BeCast( GamePadDirectionPadInputEvent::DirectionPadButtonType::Down ), "The down button on the dpad" },
	{ "Left", BeCast( GamePadDirectionPadInputEvent::DirectionPadButtonType::Left ), "The left button on the dpad" },
	{ "Right", BeCast( GamePadDirectionPadInputEvent::DirectionPadButtonType::Right ), "The right button on the dpad" },
	{ "UpLeft", BeCast( GamePadDirectionPadInputEvent::DirectionPadButtonType::UpLeft ), "The up-left diagonal on the dpad" },
	{ "UpRight", BeCast( GamePadDirectionPadInputEvent::DirectionPadButtonType::UpRight ), "The up-right diagonal on the dpad" },
	{ "DownLeft", BeCast( GamePadDirectionPadInputEvent::DirectionPadButtonType::DownLeft ), "The down-left diagonal on the dpad" },
	{ "DownRight", BeCast( GamePadDirectionPadInputEvent::DirectionPadButtonType::DownRight ), "The down-right diagonal on the dpad" },
	{ "Any", BeCast( GamePadDirectionPadInputEvent::DirectionPadButtonType::Any ), "Any button or combination of buttons on the dpad" },
	{ 0 }
};
}

const Be::ClassInfo* GamePadDirectionPadInputEvent::ExposeToBlue()
{
	EXPOSURE_BEGIN( GamePadDirectionPadInputEvent, "Gamepad thumbstick moved input event" )
		MAP_INTERFACE( GamePadDirectionPadInputEvent )
		MAP_INTERFACE( IInputEvent )
		MAP_ATTRIBUTE_WITH_CHOOSER( "button", m_button, "The type of dpad event, if multiple then it is enough to press one of them (f.ex if all are registered then it is enough to press up)", Be::READWRITE | Be::ENUM, DirectionPadButtonTypeChooser )
		MAP_ATTRIBUTE_WITH_CHOOSER( "event", m_event, "The type of button event", Be::READWRITE | Be::ENUM, Events::ButtonStateChooser )
	EXPOSURE_END()
}