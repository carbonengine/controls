#pragma once
#include "IInputHandler.h"

class InputHandlerStub : public IInputHandler
{
public:
	InputHandlerStub() = default;
	bool Initialize() override;

	void SetDeviceActivation( BlueSharedString deviceId, bool activate ) override;
	std::vector<Events::State> Update( BlueSharedString deviceId ) override;

	void RegisterForDeviceAdded( DeviceChangedCallback callback ) override;
	void RegisterForDeviceRemoved( DeviceChangedCallback callback ) override;

	void Rumble( BlueSharedString deviceId, Events::Rumble rumble ) override;
};