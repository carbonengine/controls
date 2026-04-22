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

	// activates an input device
	IRootPtr Activate( BlueSharedString deviceID );
	void Update();

	// deactivates an input device
	void Deactivate( BlueSharedString deviceID );

private:
	void SetHoldTimeInMs( uint64_t holdTime );
	uint64_t GetHoldTimeInMs();
	void OnDeviceAdded( DeviceEnums::DeviceIdentifier& deviceIdentifier );
	void OnDeviceRemoved( DeviceEnums::DeviceIdentifier& deviceIdentifier );
	void ProcessChangedDevices();
	InputDevicePtr FindDevice( BlueSharedString deviceID ) const;
	InputDevicePtr FindActiveDevice( BlueSharedString deviceID ) const;

	PInputDeviceVector m_devices;
	std::unique_ptr<IInputHandler> m_inputHandler;
	PInputDeviceVector m_activeDevices;
	BlueScriptCallback m_activeDeviceLostCallback;
	BlueScriptCallback m_deviceAddedCallback;
	BlueScriptCallback m_deviceRemovedCallback;
	std::mutex m_deviceChangedMutex;

	std::vector<DeviceEnums::DeviceIdentifier> m_addedDevices;
	std::vector<DeviceEnums::DeviceIdentifier> m_removedDevices;

	bool m_initialDevicesProcessed = false;
};

TYPEDEF_BLUECLASS( ControlManager );