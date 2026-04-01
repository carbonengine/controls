#pragma once

#include "../../StdAfx.h"
#include "../IInputEvent.h"

BLUE_CLASS( ControllerButtonInputEvent ) :
	public IInputEvent
{
public:

	EXPOSE_TO_BLUE();
	ControllerButtonInputEvent( IRoot* lockobj = nullptr );
	bool Match( Events::State state ) override;

private:
	uint32_t m_buttonIndex = 0;
	Events::ButtonState m_position{ Events::ButtonState::Pressed };
};

TYPEDEF_BLUECLASS( ControllerButtonInputEvent );