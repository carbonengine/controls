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
	for( const auto& identifier : m_inputHandler->GetAllDeviceIdentifiers() )
	{
		InputDeviceIdentifierPtr deviceIdentifier;
		deviceIdentifier.CreateInstance();
		deviceIdentifier->SetData( identifier );
		m_deviceIdentifiers.Append(deviceIdentifier);
	}
}

ControlManager::~ControlManager()
{
	m_inputHandler = nullptr;
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
