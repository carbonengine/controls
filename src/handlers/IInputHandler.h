#pragma once
#include "../StdAfx.h"
#include "../DeviceEnums.h"
#include "../events/IInputEvent.h"

// callback function when devices are added or removed
typedef std::function<void( DeviceEnums::DeviceIdentifier& )> DEVICE_CHANGED_CALLBACK;

class IInputHandler
{
public:
	virtual std::vector<DeviceEnums::DeviceIdentifier> GetAllDeviceIdentifiers() = 0;
	virtual void RegisterForDeviceAdded( DEVICE_CHANGED_CALLBACK callback ) = 0;
	virtual void RegisterForDeviceRemoved( DEVICE_CHANGED_CALLBACK callback ) = 0;
	virtual void SetDeviceActivation( DeviceEnums::DeviceId deviceId, bool activate ) = 0;
	virtual std::vector<Events::State> Update( DeviceEnums::DeviceId deviceId ) = 0;
	virtual void Rumble( DeviceEnums::DeviceId deviceId, Events::Rumble rumble ) = 0;
};
