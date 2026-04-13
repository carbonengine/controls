#pragma once
#include "../StdAfx.h"
#include <BlueScriptCallback.h>

#include "IInputEvent.h"

BLUE_CLASS( InputEventTrigger ) : public IRoot
{
public:
	EXPOSE_TO_BLUE();
	InputEventTrigger( IRoot* lockobj = nullptr );
	void Process( Events::State state );
	size_t GetEventCount() const;

private:
	BlueScriptCallback m_callback;
	PIInputEventVector m_events;
};

TYPEDEF_BLUECLASS( InputEventTrigger );