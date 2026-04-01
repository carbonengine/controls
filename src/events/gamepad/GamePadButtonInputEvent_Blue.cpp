#include "GamePadButtonInputEvent.h"

BLUE_DEFINE( GamePadButtonInputEvent );
namespace
{
Be::VarChooser GamePadButtonTypeChooser[] = {
	{ "None", BeCast( GamePadButtonInputEvent::GamePadButtonType::None ), "No button" },
	{ "Menu", BeCast( GamePadButtonInputEvent::GamePadButtonType::Menu ), "The menu button" },
	{ "View", BeCast( GamePadButtonInputEvent::GamePadButtonType::View ), "The view button" },
	{ "A", BeCast( GamePadButtonInputEvent::GamePadButtonType::A ), "The A button" },
	{ "B", BeCast( GamePadButtonInputEvent::GamePadButtonType::B ), "The B button" },
	{ "X", BeCast( GamePadButtonInputEvent::GamePadButtonType::X ), "The X button" },
	{ "Y", BeCast( GamePadButtonInputEvent::GamePadButtonType::Y ), "The Y button" },
	{ "LeftShoulder", BeCast( GamePadButtonInputEvent::GamePadButtonType::LeftShoulder ), "The left shoulder button" },
	{ "RightShoulder", BeCast( GamePadButtonInputEvent::GamePadButtonType::RightShoulder ), "The right shoulder button" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "GamePadButtonType", GamePadButtonInputEvent::GamePadButtonType, GamePadButtonTypeChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

}
const Be::ClassInfo* GamePadButtonInputEvent::ExposeToBlue()
{
	EXPOSURE_BEGIN( GamePadButtonInputEvent, "Gamepad button input event" )
		MAP_INTERFACE( GamePadButtonInputEvent )
		MAP_INTERFACE( IInputEvent )

		MAP_ATTRIBUTE_WITH_CHOOSER( "button", m_button, "The button to listen to", Be::READWRITE | Be::ENUM, GamePadButtonTypeChooser )
		MAP_ATTRIBUTE_WITH_CHOOSER( "event", m_position, "The event to listen to", Be::READWRITE |  Be::ENUM, Events::ButtonStateChooser )
	EXPOSURE_END()
}
