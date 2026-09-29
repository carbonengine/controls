#ifdef WIN32
#include "InputHandlerWin.h"

#include <algorithm>
#include <iterator>
#include <Windows.h>
#include <gameinput_v3.h>

#include "../ControlManager.h"

using namespace GameInput::v3;

namespace
{
// Without the gamepad info block the layout is unknown, so the gamepad view cannot be used
// as the source of button identity.
const GameInputGamepadInfo* GetUsableGamepadInfo( const GameInputDeviceInfo* info )
{
	if( !info || ( info->supportedInput & GameInputKind::GameInputKindGamepad ) == 0 )
	{
		return nullptr;
	}
	return info->gamepadInfo;
}
}

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
		std::lock_guard<std::mutex> lock( m_deviceMutex );
		for( auto& slot : m_deviceSlots )
		{
			if( slot->readCallbackToken != 0 )
			{
				m_gameInput->UnregisterCallback( slot->readCallbackToken );
				slot->readCallbackToken = 0;
			}
			slot->device = nullptr;
		}
		m_deviceSlots.clear();
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
		if( hr == E_NOINTERFACE )
		{
			CCP_LOGWARN( "InputHandlerWin: GameInput runtime not detected (error 0x%08X). Please install GameInputRedist to enable gamepad support", hr );
		}
		else
		{
			CCP_LOGERR( "InputHandlerWin: GameInputCreate failed (0x%08X)", hr );
		}
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
		// Resolved before taking the lock: it only reads the device's layout, and installing it
		// is then a single assignment the reading callback can never observe half-done.
		auto plan = ResolvePlan( device, identifier );

		bool isNewDevice = false;
		{
			// Find-or-create under one lock, otherwise two connect notifications for the same
			// device can both miss and both append a slot.
			std::lock_guard<std::mutex> lock( self->m_deviceMutex );
			auto slot = self->FindSlotLocked( identifier.deviceID );
			if( !slot )
			{
				slot = std::make_shared<DeviceSlot>();
				self->m_deviceSlots.push_back( slot );
				isNewDevice = true;
			}

			// A device that disconnects and reconnects keeps its slot; only the plan is rebuilt.
			if( !slot->device )
			{
				slot->device = device;
			}
			slot->pendingRemoval = false;
			slot->identifier = identifier;
			slot->plan = std::move( plan );
		}

		CCP_LOGNOTICE( "InputHandlerWin: Device '%s' %s", identifier.name.c_str(), isNewDevice ? "connected" : "reconnected" );
		if( self->m_deviceAddedCallback )
		{
			self->m_deviceAddedCallback( identifier );
		}
	}
	else if( !isConnected && wasConnected )
	{
		// Mark the matching slot for removal on next Update()
		DeviceEnums::DeviceIdentifier removed;
		bool found = false;
		{
			std::lock_guard<std::mutex> lock( self->m_deviceMutex );
			if( auto slot = self->FindSlotLocked( identifier.deviceID ) )
			{
				slot->pendingRemoval = true;
				self->m_devicesRemoved = true;
				removed = slot->identifier;
				found = true;
			}
		}

		if( found )
		{
			CCP_LOGNOTICE( "InputHandlerWin: Device '%s' final disconnected", removed.name.c_str() );
			if( self->m_deviceRemovedCallback )
			{
				self->m_deviceRemovedCallback( removed );
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
	// Resolve the slot first: ReadDeviceState needs the plan it was configured with.
	// GetDeviceSlot takes m_deviceMutex, so it must not be called while that lock is held.
	auto foundSlot = self->GetDeviceSlot( device );
	if( !foundSlot )
	{
		return;
	}

	// The plan is immutable once installed, so holding a reference to it is enough; a
	// reconnect swapping in a replacement cannot disturb this walk.
	std::shared_ptr<const ExtractionPlan> plan;
	{
		std::lock_guard<std::mutex> lock( self->m_deviceMutex );
		plan = foundSlot->plan;
	}

	auto state = ReadDeviceState( ownedReading, *plan );
	if( !state )
	{
		// An unreadable reading tells us nothing; publishing an empty snapshot would look
		// like every element on the device had just gone neutral.
		return;
	}

	{
		std::lock_guard<std::mutex> lock( self->m_readingMutex );
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
		if( foundDevice->readCallbackToken != 0 )
		{
			return;
		}

		// GameInput only reports readings when something changes, so until the first input arrives
		// events would evaluate against a snapshot that has no entry for any of their elements.
		std::shared_ptr<const ExtractionPlan> plan;
		{
			std::lock_guard<std::mutex> lock( m_deviceMutex );
			plan = foundDevice->plan;
		}

		CComPtr<IGameInputReading> initialReading;
		if( SUCCEEDED( m_gameInput->GetCurrentReading( SUPPORTED_INPUTS, foundDevice->device, &initialReading ) ) )
		{
			if( auto initialState = ReadDeviceState( initialReading, *plan ) )
			{
				std::lock_guard<std::mutex> lock( m_readingMutex );
				foundDevice->accumulatedStates.push_back( std::move( *initialState ) );
			}
		}

		// Filtered to this device: an unfiltered callback fires for every device's readings, so
		// one registration per active device would accumulate each reading once per active device.
		auto hr = m_gameInput->RegisterReadingCallback(
			foundDevice->device,
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
			std::lock_guard<std::mutex> lock( m_deviceMutex );
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
		std::lock_guard<std::mutex> lock( m_deviceMutex );
		for( auto& slot : m_deviceSlots )
		{
			if( !slot->pendingRemoval || !slot->device )
			{
				continue;
			}
			slot->device = nullptr;
			slot->pendingRemoval = false;
			if( slot->readCallbackToken != 0 )
			{
				m_gameInput->UnregisterCallback( slot->readCallbackToken );
				slot->readCallbackToken = 0;
			}
			CCP_LOGNOTICE( "InputHandlerWin: Device '%s' final removal", slot->identifier.name.c_str() );
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
		std::lock_guard<std::mutex> lock( m_readingMutex );
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

std::shared_ptr<const InputHandlerWin::ExtractionPlan> InputHandlerWin::ResolvePlan( IGameInputDevice* device, DeviceEnums::DeviceIdentifier& identifier )
{
	auto plan = std::make_shared<ExtractionPlan>();

	identifier.buttonElements.clear();
	identifier.axisElements.clear();
	identifier.switchElements.clear();

	const GameInputDeviceInfo* info = nullptr;
	if( device )
	{
		device->GetDeviceInfo( &info );
	}
	if( info == nullptr )
	{
		return plan;
	}

	const GameInputGamepadInfo* gamepadInfo = GetUsableGamepadInfo( info );
	const GameInputControllerInfo* controllerInfo = info->controllerInfo;

	plan->buttonSources = ButtonHandling::GetButtonSources( controllerInfo, gamepadInfo );
	plan->axisSources = AxisHandling::GetAxisSources( controllerInfo, gamepadInfo );
	plan->switchSources = SwitchHandling::GetSwitchSources( controllerInfo );

	// Decided once here so ReadDeviceState never has to work out which views to fetch.
	for( const auto& source : plan->buttonSources )
	{
		plan->needsGamepadState |= source.kind == ButtonHandling::ButtonSource::Kind::GamepadMask;
		plan->needsRawButtons |= source.kind == ButtonHandling::ButtonSource::Kind::RawIndex;
		identifier.buttonElements.push_back( source.key );
	}
	for( const auto& source : plan->axisSources )
	{
		plan->needsGamepadState |= source.kind == AxisHandling::AxisSource::Kind::GamepadField;
		plan->needsRawAxes |= source.kind == AxisHandling::AxisSource::Kind::RawIndex;
		identifier.axisElements.push_back( source.key );
	}
	for( const auto& source : plan->switchSources )
	{
		identifier.switchElements.push_back( source.key );
	}
	plan->needsRawSwitches = !plan->switchSources.empty();

	return plan;
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
std::optional<Events::State> InputHandlerWin::ReadDeviceState( IGameInputReading* reading, const ExtractionPlan& plan )
{
	// Pure execution of the extraction plan. Every layout question was answered once in
	// ResolvePlan, so nothing here inspects device capabilities or element descriptors.
	if( !reading )
	{
		return std::nullopt;
	}

	Events::State state = {};
	state.timestamp = Events::GetTimestamp();

	// retrieve the needed gamepad and controller information
	GameInputGamepadState gamepadState = {};
	if( plan.needsGamepadState )
	{
		if( !reading->GetGamepadState( &gamepadState ) )
		{
			return std::nullopt;
		}
	}

	const uint32_t buttonCount = plan.needsRawButtons ? reading->GetControllerButtonCount() : 0;
	auto rawButtons = std::make_unique<bool[]>( buttonCount );
	if( plan.needsRawButtons )
	{
		reading->GetControllerButtonState( buttonCount, rawButtons.get() );
	}

	const uint32_t axisCount = plan.needsRawAxes ? reading->GetControllerAxisCount() : 0;
	auto rawAxes = std::make_unique<float[]>( axisCount );
	if( plan.needsRawAxes )
	{
		reading->GetControllerAxisState( axisCount, rawAxes.get() );
	}

	const uint32_t switchCount = plan.needsRawSwitches ? reading->GetControllerSwitchCount() : 0;
	auto rawSwitches = std::make_unique<GameInputSwitchPosition[]>( switchCount );
	if( switchCount > 0 )
	{
		reading->GetControllerSwitchState( switchCount, rawSwitches.get() );
	}

	for( const auto& source : plan.buttonSources )
	{
		state.buttons.insert( { source.key, ButtonHandling::Handle( source, gamepadState, rawButtons.get(), buttonCount ) } );
	}
	for( const auto& source : plan.axisSources )
	{
		state.axis.insert( { source.key, AxisHandling::Handle( source, gamepadState, rawAxes.get(), axisCount ) } );
	}
	for( const auto& source : plan.switchSources )
	{
		state.switches.insert( { source.key, SwitchHandling::Handle( source, rawSwitches.get(), switchCount ) } );
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

		std::lock_guard<std::mutex> lock( m_deviceMutex );
		if( deviceSlot->device )
		{
			deviceSlot->device->SetRumbleState( &rumbleParams );
		}
	}
}

std::shared_ptr<InputHandlerWin::DeviceSlot> InputHandlerWin::FindSlotLocked( BlueSharedString deviceID )
{
	auto it = std::find_if( m_deviceSlots.begin(), m_deviceSlots.end(), [deviceID]( const std::shared_ptr<DeviceSlot>& slot ) {
		return slot->identifier.deviceID == deviceID;
	} );
	return it != m_deviceSlots.end() ? *it : nullptr;
}

std::shared_ptr<InputHandlerWin::DeviceSlot> InputHandlerWin::GetDeviceSlot( BlueSharedString deviceID )
{
	std::lock_guard<std::mutex> lock( m_deviceMutex );

	auto it = std::find_if( m_deviceSlots.begin(), m_deviceSlots.end(), [deviceID]( const std::shared_ptr<DeviceSlot>& slot ) {
		return slot->device && slot->identifier.deviceID == deviceID;
	} );
	return it != m_deviceSlots.end() ? *it : nullptr;
}

std::shared_ptr<InputHandlerWin::DeviceSlot> InputHandlerWin::GetDeviceSlot( CComPtr<IGameInputDevice> device )
{
	std::lock_guard<std::mutex> lock( m_deviceMutex );

	auto it = std::find_if( m_deviceSlots.begin(), m_deviceSlots.end(), [device]( const std::shared_ptr<DeviceSlot>& slot ) {
		return slot->device == device && !slot->pendingRemoval;
	} );
	return it != m_deviceSlots.end() ? *it : nullptr;
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
