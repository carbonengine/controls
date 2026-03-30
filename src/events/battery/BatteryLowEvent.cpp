#include "BatteryLowEvent.h"

BatteryLowEvent::BatteryLowEvent( IRoot* lockobj )
{
}

bool BatteryLowEvent::Match( Events::State state )
{
	float batteryLevel = state.batteryState.remainingCapacity / state.batteryState.fullChargeCapacity;
	return batteryLevel < max(0.0f, min(1.0f, m_threshold)) && !state.batteryState.charging;
}