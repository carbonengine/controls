#include "ControllerSwitchInputEvent.h"

BLUE_DEFINE( ControllerSwitchInputEvent );
namespace
{
Be::VarChooser SwitchPositionChooser[] = {
	{ "Center", BeCast( Events::SwitchPosition::Center ), "Center" },
	{ "Up", BeCast( Events::SwitchPosition::Up ), "Up" },
	{ "UpRight", BeCast( Events::SwitchPosition::UpRight ), "Up and Right" },
	{ "Right", BeCast( Events::SwitchPosition::Right ), "Right" },
	{ "DownRight", BeCast( Events::SwitchPosition::DownRight ), "Down and Right" },
	{ "Down", BeCast( Events::SwitchPosition::Down ), "Down" },
	{ "DownLeft", BeCast( Events::SwitchPosition::DownLeft ), "Down and Left" },
	{ "Left", BeCast( Events::SwitchPosition::Left ), "Left" },
	{ "UpLeft", BeCast( Events::SwitchPosition::UpLeft ), "Up and Left" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "SwitchPosition", Events::SwitchPosition, SwitchPositionChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

}
const Be::ClassInfo* ControllerSwitchInputEvent::ExposeToBlue()
{
	EXPOSURE_BEGIN( ControllerSwitchInputEvent, "Controller switch input event" )
		MAP_INTERFACE( ControllerSwitchInputEvent )
		MAP_INTERFACE( IInputEvent )

		MAP_ATTRIBUTE( "switchIndex", m_switchIndex, "The switch to listen to", Be::READWRITE )
		MAP_ATTRIBUTE_WITH_CHOOSER( "position", m_position, "The event to listen to", Be::READWRITE | Be::ENUM, SwitchPositionChooser )
	EXPOSURE_END()
}
