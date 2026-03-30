#pragma once
#include "../../StdAfx.h"
#include "../IInputEvent.h"

BLUE_CLASS( GamePadTriggerInputEvent ) :
	public IInputEvent
{
public:
	EXPOSE_TO_BLUE();
	GamePadTriggerInputEvent( IRoot* lockobj = nullptr );
	bool Match( Events::State state ) override;

private:
	// minimum amount the trigger needs to be pressed for the event to fire, from 0.0 to 1.0
	float m_minThreshold{ 0.0f };
	// maximum amount the trigger needs to be pressed for the event to fire, from 0.0 to 1.0
	float m_maxThreshold{ 1.0f };
	Events::Side m_side{ Events::Side::Left };
};

TYPEDEF_BLUECLASS( GamePadTriggerInputEvent );
