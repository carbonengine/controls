#pragma once
#include "../../StdAfx.h"
#include "../IInputEvent.h"

BLUE_CLASS( FlightStickButtonInputEvent ) :
	public IInputEvent
{
public:
	enum class FlightStickButtonEventType
	{
		PrimaryFire,
		SecondaryFire,
	};

	EXPOSE_TO_BLUE();
	FlightStickButtonInputEvent( IRoot* lockobj = nullptr );
	bool Match( Events::State state ) override;

private:
	FlightStickButtonEventType m_button { FlightStickButtonEventType::PrimaryFire };
	Events::ButtonState m_position{ Events::ButtonState::Pressed };
};

TYPEDEF_BLUECLASS( FlightStickButtonInputEvent );
