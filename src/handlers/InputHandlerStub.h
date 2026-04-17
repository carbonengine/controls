#pragma once
#include "IInputHandler.h"

class InputHandlerStub : public IInputHandler
{
public:
	InputHandlerStub() = default;
	bool Initialize() override;

	void SetDeviceActivation( DeviceEnums::DeviceId deviceId, bool activate ) override;
	std::vector<Events::State> Update( DeviceEnums::DeviceId deviceId ) override;

	void RegisterForDeviceAdded( DEVICE_CHANGED_CALLBACK callback ) override;
	void RegisterForDeviceRemoved( DEVICE_CHANGED_CALLBACK callback ) override;

	void Rumble( DeviceEnums::DeviceId deviceId, Events::Rumble rumble ) override;
};