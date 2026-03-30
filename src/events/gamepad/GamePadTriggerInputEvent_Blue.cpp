#include "GamePadTriggerInputEvent.h"


BLUE_DEFINE( GamePadTriggerInputEvent );

const Be::ClassInfo* GamePadTriggerInputEvent::ExposeToBlue()
{
	EXPOSURE_BEGIN( GamePadTriggerInputEvent, "Gamepad trigger input event" )
		MAP_INTERFACE( GamePadTriggerInputEvent )
		MAP_INTERFACE( IInputEvent )
		MAP_ATTRIBUTE_WITH_CHOOSER( "side", m_side, "The side of the trigger on the device", Be::READWRITE | Be::ENUM, Events::SideChooser )
		MAP_ATTRIBUTE( "minThreshold", m_minThreshold, "The minimum amount the trigger needs to be pressed for the event to fire", Be::READWRITE )
		MAP_ATTRIBUTE( "maxThreshold", m_maxThreshold, "The maximum amount the trigger needs to be pressed for the event to fire", Be::READWRITE )
	EXPOSURE_END()
}