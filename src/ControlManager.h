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

	// activates an input device
	void Activate( DeviceEnums::DeviceId deviceID );

	// deactivates an input device
	void Deactivate( DeviceEnums::DeviceId deviceID );

	// Updates the state of the active device. Should be called once per frame.
	void Update();

private:
	void SetHoldTimeInMs( float holdTime );
	float GetHoldTimeInMs();

	PInputDeviceVector m_devices;
	IInputHandler* m_inputHandler;
	PInputDeviceVector m_activeDevices;
	DeviceEnums::DeviceId m_activeDeviceID = 0;
	BlueScriptCallback m_activeDeviceLostCallback;
	BlueScriptCallback m_deviceConnectedCallback;
};

TYPEDEF_BLUECLASS( ControlManager );