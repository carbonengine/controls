#pragma once

#include "../../StdAfx.h"
#include "../IInputEvent.h"

BLUE_CLASS( ControllerSwitchInputEvent ) :
	public IInputEvent
{
public:

	EXPOSE_TO_BLUE();
	ControllerSwitchInputEvent( IRoot* lockobj = nullptr );
	bool Match( Events::State state ) override;

private:
	uint32_t m_switchIndex{ 0 };
	Events::SwitchPosition m_position{ Events::SwitchPosition::Center };
};

TYPEDEF_BLUECLASS( ControllerSwitchInputEvent );