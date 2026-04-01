#include "FlightStickMovementInputEvent.h"


FlightStickMovementInputEvent::FlightStickMovementInputEvent( IRoot* lockobj )
{
}

bool FlightStickMovementInputEvent::Match( Events::State state )
{
	Events::FlightStickState flightStickState = state.flightStickState;
	if( ( ( (uint32_t)m_position & static_cast<uint32_t>( FlightStickEventType::Yaw ) ) != 0 ) && flightStickState.yaw != 0.0f )
	{
		return true;
	}
	else if( ( ( (uint32_t)m_position & static_cast<uint32_t>( FlightStickEventType::Pitch ) ) != 0 ) && flightStickState.pitch != 0.0f )
	{
		return true;
	}
	else if( ( ( (uint32_t)m_position & static_cast<uint32_t>( FlightStickEventType::Roll ) ) != 0 ) && flightStickState.roll != 0.0f )
	{
		return true;
	}
	return false;
}