#pragma once
#include "../StdAfx.h"
#include "../DeviceEnums.h"
#include "../events/IInputEvent.h"

// callback function when devices are added or removed
typedef std::function<void( DeviceEnums::DeviceIdentifier& )> DeviceChangedCallback;

class IInputHandler
{
public:
	virtual bool Initialize() = 0;
	virtual void RegisterForDeviceAdded( DeviceChangedCallback callback ) = 0;
	virtual void RegisterForDeviceRemoved( DeviceChangedCallback callback ) = 0;
	virtual void SetDeviceActivation( BlueSharedString deviceId, bool activate ) = 0;
	virtual std::vector<Events::State> Update( BlueSharedString deviceId ) = 0;
	virtual void Rumble( BlueSharedString deviceId, Events::Rumble rumble ) = 0;
};
