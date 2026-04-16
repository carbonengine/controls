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
	IRootPtr Activate( DeviceEnums::DeviceId deviceID );
	void Update();

	// deactivates an input device
	void Deactivate( DeviceEnums::DeviceId deviceID );

private:
	void SetHoldTimeInMs( float holdTime );
	float GetHoldTimeInMs();
	void OnDeviceChanged( std::vector<DeviceEnums::DeviceIdentifier> deviceIdentifiers );
	void ProcessChangedDevices();

	PInputDeviceVector m_devices;
	IInputHandler* m_inputHandler;
	PInputDeviceVector m_activeDevices;
	BlueScriptCallback m_activeDeviceLostCallback;
	BlueScriptCallback m_devicesChangedCallback;
	std::vector<DeviceEnums::DeviceIdentifier> m_changedDevices;
	// We use this flag to ignore the initial device list callback from the input handler, since we will be populating the device list ourselves on initialization
	bool m_initialDeviceListReceived = false;
};

TYPEDEF_BLUECLASS( ControlManager );