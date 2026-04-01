#pragma once
#include "../../StdAfx.h"
#include "../IInputEvent.h"

BLUE_CLASS( GamePadThumbStickPressedInputEvent ) :
	public IInputEvent
{
public:
	EXPOSE_TO_BLUE();
	GamePadThumbStickPressedInputEvent( IRoot* lockobj = nullptr );
	bool Match( Events::State state ) override;
private:
	Events::Side m_side{ Events::Side::Left };
	Events::ButtonState m_position{ Events::ButtonState::Pressed };
};

TYPEDEF_BLUECLASS( GamePadThumbStickPressedInputEvent );