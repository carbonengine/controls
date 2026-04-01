#include "GamePadThumbStickPressedInputEvent.h"


BLUE_DEFINE( GamePadThumbStickPressedInputEvent );


const Be::ClassInfo* GamePadThumbStickPressedInputEvent::ExposeToBlue()
{
	EXPOSURE_BEGIN( GamePadThumbStickPressedInputEvent, "Gamepad thumbstick input event" )
		MAP_INTERFACE( GamePadThumbStickPressedInputEvent )
		MAP_INTERFACE( IInputEvent )
		MAP_ATTRIBUTE_WITH_CHOOSER( "side", m_side, "The side of the thumbstick on the device", Be::READWRITE | Be::ENUM, Events::SideChooser )
		MAP_ATTRIBUTE_WITH_CHOOSER( "event", m_position, "The type of button event", Be::READWRITE | Be::ENUM, Events::ButtonStateChooser )
	EXPOSURE_END()
}