#pragma once

#include "../../StdAfx.h"
#include "../IInputEvent.h"

BLUE_CLASS( ControllerButtonInputEvent ) :
	public IInputEvent
{
public:

	EXPOSE_TO_BLUE();
	ControllerButtonInputEvent( IRoot* lockobj = nullptr );
	bool Match( const Events::State& state ) override;
	void Own( Events::State& state ) override;

private:
	uint32_t m_buttonIndex = 0;
	Events::ButtonState m_event{ Events::ButtonState::Pressed };

	bool m_previouslyPressed = false;
	uint64_t m_previousStateChangeTimestamp = 0;
};

TYPEDEF_BLUECLASS( ControllerButtonInputEvent );