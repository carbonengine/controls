#pragma once
#include "IInputHandler.h"

class InputHandlerStub : public IInputHandler
{
public:
	InputHandlerStub() = default;

	Events::State Update( DeviceEnums::DeviceId deviceId ) override;

	std::vector<DeviceEnums::DeviceIdentifier> GetAllDeviceIdentifiers() override;
	void RegisterForDeviceChange( std::function<void( std::vector<DeviceEnums::DeviceIdentifier> )> callback ) override;
};