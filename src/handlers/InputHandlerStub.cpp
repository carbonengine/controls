#include "InputHandlerStub.h"

std::vector<DeviceEnums::DeviceIdentifier> InputHandlerStub::GetAllDeviceIdentifiers()
{
	return {};
}

void InputHandlerStub::RegisterForDeviceChange( std::function<void( std::vector<DeviceEnums::DeviceIdentifier> )> callback )
{
}

Events::State InputHandlerStub::Update( DeviceEnums::DeviceId deviceId )
{
	return {};
}
