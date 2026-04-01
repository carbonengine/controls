#include "ControlManager.h"

#ifdef WIN32
#include "handlers/InputHandlerWin.h"
#else
#include "handlers/InputHandlerStub.h"
#endif


ControlManager::ControlManager( IRoot* lockobj ) :
	PARENTLOCK( m_deviceIdentifiers ),
#ifdef WIN32
	m_inputHandler( new InputHandlerWin() )
#else
	m_inputHandler( new InputHandlerStub() )
#endif
{
	m_inputHandler->RegisterForDeviceChange( [this]( std::vector<DeviceEnums::DeviceIdentifier> deviceIdentifiers )
	{
		UpdateDeviceList( deviceIdentifiers );
		if( m_activeDevice && std::none_of( deviceIdentifiers.begin(), deviceIdentifiers.end(), [this]( const DeviceEnums::DeviceIdentifier& identifier )
		{
			return identifier.deviceID == m_activeDeviceID;
		} ) )
		{
			CCP_LOGNOTICE( "Active device (ID: %u) was disconnected", m_activeDeviceID );
			m_activeDevice = nullptr;
			m_activeDeviceID = 0;
			if( m_activeDeviceLostCallback )
			{
				m_activeDeviceLostCallback.CallVoid();
			}
		}
		else
		{
			if( m_deviceConnectedCallback )
			{
				m_deviceConnectedCallback.CallVoid();
			}
		}
	} );
}

ControlManager::~ControlManager()
{
	m_inputHandler = nullptr;
}

void ControlManager::UpdateDeviceList( std::vector<DeviceEnums::DeviceIdentifier> deviceIdentifiers )
{
	m_deviceIdentifiers.Remove(-1);
	for( const auto& identifier : deviceIdentifiers )
	{
		InputDeviceIdentifierPtr id;
		id.CreateInstance();
		id->SetData( identifier );
		m_deviceIdentifiers.Append( id );
	}
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
	if( !m_activeDevice )
	{
		CCP_LOGERR( "ControlManager::Update - No active device connected" );
		return;
	}
	auto newState = m_inputHandler->Update( m_activeDeviceID );
	m_activeDevice->ProcessTriggers( newState );
}

void ControlManager::Connect( DeviceEnums::DeviceId deviceID )
{
	auto foundDevice = std::find_if( m_deviceIdentifiers.begin(), m_deviceIdentifiers.end(), [deviceID]( InputDeviceIdentifierPtr identifier )
	{
		return identifier->GetDeviceID() == deviceID;
	} );
	m_activeDevice = nullptr;
	m_activeDevice.CreateInstance();

	if( foundDevice != m_deviceIdentifiers.end() )
	{
		m_activeDeviceID = deviceID;
		m_activeDevice->SetIdentifier( *foundDevice );
	}
}

void ControlManager::Disconnect()
{
	if( !m_activeDevice )
	{
		CCP_LOGNOTICE( "ControlManager::Disconnect called while not connected, ignoring" );
		return;
	}
	m_activeDevice = nullptr;
	m_activeDeviceID = 0;
}
