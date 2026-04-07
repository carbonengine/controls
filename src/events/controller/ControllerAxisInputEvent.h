#pragma once

#include "../../StdAfx.h"
#include "../IInputEvent.h"

BLUE_CLASS( ControllerAxisInputEvent ) :
	public IInputEvent
{
public:
	EXPOSE_TO_BLUE();
	ControllerAxisInputEvent( IRoot* lockobj = nullptr );
	bool Match( Events::State state ) override;

private:
	uint32_t m_axisIndex = 0;
	float m_value = 0.0f;
	float m_delta = 0.0f;
};

TYPEDEF_BLUECLASS( ControllerAxisInputEvent );