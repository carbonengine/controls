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
	m_inputHandler->RegisterForDeviceChange( [this]( std::vector<DeviceEnums::DeviceIdentifier> deviceIdentifiers ) {
		OnDeviceChanged( std::move( deviceIdentifiers ) );
	} );
}

ControlManager::~ControlManager()
{
	m_inputHandler = nullptr;
}

void ControlManager::SetHoldTimeInMs( float holdTime )
{
	InputDevice::g_holdTimeInMs = holdTime;
}

float ControlManager::GetHoldTimeInMs()
{
	return InputDevice::g_holdTimeInMs;
}

void ControlManager::OnDeviceChanged( std::vector<DeviceEnums::DeviceIdentifier> deviceIdentifiers )
{
	// Check if an active device was removed
	for( auto& activeDevice : m_activeDevices )
	{
		auto foundDevice = std::find_if( deviceIdentifiers.begin(), deviceIdentifiers.end(), [activeDevice]( const DeviceEnums::DeviceIdentifier& identifier ) {
			return identifier.deviceID == activeDevice->GetDeviceID();
		} );
		if( foundDevice != deviceIdentifiers.end() )
		{
			CCP_LOGNOTICE( "Active device %ls (ID: %u) was disconnected", activeDevice->GetName().c_str(), activeDevice->GetDeviceID() );
			if( m_activeDeviceLostCallback )
			{
				m_activeDeviceLostCallback.CallVoid( activeDevice->GetDeviceID() );
			}
		}
	}

	m_changedDevices = deviceIdentifiers;
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
	if( !m_changedDevices.empty() )
	{
		ProcessChangedDevices();
	}

	for( auto& activeDevice : m_activeDevices )
	{
		activeDevice->Update( m_inputHandler );
	}
}

void ControlManager::ProcessChangedDevices()
{
	std::vector<InputDevicePtr> removedDevices;
	for( auto& device : m_devices )
	{
		auto foundDevice = std::find_if( m_changedDevices.begin(), m_changedDevices.end(), [device]( const DeviceEnums::DeviceIdentifier& identifier ) {
			return identifier.deviceID == device->GetDeviceID();
		} );
		if( foundDevice == m_changedDevices.end() )
		{
			removedDevices.push_back( device );
		}
	}

	for( auto& removedDevice : removedDevices )
	{
		auto indexInDevices = m_devices.FindKey( removedDevice->GetRawRoot() );
		if( indexInDevices != -1 )
		{
			m_devices.Remove( indexInDevices );
		}
		auto indexInActiveDevices = m_activeDevices.FindKey( removedDevice->GetRawRoot() );
		if( indexInActiveDevices != -1 )
		{
			m_activeDevices.Remove( indexInActiveDevices );
		}
	}

	// new devices
	for( auto& deviceIdentifier : m_changedDevices )
	{
		auto foundDevice = std::find_if( m_devices.begin(), m_devices.end(), [deviceIdentifier]( InputDevicePtr identifier ) {
			return identifier->GetDeviceID() == deviceIdentifier.deviceID;
		} );
		if( foundDevice == m_devices.end() )
		{
			InputDevicePtr newDevice;
			newDevice.CreateInstance();
			newDevice->SetIdentifier( deviceIdentifier );
			m_devices.Append( newDevice->GetRawRoot() );
		}
	}

	if( m_devicesChangedCallback && m_initialDeviceListReceived )
	{
		m_devicesChangedCallback.CallVoid();
	}
	
	m_changedDevices.clear();
	m_initialDeviceListReceived = true;
}