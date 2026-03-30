#pragma once
#include "../../StdAfx.h"
#include "../IInputEvent.h"

BLUE_CLASS( BatteryLowEvent ) :
	public IInputEvent
{
public:
	EXPOSE_TO_BLUE();
	BatteryLowEvent( IRoot* lockobj = nullptr );
	bool Match( Events::State state ) override;
private:
	float m_threshold{ 0.2f }; // The battery level threshold below which the event will match
};

TYPEDEF_BLUECLASS( BatteryLowEvent );
