#include "ControlManager.h"

#include "handlers/MockInputHandler.h"

#if defined( WIN32 )
#include "handlers/InputHandlerWin.h"
#elif defined( __APPLE__ )
#include "handlers/InputHandlerApple.h"
#else
#include "handlers/InputHandlerStub.h"
#endif


ControlManager::ControlManager( IRoot* lockobj ) :
	PARENTLOCK( m_devices ),
	PARENTLOCK( m_activeDevices ),
	m_inputHandler( nullptr )
{
}

bool ControlManager::Initialize()
{
	if( !m_inputHandler )
	{
#if defined( WIN32 )
		m_inputHandler.reset( new InputHandlerWin() );
#elif defined( __APPLE__ )
		m_inputHandler.reset( new InputHandlerApple() );
#else
		m_inputHandler.reset( new InputHandlerStub() );
#endif
	}

	m_inputHandler->RegisterForDeviceAdded( [this]( DeviceEnums::DeviceIdentifier& deviceIdentifiers ) {
		OnDeviceAdded( deviceIdentifiers );
	} );
	m_inputHandler->RegisterForDeviceRemoved( [this]( DeviceEnums::DeviceIdentifier& deviceIdentifiers ) {
		OnDeviceRemoved( deviceIdentifiers );
	} );
	return m_inputHandler->Initialize();
}

void ControlManager::SetHoldTimeInMs( uint64_t holdTime )
{
	Events::g_holdTimeInMicroSeconds = holdTime * 1000;
}

uint64_t ControlManager::GetHoldTimeInMs()
{
	return Events::g_holdTimeInMicroSeconds / 1000;
}

void ControlManager::OnDeviceAdded( DeviceEnums::DeviceIdentifier& deviceIdentifier )
{
	std::lock_guard<std::mutex> lock( m_deviceChangedMutex );
	m_addedDevices.push_back( deviceIdentifier );
}

void ControlManager::OnDeviceRemoved( DeviceEnums::DeviceIdentifier& deviceIdentifier )
{
	std::lock_guard<std::mutex> lock( m_deviceChangedMutex );
	m_removedDevices.push_back( deviceIdentifier );
}

IRootPtr ControlManager::Activate( BlueSharedString deviceID )
{
	auto foundDevice = FindDevice( deviceID );

	if( foundDevice )
	{
		auto foundActiveDevice = FindActiveDevice( deviceID );
		if( foundActiveDevice )
		{
			CCP_LOGWARN( "Device %s (ID: %s) is already active returning the existing instance", foundDevice->GetName().c_str(), foundDevice->GetDeviceID().c_str() );
			return foundActiveDevice->GetRawRoot();
		}

		m_activeDevices.Append( foundDevice->GetRawRoot() );
		CCP_LOGNOTICE( "Device %s (ID: %s) is active", foundDevice->GetName().c_str(), foundDevice->GetDeviceID().c_str() );
		m_inputHandler->SetDeviceActivation( deviceID, true );

		return foundDevice->GetRawRoot();
	}

	CCP_LOGERR( "Device with ID: %s is not connected", deviceID.c_str() );
	return nullptr;
}

void ControlManager::Deactivate( BlueSharedString deviceID )
{
	auto foundDevice = FindActiveDevice( deviceID );
	if( !foundDevice )
	{
		CCP_LOGWARN( "ControlManager::Deactivate called for an inactive device ID '%s'; ignoring", deviceID.c_str() );
		return;
	}

	foundDevice->ResetRumble();
	m_activeDevices.Remove( m_activeDevices.FindKey( foundDevice->GetRawRoot() ) );
	m_inputHandler->SetDeviceActivation( deviceID, false );
	CCP_LOGNOTICE( "Device %s (ID: %s) is no longer active", foundDevice->GetName().c_str(), foundDevice->GetDeviceID().c_str() );
}

void ControlManager::Update()
{
	if( !m_addedDevices.empty() || !m_removedDevices.empty() )
	{
		ProcessChangedDevices();
	}

	m_initialDevicesProcessed = true;

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
		auto deviceID = removedDeviceIdentifier.deviceID;
		auto foundDevice = FindDevice( deviceID );
		auto root = foundDevice ? foundDevice->GetRawRoot() : nullptr;

		if( root )
		{
			auto indexInActiveDevices = m_activeDevices.FindKey( root );
			auto indexInDevices = m_devices.FindKey( root );
			// only call device removed if the device is not active
			if( m_deviceRemovedCallback && indexInActiveDevices == -1 && indexInDevices != -1 )
			{
				m_deviceRemovedCallback.CallVoid( deviceID );
			}
			else if( m_activeDeviceLostCallback && indexInActiveDevices != -1 )
			{
				m_activeDeviceLostCallback.CallVoid( deviceID );
			}

			// and now remove the devices
			if( indexInDevices != -1 )
			{
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
		auto foundDevice = FindDevice( deviceIdentifier.deviceID );
		if( !foundDevice )
		{
			// add the device first before we call the callback so we have access to the device in the callback
			InputDevicePtr newDevice;
			newDevice.CreateInstance();
			newDevice->SetIdentifier( deviceIdentifier );
			m_devices.Append( newDevice->GetRawRoot() );

			if( m_deviceAddedCallback && m_initialDevicesProcessed )
			{
				m_deviceAddedCallback.CallVoid( newDevice->GetDeviceID() );
			}
		}
	}
}

InputDevicePtr ControlManager::FindDevice( BlueSharedString deviceID ) const
{
	auto foundDevice = std::find_if( m_devices.begin(), m_devices.end(), [deviceID]( InputDevicePtr identifier ) {
		return identifier->GetDeviceID() == deviceID;
	} );
	return foundDevice != m_devices.end() ? *foundDevice : nullptr;
}

InputDevicePtr ControlManager::FindActiveDevice( BlueSharedString deviceID ) const
{
	auto foundDevice = std::find_if( m_activeDevices.begin(), m_activeDevices.end(), [deviceID]( InputDevicePtr identifier ) {
		return identifier->GetDeviceID() == deviceID;
	} );
	return foundDevice != m_activeDevices.end() ? *foundDevice : nullptr;
}

void ControlManager::SetBackgroundEventsEnabled( bool enabled )
{
	m_inputHandler->SetBackgroundEventsEnabled( enabled );
}

// Helper functions for unit testing
namespace
{
bool GetMockKey( const InputElement* element, DeviceEnums::ElementKey& key )
{
	if( !element )
	{
		CCP_LOGERR( "ControlManager: No input element given" );
		return false;
	}
	key = element->GetKey();
	return true;
}
}

void ControlManager::EnableMockInputHandler()
{
	m_inputHandler.reset( new MockInputHandler() );
	// reinitialize the mock input handler to ensure it is ready for use
	Initialize();
}

bool ControlManager::MockAddDevice( BlueSharedString deviceID, BlueSharedString name )
{
	using Descriptor = DeviceEnums::InputElementDescriptor;

	DeviceEnums::DeviceIdentifier identifier;
	identifier.deviceID = deviceID;
	identifier.name = name;
	identifier.vendorID = BlueSharedString( "mock" );
	identifier.productID = BlueSharedString( "mock" );
	identifier.family = DeviceEnums::DeviceFamily::Generic;

	for( auto descriptor : { Descriptor::FaceSouth, Descriptor::FaceEast, Descriptor::FaceWest, Descriptor::FaceNorth,
							 Descriptor::LeftShoulder, Descriptor::LeftTriggerButton, Descriptor::LeftStickButton,
							 Descriptor::RightShoulder, Descriptor::RightTriggerButton, Descriptor::RightStickButton,
							 Descriptor::Start, Descriptor::Select, Descriptor::Guide,
							 Descriptor::DPadUp, Descriptor::DPadDown, Descriptor::DPadLeft, Descriptor::DPadRight } )
	{
		identifier.buttonElements.push_back( { descriptor, 0 } );
	}
	for( auto descriptor : { Descriptor::LeftStickX, Descriptor::LeftStickY, Descriptor::RightStickX, Descriptor::RightStickY,
							 Descriptor::LeftTriggerAxis, Descriptor::RightTriggerAxis } )
	{
		identifier.axisElements.push_back( { descriptor, 0 } );
	}
	identifier.switchElements.push_back( { Descriptor::DPad, 0 } );

	return static_cast<MockInputHandler*>( m_inputHandler.get() )->AddDevice( identifier );
}

bool ControlManager::MockRemoveDevice( BlueSharedString deviceID )
{
	return static_cast<MockInputHandler*>( m_inputHandler.get() )->RemoveDevice( deviceID );
}

bool ControlManager::MockSetButton( BlueSharedString deviceID, const InputElement* element, bool pressed )
{
	DeviceEnums::ElementKey key;
	return GetMockKey( element, key ) && static_cast<MockInputHandler*>( m_inputHandler.get() )->SetButton( deviceID, key, pressed );
}

bool ControlManager::MockSetAxis( BlueSharedString deviceID, const InputElement* element, float value )
{
	DeviceEnums::ElementKey key;
	return GetMockKey( element, key ) && static_cast<MockInputHandler*>( m_inputHandler.get() )->SetAxis( deviceID, key, value );
}

bool ControlManager::MockSetSwitch( BlueSharedString deviceID, const InputElement* element, uint32_t position )
{
	if( position >= static_cast<uint32_t>( Events::SwitchPosition::NonCenter ) )
	{
		CCP_LOGERR( "ControlManager::MockSetSwitch: Invalid switch position %u", position );
		return false;
	}
	DeviceEnums::ElementKey key;
	return GetMockKey( element, key ) && static_cast<MockInputHandler*>( m_inputHandler.get() )->SetSwitch( deviceID, key, static_cast<Events::SwitchPosition>( position ) );
}

void ControlManager::MockAdvanceTimeMs( uint64_t milliseconds )
{
	static_cast<MockInputHandler*>( m_inputHandler.get() )->AdvanceTime( milliseconds * 1000 );
}

bool ControlManager::MockIsDeviceActive( BlueSharedString deviceID ) const
{
	return static_cast<const MockInputHandler*>( m_inputHandler.get() )->IsDeviceActive( deviceID );
}

