#include "InputHandlerStub.h"

bool InputHandlerStub::Initialize()
{
	return true;
}

void InputHandlerStub::RegisterForDeviceAdded( DeviceChangedCallback callback )
{
}

void InputHandlerStub::RegisterForDeviceRemoved( DeviceChangedCallback callback )
{
}

std::vector<Events::State> InputHandlerStub::Update( BlueSharedString deviceId )
{
	return {};
}

void InputHandlerStub::SetDeviceActivation( BlueSharedString deviceId, bool activate )
{
}

void InputHandlerStub::Rumble( BlueSharedString deviceId, Events::Rumble rumble )
{
}

void InputHandlerStub::SetBackgroundEventsEnabled( bool enabled )
{
}
