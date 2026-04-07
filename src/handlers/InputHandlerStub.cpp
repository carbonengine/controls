#include "InputHandlerStub.h"

std::vector<DeviceEnums::DeviceIdentifier> InputHandlerStub::GetAllDeviceIdentifiers()
{
	return {};
}

Events::State InputHandlerStub::Update( DeviceEnums::DeviceId deviceID )
{
	return {};
}

void InputHandlerStub::RegisterForDeviceChange( std::function<void( std::vector<DeviceEnums::DeviceIdentifier> )> callback )
{
}