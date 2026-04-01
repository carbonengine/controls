#include "FlightStickButtonInputEvent.h"

BLUE_DEFINE( FlightStickButtonInputEvent );

namespace
{
Be::VarChooser FlightStickButtonInputEventChooser[] = {
	{ "PrimaryFire", BeCast( FlightStickButtonInputEvent::FlightStickButtonEventType::PrimaryFire ), "Indicates the primary fire button" },
	{ "SecondaryFire", BeCast( FlightStickButtonInputEvent::FlightStickButtonEventType::SecondaryFire ), "Indicates the secondary fire button" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "FlightStickButtonEventType", FlightStickButtonInputEvent::FlightStickButtonEventType, FlightStickButtonInputEventChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );
}

const Be::ClassInfo* FlightStickButtonInputEvent::ExposeToBlue()
{
	EXPOSURE_BEGIN( FlightStickButtonInputEvent, "Flight stick button input event" )
		MAP_INTERFACE( FlightStickButtonInputEvent )
		MAP_INTERFACE( IInputEvent )

		MAP_ATTRIBUTE_WITH_CHOOSER( "button", m_button, "The button pressed", Be::READWRITE | Be::ENUM, FlightStickButtonInputEventChooser )
		MAP_ATTRIBUTE_WITH_CHOOSER( "event", m_position, "The event", Be::READWRITE | Be::ENUM, Events::ButtonStateChooser )
	EXPOSURE_END()
}
