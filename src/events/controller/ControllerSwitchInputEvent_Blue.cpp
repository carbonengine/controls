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
	{ "Any", BeCast( Events::SwitchPosition::Any ), "Any" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "SwitchPosition", Events::SwitchPosition, SwitchPositionChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

}
const Be::ClassInfo* ControllerSwitchInputEvent::ExposeToBlue()
{
	EXPOSURE_BEGIN( ControllerSwitchInputEvent, "Controller switch input event" )
		MAP_INTERFACE( ControllerSwitchInputEvent )
		MAP_INTERFACE( IInputEvent )

		MAP_METHOD_AND_WRAP( "AttachTo", AttachTo, "Attaches the input element to the event" )
		MAP_ATTRIBUTE( "attached", m_attached, "Whether the input element has been attached to a physical input", Be::READ )
		MAP_ATTRIBUTE( "element", m_element, "The input element to monitor for button state changes", Be::READ )
		MAP_ATTRIBUTE( "index", m_index, "The index of the input element in the device's element array", Be::READ )

		MAP_ATTRIBUTE_WITH_CHOOSER( "event", m_event, "The event to listen to", Be::READWRITE | Be::ENUM, SwitchPositionChooser )
		MAP_ATTRIBUTE_WITH_CHOOSER( "state", m_state, "The current state of the switch", Be::READWRITE | Be::ENUM, SwitchPositionChooser )
	EXPOSURE_END()
}
