#ifdef WIN32
#include "InputHandlerWin.h"

#include <algorithm>
#include <Windows.h>
#include <gameinput.h>
#include "../ControlManager.h"

using namespace GameInput::v3;

namespace RegistryValues
{
std::wstring GetStringValueFromHKLM( const std::wstring& regSubKey, const std::wstring& regValue )
{
	size_t bufferSize = 0xFFF; // If too small, will be resized down below.
	std::wstring valueBuf; // Contiguous buffer since C++11.
	valueBuf.resize( bufferSize );
	auto cbData = static_cast<DWORD>( bufferSize * sizeof( wchar_t ) );
	auto rc = RegGetValueW(
		HKEY_LOCAL_MACHINE,
		regSubKey.c_str(),
		regValue.c_str(),
		RRF_RT_REG_SZ,
		nullptr,
		static_cast<void*>( valueBuf.data() ),
		&cbData );
	while( rc == ERROR_MORE_DATA )
	{
		// Get a buffer that is big enough.
		cbData /= sizeof( wchar_t );
		if( cbData > static_cast<DWORD>( bufferSize ) )
		{
			bufferSize = static_cast<size_t>( cbData );
		}
		else
		{
			bufferSize *= 2;
			cbData = static_cast<DWORD>( bufferSize * sizeof( wchar_t ) );
		}
		valueBuf.resize( bufferSize );
		rc = RegGetValueW(
			HKEY_LOCAL_MACHINE,
			regSubKey.c_str(),
			regValue.c_str(),
			RRF_RT_REG_SZ,
			nullptr,
			static_cast<void*>( valueBuf.data() ),
			&cbData );
	}
	if( rc == ERROR_SUCCESS )
	{
		cbData /= sizeof( wchar_t );
		valueBuf.resize( static_cast<size_t>( cbData - 1 ) ); // remove end null character
		return valueBuf;
	}
	else
	{
		return std::wstring( L"" );
	}
}
}

namespace
{
DeviceEnums::DeviceId GetDeviceID( const BYTE* bytes, uint32_t size )
{
	// Build a stable device ID by hashing the APP_LOCAL_DEVICE_ID bytes
	uint32_t hash = 2166136261u; // FNV-1a offset basis
	for( size_t i = 0; i < size; ++i )
	{
		hash ^= static_cast<uint32_t>( bytes[i] );
		hash *= 16777619u; // FNV-1a prime
	}
	return hash;
}
}

// ---------------------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------------------
InputHandlerWin::InputHandlerWin()
{
}

InputHandlerWin::~InputHandlerWin()
{
	ShutdownGameInput();
}


// ---------------------------------------------------------------------------
// GameInput lifetime
// ---------------------------------------------------------------------------
bool InputHandlerWin::Initialize()
{
	if( m_initialized )
	{
		return true;
	}

	HRESULT hr = GameInputCreate( &m_gameInput );
	if( FAILED( hr ) || !m_gameInput )
	{
		CCP_LOGERR( "InputHandlerWin: GameInputCreate failed (0x%08X)", hr );
		return false;
	}

	// Register for device connect / disconnect notifications.
	// Passing nullptr for the device filter means we get notified for all devices.
	hr = m_gameInput->RegisterDeviceCallback(
		nullptr, // no specific device filter
		SUPPORTED_INPUTS,
		GameInputDeviceConnected, // status filter
		GameInputBlockingEnumeration, // enumerate already-connected devices synchronously
		this, // context
		OnDeviceStatusChanged,
		&m_deviceCallbackToken );

	if( FAILED( hr ) )
	{
		CCP_LOGERR( "InputHandlerWin: RegisterDeviceCallback failed (0x%08X)", hr );
		m_gameInput->Release();
		m_gameInput = nullptr;
		return false;
	}

	m_initialized = true;
	CCP_LOGNOTICE( "InputHandlerWin: Initialized successfully" );
	return true;
}

void InputHandlerWin::ShutdownGameInput()
{
	if( !m_initialized )
	{
		return;
	}

	// Unregister the device callback before releasing devices
	if( m_gameInput )
	{
		if(  m_deviceCallbackToken != 0 )
		{
			m_gameInput->UnregisterCallback( m_deviceCallbackToken );
			m_deviceCallbackToken = 0;
		}

		for( auto& pair : m_deviceReadCallbackTokens )
		{
			m_gameInput->UnregisterCallback( pair.second );
		}
	}
	
	{
		std::lock_guard<std::mutex> lock( m_deviceMutex );
		for( auto& slot : m_deviceSlots )
		{
			if( slot.device )
			{
				slot.device->Release();
				slot.device = nullptr;
			}
		}
	}

	if( m_gameInput )
	{
		m_gameInput->Release();
		m_gameInput = nullptr;
	}

	m_initialized = false;
	CCP_LOGNOTICE( "InputHandlerWin: Shut down" );
}

// ---------------------------------------------------------------------------
// Device callback  (may be called from any thread)
// ---------------------------------------------------------------------------
void CALLBACK InputHandlerWin::OnDeviceStatusChanged(
	_In_ GameInputCallbackToken,
	_In_ void* context,
	_In_ IGameInputDevice* device,
	_In_ uint64_t,
	_In_ GameInputDeviceStatus currentStatus,
	_In_ GameInputDeviceStatus previousStatus ) noexcept
{
	auto* self = reinterpret_cast<InputHandlerWin*>( context );
	if( !self )
	{
		return;
	}

	const bool wasConnected = ( previousStatus & GameInputDeviceConnected ) != 0;
	const bool isConnected = ( currentStatus & GameInputDeviceConnected ) != 0;

	// we have no control over when this happens, so lock the device
	std::lock_guard<std::mutex> lock( self->m_deviceMutex );

	if( isConnected && !wasConnected )
	{
		device->AddRef();
		auto identifier = self->GetIdentifier( device );
		auto it = std::find_if( self->m_deviceSlots.begin(), self->m_deviceSlots.end(), [identifier]( const DeviceSlot& slot ) { return slot.identifier.deviceID == identifier.deviceID; } );
		if( it != self->m_deviceSlots.end() )
		{
			// This can happen if a device disconnects and reconnects again, no need to create a new slot for it, just update the existing one
			if( !it->device )
			{
				it->device = device;
			}
			it->pendingRemoval = false;
			CCP_LOGNOTICE( "InputHandlerWin: Device '%ls' reconnected", identifier.name.c_str() );
		}
		else
		{
			DeviceSlot slot = {
				device,
				false,
				identifier
			};

			self->m_deviceSlots.push_back( slot );
			CCP_LOGNOTICE( "InputHandlerWin: Device '%ls' connected", identifier.name.c_str() );
		}
		if( self->m_deviceAddedCallback )
		{
			self->m_deviceAddedCallback( identifier );
		}
	}
	else if( !isConnected && wasConnected )
	{
		// Mark the matching slot for removal on next Update()
		auto it = std::find_if( self->m_deviceSlots.begin(), self->m_deviceSlots.end(), [device]( const DeviceSlot& slot ) { return slot.device == device; } );
		if( it != self->m_deviceSlots.end() )
		{
			CCP_LOGNOTICE( "InputHandlerWin: Device '%ls' final disconnected", it->identifier.name.c_str() );

			it->pendingRemoval = true;
			self->m_devicesRemoved = true;
			if( self->m_deviceRemovedCallback )
			{
				self->m_deviceRemovedCallback( it->identifier );
			}
		}
	}
}

void CALLBACK InputHandlerWin::OnDeviceRead(
	_In_ GameInputCallbackToken callbackToken,
	_In_ void* context,
	_In_ IGameInputReading* reading ) noexcept
{
	auto* self = reinterpret_cast<InputHandlerWin*>( context );
	if( !self || !reading )
	{
		return;
	}

	// find the device id
	IGameInputDevice* device;
	reading->GetDevice( &device );

	if( !device )
	{
		return;
	}

	auto foundSlot = std::find_if( self->m_deviceSlots.begin(), self->m_deviceSlots.end(), [device]( const DeviceSlot& slot ) { return slot.device == device; } );

	if( foundSlot != self->m_deviceSlots.end() )
	{
		auto state = self->ReadDeviceState( reading );
		{
			std::lock_guard<std::mutex> lock( self->m_readingMutex );
			self->m_accumulatedStates[foundSlot->identifier.deviceID].push_back( state );
		}
	}
}

void InputHandlerWin::SetDeviceActivation( DeviceEnums::DeviceId deviceID, bool activate )
{
	if( activate )
	{
		// find the device and register it for reading callbacks
		auto foundDevice = std::find_if( m_deviceSlots.begin(), m_deviceSlots.end(), [deviceID]( const DeviceSlot& slot ) {
			return slot.identifier.deviceID == deviceID;
		} );
		if( foundDevice != m_deviceSlots.end() )
		{
			GameInputCallbackToken token;
			auto hr = m_gameInput->RegisterReadingCallback(
				nullptr,
				SUPPORTED_INPUTS,
				this,
				OnDeviceRead,
				&token );
			m_deviceReadCallbackTokens.emplace_back( deviceID, token );
		}
	}
	else
	{
		// find the device and unregister it from reading callbacks
		auto it = std::find_if( m_deviceReadCallbackTokens.begin(), m_deviceReadCallbackTokens.end(), [deviceID]( const std::pair<DeviceEnums::DeviceId, GameInputCallbackToken>& pair ) {
			return pair.first == deviceID;
		} );
		if( it != m_deviceReadCallbackTokens.end() )
		{
			m_gameInput->UnregisterCallback( it->second );
			m_deviceReadCallbackTokens.erase( it );
		}
	}
}

std::vector<Events::State> InputHandlerWin::Update( DeviceEnums::DeviceId deviceID )
{
	if( !m_initialized )
	{
		return {};
	}
	std::lock_guard<std::mutex> lock( m_deviceMutex );

	// need to remove devices here, but not in the callback 
	if( m_devicesRemoved )
	{
		for( auto& slot : m_deviceSlots )
		{
			if( slot.pendingRemoval )
			{
				if( slot.device )
				{
					slot.device->Release();
					slot.device = nullptr;
					slot.pendingRemoval = false;

					CCP_LOGNOTICE( "InputHandlerWin: Device '%ls' final removal", slot.identifier.name.c_str() );
				}
				auto foundCallback = std::remove_if( m_deviceReadCallbackTokens.begin(), m_deviceReadCallbackTokens.end(), [&slot]( const std::pair<DeviceEnums::DeviceId, GameInputCallbackToken>& pair ) {
					return pair.first == slot.identifier.deviceID;
				} );
				if( foundCallback != m_deviceReadCallbackTokens.end() )
				{
					m_gameInput->UnregisterCallback( foundCallback->second );
					m_deviceReadCallbackTokens.erase( foundCallback );
				}
			}
		}
		m_devicesRemoved = false;
	}

	std::vector<Events::State> statesForDevice = {};
	{
		std::lock_guard<std::mutex> lock( m_readingMutex );
		auto it = m_accumulatedStates.find( deviceID );
		if( it != m_accumulatedStates.end() )
		{
			std::swap( statesForDevice, it->second );

		}
	}

	return statesForDevice;
}

DeviceEnums::DeviceIdentifier InputHandlerWin::GetIdentifier( IGameInputDevice* device )
{
	DeviceEnums::DeviceIdentifier identifier;

	if( !device )
	{
		return identifier;
	}

	const GameInputDeviceInfo* info = nullptr;
	device->GetDeviceInfo( &info );

	if( info == nullptr )
	{
		return identifier;
	}

	identifier.deviceID = GetDeviceID( info->deviceId.value, sizeof( info->deviceId.value ) );
	identifier.rawDeviceId = info->deviceId;

	char vid[16];
	snprintf( vid, sizeof( vid ), "%04X", info->vendorId );

	char pid[16];
	snprintf( pid, sizeof( pid ), "%04X", info->productId );

	identifier.vendorID = BlueSharedString( vid );
	identifier.productID = BlueSharedString( pid );

	if( info->deviceFamily == GameInputDeviceFamily::GameInputFamilyHid )
	{
		std::wstring vid_w( static_cast<const wchar_t*>( CA2W( vid ) ) );
		std::wstring pid_w( static_cast<const wchar_t*>( CA2W( pid ) ) );

		// check the registry for the device name, using the vendor/product ID as a key
		auto registryName = RegistryValues::GetStringValueFromHKLM(
			L"SYSTEM\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Joystick\\OEM\\VID_" + vid_w + L"&PID_" + pid_w,
			L"OEMName" );
		if( !registryName.empty() )
		{
			identifier.name = BlueSharedStringW( registryName.c_str() );
		}
	}

	if( identifier.name.empty() && info->displayName )
	{
		identifier.name = BlueSharedStringW( static_cast<const wchar_t*>( CA2W( info->displayName ) ) );
	}

	bool hasLowFreq = ( info->supportedRumbleMotors & GameInputRumbleMotors::GameInputRumbleLowFrequency ) != 0;
	bool hasHighFreq = ( info->supportedRumbleMotors & GameInputRumbleMotors::GameInputRumbleHighFrequency ) != 0;
	bool hasLeftTrigger = ( info->supportedRumbleMotors & GameInputRumbleMotors::GameInputRumbleLeftTrigger ) != 0;
	bool hasRightTrigger = ( info->supportedRumbleMotors & GameInputRumbleMotors::GameInputRumbleRightTrigger ) != 0;

	identifier.rumbleCapacity.hasLowFrequencyRumble = hasLowFreq;
	identifier.rumbleCapacity.hasHighFrequencyRumble = hasHighFreq;
	identifier.rumbleCapacity.hasLeftTriggerRumble = hasLeftTrigger;
	identifier.rumbleCapacity.hasRightTriggerRumble = hasRightTrigger;

	identifier.rumbleCapacity.rumbleMotorCount = hasLowFreq + hasHighFreq + hasLeftTrigger + hasRightTrigger;
	if( info->controllerInfo != nullptr )
	{
		identifier.axisCount = info->controllerInfo->controllerAxisCount;
		identifier.buttonCount = info->controllerInfo->controllerButtonCount;
		identifier.switchCount = info->controllerInfo->controllerSwitchCount;
	}
	return identifier;
}

void InputHandlerWin::RegisterForDeviceAdded( DEVICE_CHANGED_CALLBACK callback )
{
	m_deviceAddedCallback = callback;
}

void InputHandlerWin::RegisterForDeviceRemoved( DEVICE_CHANGED_CALLBACK callback )
{
	m_deviceRemovedCallback = callback;
}

// ---------------------------------------------------------------------------
// ReadDeviceState  –  get the most recent reading for a device
// ---------------------------------------------------------------------------
Events::State InputHandlerWin::ReadDeviceState( IGameInputReading* reading )
{
	Events::State state = {};

	if( !reading )
	{
		return state;
	}

	state.timestamp = Events::GetTimestamp();

	// convert the GameInputReading into our internal State representation
	auto buttonCount = reading->GetControllerButtonCount();
	auto axisCount = reading->GetControllerAxisCount();
	auto switchCount = reading->GetControllerSwitchCount();

	if( buttonCount > 0 )
	{
		auto buttonReading = std::make_unique<bool[]>( buttonCount );
		reading->GetControllerButtonState( buttonCount, reinterpret_cast<bool*>( buttonReading.get() ) );

		state.buttons.resize( buttonCount );
		for( uint32_t index = 0; index < buttonCount; ++index )
		{
			state.buttons[index].pressed = buttonReading[index] != 0;
		}
	}

	if( axisCount > 0 )
	{
		auto axisReading = std::make_unique<float[]>( axisCount );
		reading->GetControllerAxisState( axisCount, axisReading.get() );
		state.axis.resize( axisCount );
		for( uint32_t index = 0; index < axisCount; ++index )
		{
			state.axis[index].value = axisReading[index];
		}
	}

	if( switchCount > 0 )
	{
		auto switchReading = std::make_unique<GameInputSwitchPosition[]>( switchCount );
		state.switches.resize( switchCount );
		reading->GetControllerSwitchState( switchCount, switchReading.get() );
		for( uint32_t index = 0; index < state.switches.size(); ++index )
		{
			state.switches[index].position = static_cast<Events::SwitchPosition>( switchReading[index] );
		}
	}
	reading->Release();

	return state;
}

void InputHandlerWin::Rumble( DeviceEnums::DeviceId deviceID, Events::Rumble rumble )
{
	std::lock_guard<std::mutex> lock( m_deviceMutex );
	for( auto& slot : m_deviceSlots )
	{
		if( slot.device && slot.identifier.deviceID == deviceID )
		{
			GameInputRumbleParams rumbleParams = {};
			rumbleParams.highFrequency = rumble.highFrequency;
			rumbleParams.lowFrequency = rumble.lowFrequency;
			rumbleParams.leftTrigger = rumble.leftTrigger;
			rumbleParams.rightTrigger = rumble.rightTrigger;

			slot.device->SetRumbleState(&rumbleParams);
			break;
		}
	}
}

#endif // WIN32