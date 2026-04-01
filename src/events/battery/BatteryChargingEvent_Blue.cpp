#include "BatteryChargingEvent.h"

BLUE_DEFINE( BatteryChargingEvent );

namespace
{
Be::VarChooser ChargingStateChooser[] = {
	{ "StartedCharging", BeCast( BatteryChargingEvent::ChargingState::StartedCharging ), "Indicates that the battery has started charging" },
	{ "StoppedCharging", BeCast( BatteryChargingEvent::ChargingState::StoppedCharging ), "Indicates that the battery has stopped charging" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "ChargingState", BatteryChargingEvent::ChargingState, ChargingStateChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );
}

const Be::ClassInfo* BatteryChargingEvent::ExposeToBlue()
{
	EXPOSURE_BEGIN( BatteryChargingEvent, "Battery charging event" )
		MAP_INTERFACE( BatteryChargingEvent )
		MAP_INTERFACE( IInputEvent )

		MAP_ATTRIBUTE_WITH_CHOOSER( "event", m_position, "The battery charging event state", Be::READWRITE | Be::ENUM, ChargingStateChooser)
	EXPOSURE_END()
}
