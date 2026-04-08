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
	m_inputHandler->RegisterForDeviceChange( [this]( std::vector<DeviceEnums::DeviceIdentifier> deviceIdentifiers )
	{
		// remove all the active devices that are no longer connected
		std::vector<InputDevicePtr> removedDevices;
		for( auto& activeDevice : m_activeDevices )
		{
			auto foundDevice = std::find_if( deviceIdentifiers.begin(), deviceIdentifiers.end(), [activeDevice]( const DeviceEnums::DeviceIdentifier& identifier )
			{
				return identifier.deviceID == activeDevice->GetDeviceID();
			} );
			if( foundDevice == deviceIdentifiers.end() )
			{
				removedDevices.push_back( activeDevice );
				CCP_LOGNOTICE( "Active device %ls (ID: %u) was disconnected", activeDevice->GetName().c_str(), activeDevice->GetDeviceID() );
				if( m_activeDeviceLostCallback )
				{
					m_activeDeviceLostCallback.CallVoid();
				}
			}
		}

		for( auto& removedDevice : removedDevices )
		{
			m_activeDevices.Remove( m_activeDevices.FindKey( removedDevice ) );
		}

		// update the list of all devices
		// removed devices
		removedDevices.clear();
		
		for( auto& device : m_devices )
		{
			auto foundDevice = std::find_if( deviceIdentifiers.begin(), deviceIdentifiers.end(), [device]( const DeviceEnums::DeviceIdentifier& identifier ) {
				return identifier.deviceID == device->GetDeviceID();
			} );
			if( foundDevice == deviceIdentifiers.end() )
			{
				removedDevices.push_back( device );
			}
		}

		for( auto& removedDevice : removedDevices )
		{
			m_devices.Remove( m_devices.FindKey( removedDevice ) );
		}

		// new devices
		for( auto& deviceIdentifier : deviceIdentifiers )
		{
			auto foundDevice = std::find_if( m_devices.begin(), m_devices.end(), [deviceIdentifier]( InputDevicePtr identifier ) {
				return identifier->GetDeviceID() == deviceIdentifier.deviceID;
			} );
			if( foundDevice == m_devices.end() )
			{
				InputDevicePtr newDevice;
				newDevice.CreateInstance();
				newDevice->SetIdentifier( deviceIdentifier );
				m_devices.Append( newDevice );
				if( m_deviceConnectedCallback )
				{
					m_deviceConnectedCallback.CallVoid();
				}
			}
		}
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

void ControlManager::Update()
{
	for( auto& activeDevice : m_activeDevices )
	{
		activeDevice->Update( m_inputHandler );
	}
}

void ControlManager::Activate( DeviceEnums::DeviceId deviceID )
{
	auto foundDevice = std::find_if( m_devices.begin(), m_devices.end(), [deviceID]( InputDevicePtr identifier )
	{
		return identifier->GetDeviceID() == deviceID;
	} );
	
	if( foundDevice != m_devices.end() )
	{
		m_activeDevices.Append( *foundDevice );
		CCP_LOGNOTICE( "Device %ls (ID: %u) is active", (*foundDevice)->GetName().c_str(), (*foundDevice)->GetDeviceID() );
	}
}

void ControlManager::Deactivate( DeviceEnums::DeviceId deviceID )
{
	auto foundDevice = std::find_if( m_devices.begin(), m_devices.end(), [deviceID]( InputDevicePtr identifier ) {
		return identifier->GetDeviceID() == deviceID;
	} );
	if( foundDevice != m_devices.end() )
	{
		m_activeDevices.Remove( m_activeDevices.FindKey( *foundDevice ) );
		CCP_LOGNOTICE( "Device %ls (ID: %u) is no longer active", ( *foundDevice )->GetName().c_str(), ( *foundDevice )->GetDeviceID() );
	}
	else
	{
		CCP_LOGNOTICE( "ControlManager::Deactivate called with a device id that is not connected, ignoring" );
	}
}
