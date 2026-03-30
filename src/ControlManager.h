#pragma once
#include "StdAfx.h"
#include "handlers/IInputHandler.h"
#include "InputDevice.h"
#include "DeviceEnums.h"

BLUE_DECLARE( InputDevice );
BLUE_DECLARE_VECTOR( InputDevice );
BLUE_DECLARE( InputDeviceIdentifier );
BLUE_DECLARE_VECTOR( InputDeviceIdentifier );

BLUE_CLASS( ControlManager ) : public IRoot
{
public:
	EXPOSE_TO_BLUE();

	ControlManager( IRoot* lockobj = nullptr );
	~ControlManager( );

	// connects to an input device
	// returns true if the connection was successful, false if it failed (e.g. invalid device ID)
	void Connect( DeviceEnums::DeviceId deviceID );

	// disconnects the active device, if any
	void Disconnect();

	// Updates the state of the active device. Should be called once per frame.
	void Update();
private:

	IInputHandler* m_inputHandler;
	InputDevicePtr m_activeDevice;
	PInputDeviceIdentifierVector m_deviceIdentifiers;
	DeviceEnums::DeviceId m_activeDeviceID = 0;
	BlueScriptCallback m_activeDeviceLostCallback;
	BlueScriptCallback m_deviceConnectedCallback;
};

TYPEDEF_BLUECLASS( ControlManager );