#include "GamePadThumbStickMovedInputEvent.h"


BLUE_DEFINE( GamePadThumbStickMovedInputEvent );


const Be::ClassInfo* GamePadThumbStickMovedInputEvent::ExposeToBlue()
{
	EXPOSURE_BEGIN( GamePadThumbStickMovedInputEvent, "Gamepad thumbstick moved input event" )
		MAP_INTERFACE( GamePadThumbStickMovedInputEvent )
		MAP_INTERFACE( IInputEvent )
		MAP_ATTRIBUTE_WITH_CHOOSER( "side", m_side, "The side of the thumbstick on the device", Be::READWRITE | Be::ENUM, Events::SideChooser )
	EXPOSURE_END()
}