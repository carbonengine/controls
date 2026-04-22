#pragma once
#include "../StdAfx.h"
#include "Events.h"

// Interface to match the state from a device
BLUE_INTERFACE( IInputEvent ) :
	public IRoot
{
public:
	// Checks the state and returns true/false if it matches
	virtual bool Match( const Events::State& state ) = 0;
	// Marks the parts of the state that matched this event as "owned" by this trigger, so that they won't be considered for identical events 
	virtual void Own( Events::State& state ) = 0; 
};
BLUE_DECLARE_INTERFACE( IInputEvent );
BLUE_DECLARE_IVECTOR( IInputEvent );