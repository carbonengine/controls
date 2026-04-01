#include "../StdAfx.h"
#include "InputHandlerWin.h"

#include <algorithm>
#include <Windows.h>
#include "../ControlManager.h"

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
	CCP_LOG( "InputHandlerWin: Initialized successfully" );
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
		m_gameInput->UnregisterCallback( m_deviceCallbackToken, 500 );
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
	CCP_LOG( "InputHandlerWin: Shut down" );
}

// ---------------------------------------------------------------------------
// Device callback  (may be called from any thread)
// ---------------------------------------------------------------------------
void CALLBACK InputHandlerWin::OnDeviceStatusChanged(
	_In_ GameInputCallbackToken /*callbackToken*/,
	_In_ void* context,
	_In_ IGameInputDevice* device,
	_In_ uint64_t /*timestamp*/,
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
		DeviceSlot slot = {
			device,
			nullptr,
			false,
			identifier
		};

		self->m_deviceSlots.push_back( slot );
		CCP_LOGNOTICE( "InputHandlerWin: Device '%ls' connected", identifier.name.c_str() );
	}
	else if( !isConnected && wasConnected )
	{
		// Mark the matching slot for removal on next Update()
		auto it = std::find_if( self->m_deviceSlots.begin(), self->m_deviceSlots.end(), [device]( const DeviceSlot& slot ) { return slot.device == device; } );
		if( it != self->m_deviceSlots.end() )
		{
			it->needDelete = true;
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
	m_gameInput->SetFocusPolicy( GameInputDefaultFocusPolicy );

	std::lock_guard<std::mutex> lock( m_deviceMutex );

	for( auto& slot : m_deviceSlots )
	{
		// Remove devices flagged for deletion by the callback
		if( slot.needDelete )
		{
			if( slot.device )
			{
				slot.device->Release();
				slot.device = nullptr;
			}
			slot.needDelete = false;
		}
		else
		{
			if( slot.identifier.deviceID == deviceID )
			{
				return ReadDeviceState( slot.device );
			}
		}
	}

	return{};
}

DeviceEnums::DeviceIdentifier InputHandlerWin::GetIdentifier( IGameInputDevice* device )
{
	DeviceEnums::DeviceIdentifier identifier;

	if( !device )
	{
		return identifier;
	}
	auto info = device->GetDeviceInfo( );

	// Build a stable device ID by hashing the APP_LOCAL_DEVICE_ID bytes
	uint32_t hash = 2166136261u; // FNV-1a offset basis
	for( size_t i = 0; i < sizeof( info->deviceId.value ); ++i )
	{
		hash ^= static_cast<uint32_t>( info->deviceId.value[i] );
		hash *= 16777619u; // FNV-1a prime
	}
	identifier.deviceID = hash;

	if( info->displayName )
	{
		identifier.name = BlueSharedStringW( static_cast<const wchar_t*>(CA2W( info->displayName->data ) ) );
	}
	else if( info->deviceFamily == GameInputDeviceFamily::GameInputFamilyXboxOne || info->deviceFamily == GameInputDeviceFamily::GameInputFamilyXbox360 )
	{
		// For Xbox controllers, we can use a friendly name based on the device family
		if( info->deviceFamily == GameInputDeviceFamily::GameInputFamilyXboxOne )
		{
			identifier.name = BlueSharedStringW( L"Xbox One Controller" );
		}
		else 
		{
			identifier.name = BlueSharedStringW( L"Xbox 360 Controller" );
		}
	}
	else
	{
		char vid[16];
		snprintf( vid, sizeof( vid ), "%04X", info->vendorId );

		char pid[16];
		snprintf( pid, sizeof( pid ), "%04X", info->productId );

		std::wstring vid_w( static_cast<const wchar_t*>(CA2W( vid ) ) );
		std::wstring pid_w( static_cast<const wchar_t*>(CA2W( pid ) ) );

		// check the registry for the device name, using the vendor/product ID as a key
		auto registryName = RegistryValues::GetStringValueFromHKLM(
			L"SYSTEM\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Joystick\\OEM\\VID_" + vid_w + L"&PID_" + pid_w,
			L"OEMName" );
		if( !registryName.empty() )
		{
			identifier.name = BlueSharedStringW( registryName.c_str() );
		}
		else
		{
			// Last resort: identify by vendor/product ID
			char fallback[64];
			snprintf( fallback, sizeof( fallback ), "HID Device [VID: %04X - PID: %04X]", info->vendorId, info->productId );
			identifier.name = BlueSharedStringW( static_cast<const wchar_t*>(CA2W( fallback ) ) );
		}
	}

	if( info->supportedInput & GameInputKindGamepad )
	{
		identifier.deviceType = DeviceEnums::DeviceType_Gamepad;
	}
	else if( info->supportedInput & GameInputKindFlightStick )
	{
		identifier.deviceType = DeviceEnums::DeviceType_FlightStick;
	}
	else if( ( info->supportedInput & GameInputKindController ) || ( info->supportedInput & GameInputKindControllerAxis ) || ( info->supportedInput & GameInputKindControllerButton ) || ( info->supportedInput & GameInputKindControllerSwitch ) )
	{
		identifier.deviceType = DeviceEnums::DeviceType_Controller;
	}

	identifier.axisCount = info->controllerAxisCount;
	identifier.buttonCount = info->controllerButtonCount;
	identifier.switchCount = info->controllerSwitchCount;

	GameInputBatteryState batteryState;
	device->GetBatteryState( &batteryState );

	identifier.batteryPowered = batteryState.status != GameInputBatteryStatus::GameInputBatteryNotPresent;
	identifier.rumbleSupported = info->hapticFeedbackMotorInfo != nullptr && info->hapticFeedbackMotorInfo->mappedRumbleMotors != GameInputRumbleNone;

	return identifier;
}

std::vector<DeviceEnums::DeviceIdentifier> InputHandlerWin::GetAllDeviceIdentifiers()
{
	auto devices = std::vector<DeviceEnums::DeviceIdentifier>{};
	devices.reserve( m_deviceSlots.size() );
	for( const auto& slot : m_deviceSlots )
	{
		if( slot.device && !slot.needDelete )
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
	return state;
	m_gameInput->SetFocusPolicy( GameInputDefaultFocusPolicy );

	// Only request input kinds that this specific device supports
	GameInputKind readingFilter = static_cast<GameInputKind>( SUPPORTED_INPUTS );
	if( readingFilter == GameInputKindUnknown )
	{
		return state;
	}

	//HRESULT hr = m_gameInput->GetCurrentReading( info->supportedInput, device, &reading );
	//if( FAILED( hr ) )
	//{
	//	CCP_LOGERR( "InputHandlerWin: GetCurrentReading failed for device '%s' (0x%08X)", info->displayName, hr );
	//	return state;
	//}
	//if( !reading )
	//{
	//	return state;
	//}

	//// convert the GameInputReading into our internal State representation
	//GetBatteryState( reading, device, state.batteryState );
	//GetGamePadState( reading, device, state.gamePadState );
	//GetControllerState( reading, device, state.controllerState );

	//DebugAllReadings( reading );

	//reading->Release();

	//return state;
}

void InputHandlerWin::GetBatteryState( IGameInputReading* reading, IGameInputDevice* device, Events::BatteryState& batteryState )
{
	batteryState = {}; // default to 0 capacity and not charging
	if( !device )
	{
		return;
	}/*
	GameInputBatteryState deviceBatteryState;
	device->GetBatteryState( &deviceBatteryState );
	if( deviceBatteryState.status == GameInputBatteryStatus::GameInputBatteryNotPresent )
	{
		return;
	}
	batteryState.remainingCapacity = deviceBatteryState.remainingCapacity;
	batteryState.fullChargeCapacity = deviceBatteryState.fullChargeCapacity;
	batteryState.charging = deviceBatteryState.status == GameInputBatteryStatus::GameInputBatteryCharging;*/
}

void InputHandlerWin::GetControllerState( IGameInputReading* reading, IGameInputDevice* device, Events::ControllerState& controllerState )
{
	controllerState = {}; 
	if( !device )
	{
		return;
	}
	auto buttonCount = reading->GetControllerButtonCount();
	auto axisCount = reading->GetControllerAxisCount();
	auto switchCount = reading->GetControllerSwitchCount();

	if( buttonCount > 0 )
	{
		// Use a raw byte buffer — bool[] can cause ABI issues with COM interfaces
		std::vector<uint8_t> buttonState( buttonCount );
		uint32_t tmp = reading->GetControllerButtonState( buttonCount, reinterpret_cast<bool*>( buttonState.data() ) );

		controllerState.buttons.resize( buttonCount );
		for( uint32_t index = 0; index < buttonCount; ++index )
		{
			controllerState.buttons[index]._pressed = buttonState[index] != 0;
		}
	}

	if( axisCount > 0 )
	{
		controllerState.axis.resize( axisCount );
		uint32_t tmp = reading->GetControllerAxisState( axisCount, controllerState.axis.data() );
	}

	if( switchCount > 0 )
	{
		auto switchReading = std::make_unique<GameInputSwitchPosition[]>( switchCount );
		controllerState.switches.resize( switchCount );
		uint32_t tmp = reading->GetControllerSwitchState( static_cast<uint32_t>( controllerState.switches.size() ), switchReading.get() );
		for( uint32_t index = 0; index < controllerState.switches.size(); ++index )
		{
			controllerState.switches[index] = static_cast<Events::SwitchPosition>( switchReading[index] );
		}
	}
}

void InputHandlerWin::GetGamePadState( IGameInputReading* reading, IGameInputDevice* device, Events::GamePadState& gamePadState )
{
	gamePadState = {}; // default to all buttons released and triggers/thumbsticks centered
	if( !device )
	{
		return;
	}

	//if( ( info->supportedInput & GameInputKindGamepad ) == 0 )
	//{
	//	return;
	//}

	GameInputGamepadState deviceGamePadState;
	reading->GetGamepadState( &deviceGamePadState );
	
	gamePadState.a._pressed = (deviceGamePadState.buttons & GameInputGamepadButtons::GameInputGamepadA) == GameInputGamepadButtons::GameInputGamepadA;
	gamePadState.b._pressed = (deviceGamePadState.buttons & GameInputGamepadButtons::GameInputGamepadB) == GameInputGamepadButtons::GameInputGamepadB;
	gamePadState.x._pressed = (deviceGamePadState.buttons & GameInputGamepadButtons::GameInputGamepadX) == GameInputGamepadButtons::GameInputGamepadX;
	gamePadState.y._pressed = (deviceGamePadState.buttons & GameInputGamepadButtons::GameInputGamepadY) == GameInputGamepadButtons::GameInputGamepadY;
	gamePadState.leftShoulder._pressed = ( deviceGamePadState.buttons & GameInputGamepadButtons::GameInputGamepadLeftShoulder ) == GameInputGamepadButtons::GameInputGamepadLeftShoulder;
	gamePadState.rightShoulder._pressed = ( deviceGamePadState.buttons & GameInputGamepadButtons::GameInputGamepadRightShoulder ) == GameInputGamepadButtons::GameInputGamepadRightShoulder;
	gamePadState.menu._pressed = ( deviceGamePadState.buttons & GameInputGamepadButtons::GameInputGamepadMenu ) == GameInputGamepadButtons::GameInputGamepadMenu;
	gamePadState.view._pressed = ( deviceGamePadState.buttons & GameInputGamepadButtons::GameInputGamepadView ) == GameInputGamepadButtons::GameInputGamepadView;

	gamePadState.dpadUp._pressed = ( deviceGamePadState.buttons & GameInputGamepadButtons::GameInputGamepadDPadUp ) == GameInputGamepadButtons::GameInputGamepadDPadUp;
	gamePadState.dpadDown._pressed = ( deviceGamePadState.buttons & GameInputGamepadButtons::GameInputGamepadDPadDown ) == GameInputGamepadButtons::GameInputGamepadDPadDown;
	gamePadState.dpadLeft._pressed = ( deviceGamePadState.buttons & GameInputGamepadButtons::GameInputGamepadDPadLeft ) == GameInputGamepadButtons::GameInputGamepadDPadLeft;
	gamePadState.dpadRight._pressed = ( deviceGamePadState.buttons & GameInputGamepadButtons::GameInputGamepadDPadRight ) == GameInputGamepadButtons::GameInputGamepadDPadRight;

	gamePadState.leftTrigger.amountPressed = deviceGamePadState.leftTrigger;
	gamePadState.rightTrigger.amountPressed = deviceGamePadState.rightTrigger;

	gamePadState.leftThumbstick.x = deviceGamePadState.leftThumbstickX;
	gamePadState.leftThumbstick.y = deviceGamePadState.leftThumbstickY;
	gamePadState.leftThumbstick.button._pressed = ( deviceGamePadState.buttons & GameInputGamepadButtons::GameInputGamepadLeftThumbstick ) == GameInputGamepadButtons::GameInputGamepadLeftThumbstick;

	gamePadState.rightThumbstick.x = deviceGamePadState.rightThumbstickX;
	gamePadState.rightThumbstick.y = deviceGamePadState.rightThumbstickY;
	gamePadState.rightThumbstick.button._pressed = ( deviceGamePadState.buttons & GameInputGamepadButtons::GameInputGamepadRightThumbstick ) == GameInputGamepadButtons::GameInputGamepadRightThumbstick;
}

void InputHandlerWin::GetFlightStickState( IGameInputReading* reading, IGameInputDevice* device, Events::FlightStickState& flightStickState )
{
	flightStickState = {}; // default to all buttons released and sticks centered
	if( !device )
	{
		return;
	}

	/*if( ( info->supportedInput & GameInputKindFlightStick ) == 0 )
	{
		return;
	}*/
	/*GameInputFlightStickState deviceFlightStickState;
	reading->GetFlightStickState( &deviceFlightStickState );
	flightStickState.yaw = deviceFlightStickState.yaw;
	flightStickState.pitch = deviceFlightStickState.pitch;
	flightStickState.roll = deviceFlightStickState.roll;
	flightStickState.firePrimary._pressed = ( deviceFlightStickState.buttons & GameInputFlightStickButtons::GameInputFlightStickFirePrimary ) == GameInputFlightStickButtons::GameInputFlightStickFirePrimary;
	flightStickState.fireSecondary._pressed = ( deviceFlightStickState.buttons & GameInputFlightStickButtons::GameInputFlightStickFireSecondary ) == GameInputFlightStickButtons::GameInputFlightStickFireSecondary;*/
}

void InputHandlerWin::DebugAllReadings( IGameInputReading* reading )
{
	if( !reading )
	{
		return;
	}
	GameInputGamepadState gamePadState;
	if( SUCCEEDED( reading->GetGamepadState( &gamePadState ) ) )
	{
		CCP_LOGNOTICE( "GamePad State: buttons=0x%08X, leftTrigger=%.2f, rightTrigger=%.2f, leftThumbstick=(%.2f, %.2f), rightThumbstick=(%.2f, %.2f)",
			gamePadState.buttons,
			gamePadState.leftTrigger,
			gamePadState.rightTrigger,
			gamePadState.leftThumbstickX,
			gamePadState.leftThumbstickY,
			gamePadState.rightThumbstickX,
			gamePadState.rightThumbstickY );
	}
	GameInputFlightStickState flightStickState;
	if( SUCCEEDED( reading->GetFlightStickState( &flightStickState ) ) )
	{
		CCP_LOGNOTICE( "FlightStick State: buttons=0x%08X, hatSwitch=%d, roll=%.2f, pitch=%.2f, yaw=%.2f, throttle=%.2f",
			flightStickState.buttons,
			flightStickState.hatSwitch,
			flightStickState.roll,
			flightStickState.pitch,
			flightStickState.yaw,
			flightStickState.throttle );
	}

	uint32_t buttonCount = reading->GetControllerButtonCount();
	if( buttonCount > 0 )
	{
		std::unique_ptr<bool[]> buttonState = std::make_unique<bool[]>( buttonCount );
		uint32_t tmp = reading->GetControllerButtonState( buttonCount, buttonState.get());
		std::string buttonStateStr = "Controller Buttons: ";
		for( uint32_t i = 0; i < buttonCount; ++i )
		{
			buttonStateStr += buttonState[i] ? "[X]" : "[ ]";
		}
		CCP_LOGNOTICE( "Controller Button State: %s", buttonStateStr.c_str() );
	}

	uint32_t axisCount = reading->GetControllerAxisCount();
	if( axisCount > 0 )
	{
		std::unique_ptr<float[]> axisState = std::make_unique<float[]>( axisCount );
		uint32_t tmp = reading->GetControllerAxisState( axisCount, axisState.get() );
		std::string axisStateStr = "Controller Axes: ";
		for( uint32_t i = 0; i < axisCount; ++i )
		{
			axisStateStr += "[" + std::to_string( axisState[i] ) + "]";
		}
		CCP_LOGNOTICE( "Controller Axis State: %s", axisStateStr.c_str() );
	}

	uint32_t switchCount = reading->GetControllerSwitchCount();
	if( switchCount > 0 )
	{
		std::unique_ptr<GameInputSwitchPosition[]> switchState = std::make_unique<GameInputSwitchPosition[]>( switchCount );
		uint32_t tmp = reading->GetControllerSwitchState( switchCount, switchState.get() );
		std::string switchStateStr = "Controller Switches: ";
		for( uint32_t i = 0; i < switchCount; ++i )
		{
			switchStateStr += "[" + std::to_string( switchState[i] ) + "]";
		}
		CCP_LOGNOTICE( "Controller Switch State: %s", switchStateStr.c_str() );
	}
}