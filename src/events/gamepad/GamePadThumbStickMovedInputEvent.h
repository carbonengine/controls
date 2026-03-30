#pragma once
#include "../../StdAfx.h"
#include "../IInputEvent.h"

BLUE_CLASS( GamePadThumbStickMovedInputEvent ) :
	public IInputEvent
{
public:
	EXPOSE_TO_BLUE();

	GamePadThumbStickMovedInputEvent( IRoot* lockobj = nullptr );
	bool Match( Events::State state ) override;
private:
	Events::Side m_side{ Events::Side::Left };
};

TYPEDEF_BLUECLASS( GamePadThumbStickMovedInputEvent );