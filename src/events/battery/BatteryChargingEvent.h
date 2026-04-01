#pragma once
#include "../../StdAfx.h"
#include "../IInputEvent.h"

BLUE_CLASS( BatteryChargingEvent ) :
	public IInputEvent
{
public:

	enum class ChargingState
	{
		StartedCharging = 0,
		StoppedCharging = 1
	};

	EXPOSE_TO_BLUE();
	BatteryChargingEvent( IRoot* lockobj = nullptr );
	bool Match( Events::State state ) override;

private:

	BatteryChargingEvent::ChargingState m_position{ BatteryChargingEvent::ChargingState::StoppedCharging };
	bool m_charging{ false };
};

TYPEDEF_BLUECLASS( BatteryChargingEvent );
