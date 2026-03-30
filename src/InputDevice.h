#pragma once
#include "StdAfx.h"
#include "DeviceEnums.h"
#include "events/InputEventTrigger.h"
#include <string>

BLUE_DECLARE_VECTOR( InputEventTrigger );

BLUE_CLASS( InputDeviceIdentifier ) : public IRoot
{
public:
	EXPOSE_TO_BLUE();
	InputDeviceIdentifier( IRoot* lockobj = nullptr );
	void SetData( const DeviceEnums::DeviceIdentifier& identifier );
	uint32_t GetDeviceID() const;

private:
	DeviceEnums::DeviceIdentifier identifier;
};

TYPEDEF_BLUECLASS( InputDeviceIdentifier );

BLUE_CLASS( InputDevice ) : public IRoot
{
public:
	EXPOSE_TO_BLUE();
	InputDevice( IRoot* lockobj = nullptr );

	void SetIdentifier( InputDeviceIdentifierPtr identifier );
	void ProcessTriggers( Events::State state );
	BlueSharedString GetStateAsJson( ) const;

private:
	InputDeviceIdentifierPtr m_deviceIdentifier;
	PInputEventTriggerVector m_triggers;
	Events::State m_currentState;
};

TYPEDEF_BLUECLASS( InputDevice );
