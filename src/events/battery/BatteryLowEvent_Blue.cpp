#include "BatteryLowEvent.h"

BLUE_DEFINE( BatteryLowEvent );

const Be::ClassInfo* BatteryLowEvent::ExposeToBlue()
{
	EXPOSURE_BEGIN( BatteryLowEvent, "Battery low event" )
		MAP_INTERFACE( BatteryLowEvent )
		MAP_INTERFACE( IInputEvent )

		MAP_ATTRIBUTE( "threshold", m_threshold, "The battery level threshold below which the event will match (0 - 1)", Be::READWRITE )
	EXPOSURE_END()
}
