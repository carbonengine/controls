#pragma once
#include "StdAfx.h"
#include "DeviceEnums.h"
#include "events/InputEventTrigger.h"
#include <string>
#include "handlers/IInputHandler.h"

BLUE_DECLARE_VECTOR( InputEventTrigger );

BLUE_CLASS( InputDevice ) : public IRoot
{
public:
	EXPOSE_TO_BLUE();
	InputDevice( IRoot* lockobj = nullptr );

	void SetIdentifier( DeviceEnums::DeviceIdentifier identifier );
	void Update( IInputHandler* inputHandler );
	BlueSharedString GetStateAsJson() const;
	uint32_t GetDeviceID() const;
	BlueSharedStringW GetName() const;
	
	static float g_holdTimeInMs; // The time in milliseconds after which a button state changes from Pressed to Held

private:
	DeviceEnums::DeviceIdentifier m_deviceIdentifier {};
	PInputEventTriggerVector m_triggers;
	Events::State m_currentState;
};

TYPEDEF_BLUECLASS( InputDevice );
