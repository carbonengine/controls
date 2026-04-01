#include "FlightStickButtonInputEvent.h"

FlightStickButtonInputEvent::FlightStickButtonInputEvent( IRoot* lockobj )
{
}

bool FlightStickButtonInputEvent::Match( Events::State state )
{
	Events::FlightStickState flightStickState = state.flightStickState;
	switch( m_button )
	{
	case FlightStickButtonEventType::PrimaryFire:
		return flightStickState.firePrimary.state == m_position;
	case FlightStickButtonEventType::SecondaryFire:
		return flightStickState.fireSecondary.state == m_position;
	default:
		return false;
	};
}