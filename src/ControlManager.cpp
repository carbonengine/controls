#include "ControlManager.h"

#ifdef WIN32
#include "handlers/InputHandlerWin.h"
#else
#include "handlers/InputHandlerStub.h"
#endif


ControlManager::ControlManager( IRoot* lockobj ) :
	PARENTLOCK( m_devices ),
	PARENTLOCK( m_activeDevices ),
#ifdef WIN32
	m_inputHandler( new InputHandlerWin() )
#else
	m_inputHandler( new InputHandlerStub() )
#endif
{
	m_inputHandler->RegisterForDeviceAdded( [this]( DeviceEnums::DeviceIdentifier& deviceIdentifiers ) {
		OnDeviceAdded( deviceIdentifiers );
	} );
	m_inputHandler->RegisterForDeviceRemoved( [this]( DeviceEnums::DeviceIdentifier& deviceIdentifiers ) {
		OnDeviceRemoved( deviceIdentifiers );
	} );
}

void ControlManager::SetHoldTimeInMs( float holdTime )
{
	InputDevice::g_holdTimeInMs = holdTime;
}

float ControlManager::GetHoldTimeInMs()
{
	return InputDevice::g_holdTimeInMs;
}

void ControlManager::OnDeviceAdded( DeviceEnums::DeviceIdentifier& deviceIdentifier )
{
	std::lock_guard<std::mutex> lock( m_deviceChangedMutex );
	m_addedDevices.push_back( deviceIdentifier );
}

void ControlManager::OnDeviceRemoved( DeviceEnums::DeviceIdentifier & deviceIdentifier )
{
	std::lock_guard<std::mutex> lock( m_deviceChangedMutex );
	m_removedDevices.push_back( deviceIdentifier );
}

IRootPtr ControlManager::Activate( DeviceEnums::DeviceId deviceID )
{
	auto foundDevice = std::find_if( m_devices.begin(), m_devices.end(), [deviceID]( InputDevicePtr identifier ) {
		return identifier->GetDeviceID() == deviceID;
	} );

	if( foundDevice != m_devices.end() )
	{
		auto foundActiveDevice = std::find_if( m_activeDevices.begin(), m_activeDevices.end(), [deviceID]( InputDevicePtr identifier ) {
			return identifier->GetDeviceID() == deviceID;
		} );
		if( foundActiveDevice != m_activeDevices.end() )
		{
			CCP_LOGERR( "Device %ls (ID: %u) is already active returning the existing instance", ( *foundDevice )->GetName().c_str(), ( *foundDevice )->GetDeviceID() );
			return (*foundActiveDevice)->GetRawRoot();
		}

		m_activeDevices.Append( (*foundDevice)->GetRawRoot() );
		CCP_LOGNOTICE( "Device %ls (ID: %u) is active", ( *foundDevice )->GetName().c_str(), ( *foundDevice )->GetDeviceID() );
		return (*foundDevice)->GetRawRoot();
	}
	else
	{
		CCP_LOGERR( "Device with ID: %u is not connected", deviceID );
	}
	return nullptr;
}

void ControlManager::Deactivate( DeviceEnums::DeviceId deviceID )
{
	auto foundDevice = std::find_if( m_activeDevices.begin(), m_activeDevices.end(), [deviceID]( InputDevicePtr identifier ) {
		return identifier->GetDeviceID() == deviceID;
	} );
	if( foundDevice != m_activeDevices.end() )
	{
		m_activeDevices.Remove( m_activeDevices.FindKey( (*foundDevice)->GetRawRoot() ) );
		CCP_LOGNOTICE( "Device %ls (ID: %u) is no longer active", ( *foundDevice )->GetName().c_str(), ( *foundDevice )->GetDeviceID() );
	}
	else
	{
		CCP_LOGNOTICE( "ControlManager::Deactivate called with a device id that is not connected, ignoring" );
	}
}

void ControlManager::Update()
{
	if( !m_addedDevices.empty() || !m_removedDevices.empty() )
	{
		ProcessChangedDevices();
	}

	for( auto& activeDevice : m_activeDevices )
	{
		activeDevice->Update( m_inputHandler.get() );
	}
}

void ControlManager::ProcessChangedDevices()
{
	std::vector<DeviceEnums::DeviceIdentifier> added;
	std::vector<DeviceEnums::DeviceIdentifier> removed;
	{
		std::lock_guard<std::mutex> lock( m_deviceChangedMutex );
		added.swap( m_addedDevices );
		removed.swap( m_removedDevices );
	}

	for( auto& removedDeviceIdentifier : removed )
	{
		auto foundDevice = std::find_if( m_devices.begin(), m_devices.end(), [removedDeviceIdentifier]( InputDevicePtr identifier ) {
			return identifier->GetDeviceID() == removedDeviceIdentifier.deviceID;
		} );
		auto root = foundDevice != m_devices.end() ? ( *foundDevice )->GetRawRoot() : nullptr;

		if( root )
		{
			auto deviceID = ( *foundDevice )->GetDeviceID();
			auto indexInActiveDevices = m_activeDevices.FindKey( root );
			auto indexInDevices = m_devices.FindKey( root );
			// call callbacks first so they still have access to the whole device list (so we can see the name of devices)
			if( m_deviceAddedCallback && indexInActiveDevices == -1 )
			{
				m_deviceAddedCallback.CallVoid( deviceID );
			}
			else if( m_activeDeviceLostCallback && indexInActiveDevices != -1 )
			{
				m_activeDeviceLostCallback.CallVoid( deviceID );
			}

			// and now remove the devices
			if( indexInDevices != -1 )
			{
				// need to remove from ALL devices first, so the callbacks when active device lost has a correct devices list
				m_devices.Remove( indexInDevices );
			}
			if( indexInActiveDevices != -1 )
			{
				m_activeDevices.Remove( indexInActiveDevices );
			}
		}
	}

	// new devices
	for( auto& deviceIdentifier : added )
	{
		auto foundDevice = std::find_if( m_devices.begin(), m_devices.end(), [deviceIdentifier]( InputDevicePtr identifier ) {
			return identifier->GetDeviceID() == deviceIdentifier.deviceID;
		} );
		if( foundDevice == m_devices.end() )
		{
			// add the device first before we call the callback so we have access to the device in the callback
			InputDevicePtr newDevice;
			newDevice.CreateInstance();
			newDevice->SetIdentifier( deviceIdentifier );
			m_devices.Append( newDevice->GetRawRoot() );

			if( m_deviceAddedCallback )
			{
				m_deviceAddedCallback.CallVoid( newDevice->GetDeviceID() );
			}
		}
	}
}