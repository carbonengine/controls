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
};

TYPEDEF_BLUECLASS( ControllerAxisInputEvent );