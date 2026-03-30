#include "../StdAfx.h"
#include "InputHandlerWin.h"

#include <algorithm>

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
		m_gameInput->UnregisterCallback( m_deviceCallbackToken, 5000 );
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
		DeviceSlot slot = {
			device,
			false,
			{}
		};

		slot.identifier = GetIdentifier( device );
		self->m_deviceSlots.push_back( slot );
		CCP_LOGNOTICE( "InputHandlerWin: Device '%s' connected", device->GetDeviceInfo()->displayName );
	}
	else if( !isConnected && wasConnected )
	{
		// Mark the matching slot for removal on next Update()
		auto it = std::find_if( self->m_deviceSlots.begin(), self->m_deviceSlots.end(), [device]( const DeviceSlot& slot ) { return slot.device == device; } );
		if( it != self->m_deviceSlots.end() )
		{
			it->needDelete = true;
			CCP_LOGNOTICE( "InputHandlerWin: Device '%s' disconnected", device->GetDeviceInfo()->displayName );
		}
	}
}

// ---------------------------------------------------------------------------
// Update  –  per-frame tick: clean up disconnected devices, read fresh state
// ---------------------------------------------------------------------------
Events::State InputHandlerWin::Update( DeviceEnums::DeviceId deviceID )
{
	if( !m_initialized )
	{
		return {};
	}

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

	const GameInputDeviceInfo* info = device->GetDeviceInfo();
	if( !info )
	{
		return identifier;
	}

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
		identifier.name = BlueSharedString(info->displayName->data);
	}

	if( info->supportedInput & GameInputKindGamepad )
	{
		identifier.deviceType = DeviceEnums::DeviceType_Gamepad;
	}
	else if( info->supportedInput & GameInputKindFlightStick )
	{
		identifier.deviceType = DeviceEnums::DeviceType_FlightStick;
	}

	return identifier;
}

std::vector<DeviceEnums::DeviceIdentifier> InputHandlerWin::GetAllDeviceIdentifiers()
{
	auto devices = std::vector<DeviceEnums::DeviceIdentifier>{};
	devices.reserve( m_deviceSlots.size() );
	for( const auto& slot : m_deviceSlots )
	{
		devices.push_back( slot.identifier );
	}
	return devices;
}

// ---------------------------------------------------------------------------
// ReadDeviceState  –  get the most recent reading for a device
// ---------------------------------------------------------------------------
Events::State InputHandlerWin::ReadDeviceState( IGameInputDevice* device )
{
	IGameInputReading* reading = nullptr;

	HRESULT hr = m_gameInput->GetCurrentReading( SUPPORTED_INPUTS, device, &reading );
	if( FAILED( hr ) || !reading )
	{
		return {};
	}

	Events::State state = {};

	// convert the GameInputReading into our internal State representation
	GetBatteryState( reading, device, state.batteryState );
	GetGamePadState( reading, device, state.gamePadState );
	GetFlightStickState( reading, device, state.flightStickState );

	reading->Release();

	return state;
}

void InputHandlerWin::GetBatteryState( IGameInputReading* reading, IGameInputDevice* device, Events::BatteryState& batteryState )
{
	if( !device )
	{
		return;
	}
	GameInputBatteryState deviceBatteryState;
	device->GetBatteryState( &deviceBatteryState );
	if( deviceBatteryState.status == GameInputBatteryStatus::GameInputBatteryNotPresent )
	{
		return;
	}
	batteryState.remainingCapacity = deviceBatteryState.remainingCapacity;
	batteryState.fullChargeCapacity = deviceBatteryState.fullChargeCapacity;
	batteryState.charging = deviceBatteryState.status == GameInputBatteryStatus::GameInputBatteryCharging;
}

void InputHandlerWin::GetGamePadState( IGameInputReading* reading, IGameInputDevice* device, Events::GamePadState& gamePadState )
{
	if( !device )
	{
		return;
	}
	const GameInputDeviceInfo* info = device->GetDeviceInfo();
	if( !info )
	{
		return;
	}

	if( ( info->supportedInput & GameInputKindGamepad ) == 0 )
	{
		return;
	}

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
	if( !device )
	{
		return;
	}
	const GameInputDeviceInfo* info = device->GetDeviceInfo();
	if( !info )
	{
		return;
	}

	if( ( info->supportedInput & GameInputKindFlightStick ) == 0 )
	{
		return;
	}
	GameInputFlightStickState deviceFlightStickState;
	reading->GetFlightStickState( &deviceFlightStickState );
	flightStickState.yaw = deviceFlightStickState.yaw;
	flightStickState.pitch = deviceFlightStickState.pitch;
	flightStickState.roll = deviceFlightStickState.roll;
	flightStickState.firePrimary._pressed = ( deviceFlightStickState.buttons & GameInputFlightStickButtons::GameInputFlightStickFirePrimary ) == GameInputFlightStickButtons::GameInputFlightStickFirePrimary;
	flightStickState.fireSecondary._pressed = ( deviceFlightStickState.buttons & GameInputFlightStickButtons::GameInputFlightStickFireSecondary ) == GameInputFlightStickButtons::GameInputFlightStickFireSecondary;
}