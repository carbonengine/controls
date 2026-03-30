#include "BatteryChargingEvent.h"

BatteryChargingEvent::BatteryChargingEvent( IRoot* lockobj )
{
}

bool BatteryChargingEvent::Match( Events::State state )
{
	if( state.batteryState.charging != m_charging )
	{
		m_charging = state.batteryState.charging;

		return (m_charging && m_event == ChargingState::StartedCharging) || (!m_charging && m_event == ChargingState::StoppedCharging);
	}
	return false; // No change in charging state, so no match
}