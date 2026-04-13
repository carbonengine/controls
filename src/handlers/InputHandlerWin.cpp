#include "../StdAfx.h"
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
	InitializeGameInput();
}

InputHandlerWin::~InputHandlerWin()
{
	ShutdownGameInput();
}

// ---------------------------------------------------------------------------
// GameInput lifetime
// ---------------------------------------------------------------------------
bool InputHandlerWin::InitializeGameInput()
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
	if( m_gameInput && m_deviceCallbackToken != 0 )
	{
		m_gameInput->UnregisterCallback( m_deviceCallbackToken );
		m_deviceCallbackToken = 0;
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
			// This can happen if a device disconnects and reconnects quickly, before the disconnect has been processed. Reuse the existing slot in this case.
			it->device = device;
			CCP_LOGNOTICE( "InputHandlerWin: Device '%ls' reconnected", identifier.name.c_str() );
		}
		else
		{
			DeviceSlot slot = {
				device,
				identifier
			};

			self->m_deviceSlots.push_back( slot );
			CCP_LOGNOTICE( "InputHandlerWin: Device '%ls' connected", identifier.name.c_str() );
		}
	}
	else if( !isConnected && wasConnected )
	{
		// Mark the matching slot for removal on next Update()
		auto it = std::find_if( self->m_deviceSlots.begin(), self->m_deviceSlots.end(), [device]( const DeviceSlot& slot ) { return slot.device == device; } );
		if( it != self->m_deviceSlots.end() )
		{
			it->device->Release();
			it->device = nullptr;
			CCP_LOGNOTICE( "InputHandlerWin: Device '%ls' disconnected", it->identifier.name.c_str() );
		}
	}
	if( self->m_deviceChangedCallback )
	{
		self->m_deviceChangedCallback( self->GetAllDeviceIdentifiers() );
	}
}

Events::State InputHandlerWin::Update( DeviceEnums::DeviceId deviceID )
{
	if( !m_initialized )
	{
		return {};
	}
	std::lock_guard<std::mutex> lock( m_deviceMutex );

	for( auto& slot : m_deviceSlots )
	{
		if( slot.device && slot.identifier.deviceID == deviceID )
		{
			return ReadDeviceState( slot.device );
		}
	}

	return {};
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

	identifier.deviceID = GetDeviceID( info->deviceId.value, sizeof( info->deviceId.value ) );
	identifier.rawDeviceId = info->deviceId;

	char vid[16];
	snprintf( vid, sizeof( vid ), "%04X", info->vendorId );

	char pid[16];
	snprintf( pid, sizeof( pid ), "%04X", info->productId );

	identifier.manufacturer = BlueSharedString( vid );
	identifier.product = BlueSharedString( pid );

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

	if( info->supportedInput & GameInputKindGamepad )
	{
		identifier.deviceType = DeviceEnums::DeviceType_Gamepad;
	}
	else if( ( info->supportedInput & GameInputKindController ) || ( info->supportedInput & GameInputKindControllerAxis ) || ( info->supportedInput & GameInputKindControllerButton ) || ( info->supportedInput & GameInputKindControllerSwitch ) )
	{
		identifier.deviceType = DeviceEnums::DeviceType_Controller;
	}
	identifier.rumbleSupported = info->forceFeedbackMotorCount != 0;
	identifier.axisCount = info->controllerInfo->controllerAxisCount;
	identifier.buttonCount = info->controllerInfo->controllerButtonCount;
	identifier.switchCount = info->controllerInfo->controllerSwitchCount;
	return identifier;
}

std::vector<DeviceEnums::DeviceIdentifier> InputHandlerWin::GetAllDeviceIdentifiers()
{
	auto devices = std::vector<DeviceEnums::DeviceIdentifier>{};
	devices.reserve( m_deviceSlots.size() );
	for( const auto& slot : m_deviceSlots )
	{
		if( slot.device )
		{
			devices.push_back( slot.identifier );
		}
	}
	return devices;
}

void InputHandlerWin::RegisterForDeviceChange( std::function<void( std::vector<DeviceEnums::DeviceIdentifier> )> callback )
{
	m_deviceChangedCallback = callback;
}

// ---------------------------------------------------------------------------
// ReadDeviceState  –  get the most recent reading for a device
// ---------------------------------------------------------------------------
Events::State InputHandlerWin::ReadDeviceState( IGameInputDevice* device )
{
	IGameInputReading* reading = nullptr;

	Events::State state = {};

	if( !device )
	{
		return state;
	}

	const GameInputDeviceInfo* info = nullptr;
	device->GetDeviceInfo( &info );
	m_gameInput->SetFocusPolicy( GameInputFocusPolicy::GameInputEnableBackgroundInput );

	// Only request input kinds that this specific device supports
	GameInputKind readingFilter = static_cast<GameInputKind>( info->supportedInput );
	if( readingFilter == GameInputKindUnknown )
	{
		return state;
	}

	HRESULT hr = m_gameInput->GetCurrentReading( info->supportedInput, device, &reading );
	if( FAILED( hr ) )
	{
		CCP_LOGERR( "InputHandlerWin: GetCurrentReading failed for device '%s' (0x%08X)", info->displayName, hr );
		return state;
	}
	if( !reading )
	{
		return state;
	}

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
			state.buttons[index]._pressed = buttonReading[index] != 0;
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

#endif // WIN32