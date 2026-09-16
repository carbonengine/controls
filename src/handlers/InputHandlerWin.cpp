#ifdef WIN32
#include "InputHandlerWin.h"

#include <algorithm>
#include <iterator>
#include <Windows.h> 
#include <gameinput_v3.h>

#include "../ControlManager.h"

using namespace GameInput::v3;

InputHandlerWin::InputHandlerWin()
{
}

InputHandlerWin::~InputHandlerWin()
{
	if( !m_initialized )
	{
		return;
	}

	// Unregister the device callback before releasing devices
	if( m_gameInput )
	{
		if( m_deviceCallbackToken != 0 )
		{
			m_gameInput->UnregisterCallback( m_deviceCallbackToken );
			m_deviceCallbackToken = 0;
		}
	}

	{
		std::unique_lock<std::shared_mutex> lock( m_deviceMutex );
		for( auto& slot : m_deviceSlots )
		{
			m_gameInput->UnregisterCallback( slot->readCallbackToken );
			slot->device = nullptr;
		}
	}

	if( m_gameInput )
	{
		m_gameInput = nullptr;
	}

	CCP_LOGNOTICE( "InputHandlerWin: Shut down" );
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
		m_gameInput = nullptr;
		return false;
	}

	m_initialized = true;
	CCP_LOGNOTICE( "InputHandlerWin: Initialized successfully" );
	return true;
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

	auto identifier = self->GetIdentifier( device );
	if( isConnected && !wasConnected )
	{
		auto slot = self->GetDeviceSlot( identifier.deviceID );
		if( slot )
		{
			// This can happen if a device disconnects and reconnects again, no need to create a new slot for it, just update the existing one
			if( !slot->device )
			{
				slot->device = device;
			}
			slot->pendingRemoval = false;
			ConfigureDeviceSlot( *slot, device );
			identifier = slot->identifier;
			CCP_LOGNOTICE( "InputHandlerWin: Device '%s' reconnected", identifier.name.c_str() );
		}
		else
		{
			auto newSlot = std::make_unique<DeviceSlot>();
			newSlot->device = device;
			newSlot->pendingRemoval = false;
			newSlot->identifier = identifier;
			ConfigureDeviceSlot( *newSlot, device );
			// ConfigureDeviceSlot rewrites the element lists to match what will
			// actually be published, so the callback must see the configured copy.
			identifier = newSlot->identifier;

			std::unique_lock<std::shared_mutex> lock( self->m_deviceMutex );
			self->m_deviceSlots.push_back( std::move( newSlot ) );
			CCP_LOGNOTICE( "InputHandlerWin: Device '%s' connected", identifier.name.c_str() );
		}
		if( self->m_deviceAddedCallback )
		{
			self->m_deviceAddedCallback( identifier );
		}
	}
	else if( !isConnected && wasConnected )
	{
		// Mark the matching slot for removal on next Update()
		auto slot = self->GetDeviceSlot( identifier.deviceID );
		if( slot )
		{
			CCP_LOGNOTICE( "InputHandlerWin: Device '%s' final disconnected", slot->identifier.name.c_str() );

			slot->pendingRemoval = true;
			self->m_devicesRemoved = true;
			if( self->m_deviceRemovedCallback )
			{
				self->m_deviceRemovedCallback( slot->identifier );
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

	// Take ownership of the reading reference so it is released on every exit path
	CComPtr<IGameInputReading> ownedReading;
	ownedReading.Attach( reading );

	// find the device id. GetDevice hands back a reference, so it needs releasing too.
	CComPtr<IGameInputDevice> device;
	ownedReading->GetDevice( &device );

	if( !device )
	{
		return;
	}
	// Resolve the slot first: ReadDeviceState needs its gamepad capability and axis roles.
	// GetDeviceSlot takes m_deviceMutex, so it must not be called while that lock is held.
	auto foundSlot = self->GetDeviceSlot( device );
	if( !foundSlot )
	{
		return;
	}

	auto state = self->ReadDeviceState( ownedReading, *foundSlot );
	if( !state )
	{
		// An unreadable reading tells us nothing; publishing an empty snapshot would look
		// like every element on the device had just gone neutral.
		return;
	}

	{
		std::unique_lock<std::shared_mutex> lock( self->m_readingMutex );
		foundSlot->accumulatedStates.push_back( std::move( *state ) );
	}
}

void InputHandlerWin::SetDeviceActivation( BlueSharedString deviceID, bool activate )
{
	auto foundDevice = GetDeviceSlot( deviceID );
	if( !foundDevice )
	{
		CCP_LOGERR( "InputHandlerWin: Could not find device with ID '%s' to set activation to %d", deviceID.c_str(), activate );
		return;
	}

	if( activate )
	{
		auto hr = m_gameInput->RegisterReadingCallback(
			nullptr,
			SUPPORTED_INPUTS,
			this,
			OnDeviceRead,
			&foundDevice->readCallbackToken );
		if( !SUCCEEDED( hr ) )
		{
			CCP_LOGERR( "InputHandlerWin: Failed to register reading callback for device '%s' (0x%08X)", foundDevice->identifier.name.c_str(), hr );
			foundDevice->readCallbackToken = 0;
		}
	}
	else
	{
		if( foundDevice->device )
		{
			GameInputRumbleParams zeroed = {};
			std::unique_lock<std::shared_mutex> lock( m_deviceMutex );
			foundDevice->device->SetRumbleState( &zeroed );
		}
		if( foundDevice->readCallbackToken != 0 )
		{
			m_gameInput->UnregisterCallback( foundDevice->readCallbackToken );
			foundDevice->readCallbackToken = 0;
		}
	}
}

std::vector<Events::State> InputHandlerWin::Update( BlueSharedString deviceID )
{
	if( !m_initialized )
	{
		return {};
	}

	// need to remove devices here, but not in the callback
	if( m_devicesRemoved )
	{
		std::unique_lock<std::shared_mutex> lock( m_deviceMutex );
		for( auto& slot : m_deviceSlots )
		{
			if( slot->pendingRemoval )
			{
				if( slot->device )
				{
					slot->device = nullptr;
					slot->pendingRemoval = false;
					if( slot->readCallbackToken != 0 )
					{
						m_gameInput->UnregisterCallback( slot->readCallbackToken );
						slot->readCallbackToken = 0;
					}
					CCP_LOGNOTICE( "InputHandlerWin: Device '%s' final removal", slot->identifier.name.c_str() );
				}
			}
		}
		m_devicesRemoved = false;
	}

	std::vector<Events::State> statesForDevice = {};
	{
		auto deviceSlot = GetDeviceSlot( deviceID );
		if( !deviceSlot )
		{
			CCP_LOGERR( "InputHandlerWin: Could not find device with ID '%s' to update", deviceID.c_str() );
			return statesForDevice;
		}
		std::unique_lock<std::shared_mutex> lock( m_readingMutex );
		std::swap( statesForDevice, deviceSlot->accumulatedStates );
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

	identifier.deviceID = InputMapping::GetDeviceIDAsString( info->deviceId );

	char vid[16];
	snprintf( vid, sizeof( vid ), "%04X", info->vendorId );

	char pid[16];
	snprintf( pid, sizeof( pid ), "%04X", info->productId );

	identifier.vendorID = BlueSharedString( vid );
	identifier.productID = BlueSharedString( pid );
	identifier.family = InputMapping::GetDeviceFamily( info->vendorId );

	if( info->deviceFamily == GameInputDeviceFamily::GameInputFamilyHid )
	{
		// check the registry for the device name, using the vendor/product ID as a key
		auto registryName = InputMapping::GetStringValueFromHKLM(
			"SYSTEM\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Joystick\\OEM\\VID_" + std::string( vid ) + "&PID_" + std::string( pid ),
			"OEMName" );
		if( !registryName.empty() )
		{
			identifier.name = BlueSharedString( registryName.c_str() );
		}
	}

	if( identifier.name.empty() && info->displayName )
	{
		identifier.name = BlueSharedString( static_cast<const char*>( info->displayName ) );
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

	return identifier;
}

bool InputHandlerWin::SupportsGamepad( IGameInputDevice* device )
{
	if( !device )
	{
		return false;
	}

	const GameInputDeviceInfo* info = nullptr;
	device->GetDeviceInfo( &info );
	if( info == nullptr )
	{
		return false;
	}

	// Without the gamepad info block the layout is unknown, so the gamepad view cannot be
	// used as the source of button identity.
	return ( info->supportedInput & GameInputKind::GameInputKindGamepad ) != 0 && info->gamepadInfo != nullptr;
}

void InputHandlerWin::ConfigureDeviceSlot( DeviceSlot& slot, IGameInputDevice* device )
{
	slot.buttonSources.clear();
	slot.axisSources.clear();
	slot.switchSources.clear();
	// The published element lists are rebuilt below. On a reconnect the slot still carries
	// the previous run's entries, which would otherwise be appended to instead of replaced.
	slot.identifier.buttonElements.clear();
	slot.identifier.axisElements.clear();
	slot.identifier.switchElements.clear();
	slot.needsGamepadState = false;
	slot.needsRawButtons = false;
	slot.needsRawAxes = false;
	slot.needsRawSwitches = false;
	slot.supportsGamepad = SupportsGamepad( device );

	if( !device )
	{
		return;
	}

	const GameInputDeviceInfo* info = nullptr;
	device->GetDeviceInfo( &info );
	if( info == nullptr )
	{
		return;
	}

	const GameInputGamepadInfo* gamepadInfo = slot.supportsGamepad ? info->gamepadInfo : nullptr;
	const GameInputControllerInfo* controllerInfo = info->controllerInfo;

	// --- Buttons -----------------------------------------------------------
	slot.buttonSources = ButtonHandling::GetButtonSources( controllerInfo, gamepadInfo );

	// --- Axes --------------------------------------------------------------
	slot.axisSources = AxisHandling::GetAxisSources( controllerInfo, gamepadInfo );

	// --- Switches -----------------------------------------------------------
	slot.switchSources = SwitchHandling::GetSwitchSources( controllerInfo );

	// --- Reading requirements ----------------------------------------------
	// Decided once here so ReadDeviceState never has to work out which views to
	// fetch for a given reading.
	for( const auto& source : slot.buttonSources )
	{
		slot.needsGamepadState |= source.kind == ButtonHandling::ButtonSource::Kind::GamepadMask;
		slot.needsRawButtons |= source.kind == ButtonHandling::ButtonSource::Kind::RawIndex;
		slot.identifier.buttonElements.push_back( source.descriptor );
	}
	for( const auto& source : slot.axisSources )
	{
		slot.needsGamepadState |= source.kind == AxisHandling::AxisSource::Kind::GamepadField;
		slot.needsRawAxes |= source.kind == AxisHandling::AxisSource::Kind::RawIndex;
		slot.identifier.axisElements.push_back( source.descriptor );
	}
	for( const auto& sourceIndex : slot.switchSources )
	{
		slot.identifier.switchElements.push_back( DeviceEnums::InputElementDescriptor::DPad );
	}
	slot.needsRawSwitches = !slot.switchSources.empty();
}

void InputHandlerWin::RegisterForDeviceAdded( DeviceChangedCallback callback )
{
	m_deviceAddedCallback = callback;
}

void InputHandlerWin::RegisterForDeviceRemoved( DeviceChangedCallback callback )
{
	m_deviceRemovedCallback = callback;
}

// ---------------------------------------------------------------------------
// ReadDeviceState - get the most recent reading for a device
// ---------------------------------------------------------------------------
std::optional<Events::State> InputHandlerWin::ReadDeviceState( IGameInputReading* reading, const DeviceSlot& slot )
{
	// Pure execution of the slot's extraction plan. Every layout question was
	// answered once in ConfigureDeviceSlot, so nothing here inspects device
	// capabilities or element descriptors.
	if( !reading )
	{
		return std::nullopt;
	}

	Events::State state = {};
	state.timestamp = Events::GetTimestamp();

	// retrieve the needed gamepad and controller information
	GameInputGamepadState gamepadState = {};
	if( slot.needsGamepadState )
	{
		if( !reading->GetGamepadState( &gamepadState ) )
		{
			return std::nullopt;
		}
	}

	const uint32_t buttonCount = slot.needsRawButtons ? reading->GetControllerButtonCount() : 0;
	auto rawButtons = std::make_unique<bool[]>( buttonCount );
	if( slot.needsRawButtons )
	{
		reading->GetControllerButtonState( buttonCount, rawButtons.get() );
	}

	const uint32_t axisCount = slot.needsRawAxes ? reading->GetControllerAxisCount() : 0;
	auto rawAxes = std::make_unique<float[]>( axisCount );
	if( slot.needsRawAxes )
	{
		reading->GetControllerAxisState( axisCount, rawAxes.get() );
	}

	const uint32_t switchCount = slot.needsRawSwitches ? reading->GetControllerSwitchCount() : 0;
	auto rawSwitches = std::make_unique<GameInputSwitchPosition[]>( switchCount );
	if( switchCount > 0 )
	{
		reading->GetControllerSwitchState( switchCount, rawSwitches.get() );
	}

	// handle the buttons
	for( const auto& source : slot.buttonSources )
	{
		// Keyed by elementIndex (published index), matching InputDevice's element identifiers -
		// rawIndex is only where the bit/array entry is sampled from, not how it is identified.
		state.buttons.insert( { static_cast<uint32_t>(source.descriptor) + source.elementIndex, ButtonHandling::Handle( source, gamepadState, rawButtons.get(), buttonCount ) } );
	}

	// handle the axes
	for( const auto& source : slot.axisSources )
	{
		state.axis.insert( { static_cast<uint32_t>(source.descriptor) + source.elementIndex, AxisHandling::Handle( source, gamepadState, rawAxes.get(), axisCount ) } );
	}

	// handle the switches
	for( const auto& sourceIndex : slot.switchSources )
	{
		// Matches the DPad descriptor lookup key ControllerSwitchInputEvent computes.
		state.switches.insert( { static_cast<uint32_t>( DeviceEnums::InputElementDescriptor::DPad ) + sourceIndex, SwitchHandling::Handle( sourceIndex, rawSwitches.get(), switchCount ) } );
	}

	return state;
}

void InputHandlerWin::Rumble( BlueSharedString deviceID, Events::Rumble rumble )
{
	auto deviceSlot = GetDeviceSlot( deviceID );

	if( deviceSlot )
	{
		GameInputRumbleParams rumbleParams = {};
		rumbleParams.highFrequency = rumble.highFrequency;
		rumbleParams.lowFrequency = rumble.lowFrequency;
		rumbleParams.leftTrigger = rumble.leftTrigger;
		rumbleParams.rightTrigger = rumble.rightTrigger;

		std::unique_lock<std::shared_mutex> lock( m_deviceMutex );
		deviceSlot->device->SetRumbleState( &rumbleParams );
	}
}

InputHandlerWin::DeviceSlot* InputHandlerWin::GetDeviceSlot( BlueSharedString deviceID )
{
	std::unique_lock<std::shared_mutex> lock( m_deviceMutex );

	auto it = std::find_if( m_deviceSlots.begin(), m_deviceSlots.end(), [deviceID]( const std::unique_ptr<DeviceSlot>& slot ) {
		return slot->device && slot->identifier.deviceID == deviceID;
	} );
	if( it != m_deviceSlots.end() )
	{
		return it->get();
	}

	return nullptr;
}

InputHandlerWin::DeviceSlot* InputHandlerWin::GetDeviceSlot( CComPtr<IGameInputDevice> device )
{
	std::unique_lock<std::shared_mutex> lock( m_deviceMutex );

	auto it = std::find_if( m_deviceSlots.begin(), m_deviceSlots.end(), [device]( const std::unique_ptr<DeviceSlot>& slot ) {
		return slot->device == device && !slot->pendingRemoval;
	} );
	if( it != m_deviceSlots.end() )
	{
		return it->get();
	}

	return nullptr;
}

void InputHandlerWin::SetBackgroundEventsEnabled( bool enabled )
{
	if( m_gameInput )
	{
		if( enabled )
		{
			m_gameInput->SetFocusPolicy( GameInputFocusPolicy::GameInputEnableBackgroundInput );
		}
		else
		{
			m_gameInput->SetFocusPolicy( GameInputFocusPolicy::GameInputDefaultFocusPolicy );
		}
	}
}
#endif // WIN32
