#pragma once
#include "../../StdAfx.h"
#include "../IInputEvent.h"

BLUE_CLASS( FlightStickMovementInputEvent ) :
	public IInputEvent
{
public:
	enum class FlightStickEventType
	{
		Yaw = 1 << 0,
		Pitch = 1 << 1,
		Roll = 1 << 2,
		Any = Yaw | Pitch | Roll
	};

	EXPOSE_TO_BLUE();
	FlightStickMovementInputEvent( IRoot* lockobj = nullptr );
	bool Match( Events::State state ) override;

private:
	FlightStickEventType m_position{ FlightStickEventType::Any };
};

TYPEDEF_BLUECLASS( FlightStickMovementInputEvent );
