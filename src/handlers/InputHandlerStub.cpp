#include "InputHandlerStub.h"

std::vector<DeviceEnums::DeviceIdentifier> InputHandlerStub::GetAllDeviceIdentifiers()
{
	return {};
}

void InputHandlerStub::RegisterForDeviceAdded( DEVICE_CHANGED_CALLBACK callback )
{
}

void InputHandlerStub::RegisterForDeviceRemoved( DEVICE_CHANGED_CALLBACK callback )
{
}

Events::State InputHandlerStub::Update( DeviceEnums::DeviceId deviceId )
{
	return {};
}

void InputHandlerStub::Rumble( DeviceEnums::DeviceId deviceId, Events::Rumble rumble )
{
}