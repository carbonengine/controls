#pragma once

#include "../../StdAfx.h"
#include "../IInputEvent.h"

BLUE_CLASS( ControllerSwitchInputEvent ) :
	public IInputEvent
{
public:

	EXPOSE_TO_BLUE();
	ControllerSwitchInputEvent( IRoot* lockobj = nullptr );
	bool Match( const Events::State& state ) override;
	void Own( Events::State& state ) override;

private:
	uint32_t m_switchIndex{ 0 };
	Events::SwitchPosition m_event{ Events::SwitchPosition::Any };
	Events::SwitchPosition m_state{ Events::SwitchPosition::Center };
};

TYPEDEF_BLUECLASS( ControllerSwitchInputEvent );