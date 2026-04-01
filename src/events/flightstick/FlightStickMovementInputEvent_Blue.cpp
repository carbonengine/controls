#include "FlightStickMovementInputEvent.h"


namespace
{
Be::VarChooser FlightStickEventTypeChooser[] = {
	{ "Any", BeCast( FlightStickMovementInputEvent::FlightStickEventType::Any ), "Indicates any flight stick event" },
	{ "Pitch", BeCast( FlightStickMovementInputEvent::FlightStickEventType::Pitch ), "Indicates the pitch flight stick event" },
	{ "Roll", BeCast( FlightStickMovementInputEvent::FlightStickEventType::Roll ), "Indicates any roll flight stick event" },
	{ "Yaw", BeCast( FlightStickMovementInputEvent::FlightStickEventType::Yaw ), "Indicates any yaw flight stick event" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "FlightStickEventType", FlightStickMovementInputEvent::FlightStickEventType, FlightStickEventTypeChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );
}

BLUE_DEFINE( FlightStickMovementInputEvent );

const Be::ClassInfo* FlightStickMovementInputEvent::ExposeToBlue()
{
	EXPOSURE_BEGIN( FlightStickMovementInputEvent, "Flight stick movement input event" )
		MAP_INTERFACE( FlightStickMovementInputEvent )
		MAP_INTERFACE( IInputEvent )
		MAP_ATTRIBUTE_WITH_CHOOSER( "eventType", m_position, "The event type", Be::READWRITE | Be::ENUM, FlightStickEventTypeChooser )
	EXPOSURE_END()
}