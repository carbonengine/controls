#pragma once
#include "../StdAfx.h"
#include "../DeviceEnums.h"
#include "../events/IInputEvent.h"

class IInputHandler
{
public:
	virtual Events::State Update( DeviceEnums::DeviceId deviceID ) = 0;
	virtual std::vector<DeviceEnums::DeviceIdentifier> GetAllDeviceIdentifiers() = 0;
	virtual void RegisterForDeviceChange( std::function<void( std::vector<DeviceEnums::DeviceIdentifier> )> callback ) = 0;
};
