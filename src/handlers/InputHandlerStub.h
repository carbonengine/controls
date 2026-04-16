#pragma once
#include "IInputHandler.h"

class InputHandlerStub : public IInputHandler
{
public:
	InputHandlerStub() = default;

	Events::State Update( DeviceEnums::DeviceId deviceId ) override;

	std::vector<DeviceEnums::DeviceIdentifier> GetAllDeviceIdentifiers() override;
	void RegisterForDeviceAdded( DEVICE_CHANGED_CALLBACK callback ) override;
	void RegisterForDeviceRemoved( DEVICE_CHANGED_CALLBACK callback ) override;

	void Rumble( DeviceEnums::DeviceId deviceId, Events::Rumble rumble ) override;
};