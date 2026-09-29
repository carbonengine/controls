#include "MockInputHandler.h"

MockInputHandler::MockInputHandler() :
	m_time( Events::GetTimestamp() )
{
}

MockInputHandler::~MockInputHandler() = default;

bool MockInputHandler::Initialize()
{
	return true;
}

void MockInputHandler::RegisterForDeviceAdded( DeviceChangedCallback callback )
{
	m_deviceAdded = std::move( callback );
}

void MockInputHandler::RegisterForDeviceRemoved( DeviceChangedCallback callback )
{
	m_deviceRemoved = std::move( callback );
}

void MockInputHandler::SetDeviceActivation( BlueSharedString deviceId, bool activate )
{
	if( auto* device = FindDevice( deviceId ) )
	{
		device->active = activate;
		device->pending.clear();
	}
}

std::vector<Events::State> MockInputHandler::Update( BlueSharedString deviceId )
{
	auto* device = FindDevice( deviceId );
	if( !device || !device->active )
	{
		return {};
	}

	std::vector<Events::State> states;
	states.swap( device->pending );
	if( states.empty() )
	{
		// Always report a state so InputDevice never falls back to the real clock, which would
		// desync from the mock clock and make hold-time based events non-deterministic.
		device->current.timestamp = m_time;
		states.push_back( device->current );
	}
	return states;
}

void MockInputHandler::Rumble( BlueSharedString deviceId, Events::Rumble rumble )
{
	if( auto* device = FindDevice( deviceId ) )
	{
		device->rumble = rumble;
	}
}

void MockInputHandler::SetBackgroundEventsEnabled( bool enabled )
{
	m_backgroundEventsEnabled = enabled;
}

bool MockInputHandler::AddDevice( const DeviceEnums::DeviceIdentifier& identifier )
{
	std::string id = identifier.deviceID.c_str();
	if( m_devices.count( id ) )
	{
		CCP_LOGERR( "MockInputHandler::AddDevice: Device %s is already connected", id.c_str() );
		return false;
	}

	MockDevice device;
	device.identifier = identifier;
	device.current.timestamp = m_time;
	for( const auto& key : identifier.buttonElements )
	{
		device.current.buttons.emplace( key, Events::Button{} );
	}
	for( const auto& key : identifier.axisElements )
	{
		device.current.axis.emplace( key, Events::Axis{} );
	}
	for( const auto& key : identifier.switchElements )
	{
		device.current.switches.emplace( key, Events::Switch{} );
	}
	auto& inserted = m_devices.emplace( id, std::move( device ) ).first->second;

	if( m_deviceAdded )
	{
		m_deviceAdded( inserted.identifier );
	}
	return true;
}

bool MockInputHandler::RemoveDevice( BlueSharedString deviceId )
{
	auto it = m_devices.find( deviceId.c_str() );
	if( it == m_devices.end() )
	{
		CCP_LOGERR( "MockInputHandler::RemoveDevice: Device %s is not connected", deviceId.c_str() );
		return false;
	}

	auto identifier = it->second.identifier;
	m_devices.erase( it );
	if( m_deviceRemoved )
	{
		m_deviceRemoved( identifier );
	}
	return true;
}

bool MockInputHandler::SetButton( BlueSharedString deviceId, DeviceEnums::ElementKey key, bool pressed )
{
	auto* device = FindDevice( deviceId );
	if( !device )
	{
		return false;
	}
	auto it = device->current.buttons.find( key );
	if( it == device->current.buttons.end() )
	{
		CCP_LOGERR( "MockInputHandler::SetButton: Button %s (index %u) not found on device %s", DeviceEnums::ToKeyString( key.descriptor ), key.index, deviceId.c_str() );
		return false;
	}
	it->second.pressed = pressed;
	QueueSnapshot( *device );
	return true;
}

bool MockInputHandler::SetAxis( BlueSharedString deviceId, DeviceEnums::ElementKey key, float value )
{
	auto* device = FindDevice( deviceId );
	if( !device )
	{
		return false;
	}
	auto it = device->current.axis.find( key );
	if( it == device->current.axis.end() )
	{
		CCP_LOGERR( "MockInputHandler::SetAxis: Axis %s (index %u) not found on device %s", DeviceEnums::ToKeyString( key.descriptor ), key.index, deviceId.c_str() );
		return false;
	}
	it->second.value = value;
	QueueSnapshot( *device );
	return true;
}

bool MockInputHandler::SetSwitch( BlueSharedString deviceId, DeviceEnums::ElementKey key, Events::SwitchPosition position )
{
	auto* device = FindDevice( deviceId );
	if( !device )
	{
		return false;
	}
	auto it = device->current.switches.find( key );
	if( it == device->current.switches.end() )
	{
		CCP_LOGERR( "MockInputHandler::SetSwitch: Switch %s (index %u) not found on device %s", DeviceEnums::ToKeyString( key.descriptor ), key.index, deviceId.c_str() );
		return false;
	}
	it->second.position = position;
	QueueSnapshot( *device );
	return true;
}

void MockInputHandler::AdvanceTime( uint64_t microseconds )
{
	m_time += microseconds;
}

uint64_t MockInputHandler::GetTime() const
{
	return m_time;
}

bool MockInputHandler::IsDeviceActive( BlueSharedString deviceId ) const
{
	const auto* device = FindDevice( deviceId );
	return device && device->active;
}

Events::Rumble MockInputHandler::GetRumble( BlueSharedString deviceId ) const
{
	const auto* device = FindDevice( deviceId );
	return device ? device->rumble : Events::Rumble{};
}

bool MockInputHandler::GetBackgroundEventsEnabled() const
{
	return m_backgroundEventsEnabled;
}

MockInputHandler::MockDevice* MockInputHandler::FindDevice( BlueSharedString deviceId )
{
	auto it = m_devices.find( deviceId.c_str() );
	if( it == m_devices.end() )
	{
		CCP_LOGERR( "MockInputHandler: Device %s is not connected", deviceId.c_str() );
		return nullptr;
	}
	return &it->second;
}

const MockInputHandler::MockDevice* MockInputHandler::FindDevice( BlueSharedString deviceId ) const
{
	auto it = m_devices.find( deviceId.c_str() );
	return it != m_devices.end() ? &it->second : nullptr;
}

void MockInputHandler::QueueSnapshot( MockDevice& device )
{
	device.current.timestamp = m_time;
	if( device.active )
	{
		device.pending.push_back( device.current );
	}
}
