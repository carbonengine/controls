#pragma once
#include "../StdAfx.h"
#include "../DeviceEnums.h"
#include "../events/IInputEvent.h"

// callback function when devices are added or removed
// the parameter is the list of currently connected devices
typedef std::function<void( std::vector<DeviceEnums::DeviceIdentifier> )> DEVICE_CHANGED_CALLBACK;

class IInputHandler
{
public:
	virtual std::vector<DeviceEnums::DeviceIdentifier> GetAllDeviceIdentifiers() = 0;
	virtual void RegisterForDeviceChange( DEVICE_CHANGED_CALLBACK callback ) = 0;
	virtual Events::State Update( DeviceEnums::DeviceId deviceId ) = 0;
	virtual void Rumble( DeviceEnums::DeviceId deviceId, Events::Rumble rumble ) = 0;
};
