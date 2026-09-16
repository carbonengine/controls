#ifdef __APPLE__
#include "InputHandlerApple.h"

#import <Foundation/Foundation.h>
#import <GameController/GameController.h>
#import <CoreHaptics/CoreHaptics.h>

#include <algorithm>
#include <sstream>
#include <string>

#include "../ControlManager.h"

namespace
{
// Locality string for a given HapticsChannelIndex; the ordering here MUST match the enum.
API_AVAILABLE( macos( 11.0 ) )
GCHapticsLocality LocalityForChannel( int channel )
{
	switch( channel )
	{
	case 0: return GCHapticsLocalityLeftHandle;
	case 1: return GCHapticsLocalityRightHandle;
	case 2: return GCHapticsLocalityLeftTrigger;
	case 3: return GCHapticsLocalityRightTrigger;
	}
	return GCHapticsLocalityDefault;
}

// Sharpness is fixed per channel: handles feel rumbly (low sharpness), triggers snappy (high sharpness).
float SharpnessForChannel( int channel )
{
	switch( channel )
	{
	case 0:
	case 1: return 0.3f;
	case 2:
	case 3: return 0.7f;
	}
	return 0.5f;
}

// Clears the corresponding rumble-capacity flag when a channel's engine or player fails to come up.
void ClearCapacityForChannel( DeviceEnums::RumbleCapacity& capacity, int channel )
{
	switch( channel )
	{
	case 0: capacity.hasLowFrequencyRumble = false; break;
	case 1: capacity.hasHighFrequencyRumble = false; break;
	case 2: capacity.hasLeftTriggerRumble = false; break;
	case 3: capacity.hasRightTriggerRumble = false; break;
	}
	if( capacity.rumbleMotorCount > 0 )
	{
		capacity.rumbleMotorCount -= 1;
	}
}

// Build a DeviceIdentifier for a freshly-connected controller.
DeviceEnums::DeviceIdentifier BuildIdentifier(
	GCController* controller,
	uint64_t counter,
	const std::vector<ButtonHandling::ButtonSource>& buttonSources,
	const std::vector<AxisHandling::AxisSource>& axisSources,
	const std::vector<SwitchHandling::SwitchSource>& switchSources )
{
	DeviceEnums::DeviceIdentifier identifier;

	NSString* vendorName = controller.vendorName;
	NSString* productCategory = controller.productCategory;

	NSString* displayName = vendorName ?: productCategory;
	if( displayName == nil )
	{
		displayName = @"Game Controller";
	}
	identifier.name = BlueSharedString( [displayName UTF8String] );

	const std::string vendorSan = InputMapping::SanitizeForDeviceID( vendorName );
	const std::string productSan = InputMapping::SanitizeForDeviceID( productCategory );
	std::ostringstream idStream;
	idStream << ( vendorSan.empty() ? "Controller" : vendorSan );
	if( !productSan.empty() )
	{
		idStream << '-' << productSan;
	}
	idStream << '-' << counter;
	identifier.deviceID = BlueSharedString( idStream.str() );

	if( productCategory != nil )
	{
		identifier.productID = BlueSharedString( [productCategory UTF8String] );
	}

	identifier.family = InputMapping::GetDeviceFamily( controller );

	for( const auto& source : buttonSources )
	{
		identifier.buttonElements.push_back( source.descriptor );
	}
	for( const auto& source : axisSources )
	{
		identifier.axisElements.push_back( source.descriptor );
	}
	for( const auto& source : switchSources )
	{
		identifier.switchElements.push_back( source.descriptor );
	}

	identifier.rumbleCapacity = DeviceEnums::RumbleCapacity{};
	// GCDeviceHaptics + CoreHaptics are 11.0+; leave capacity zeroed on older systems or controllers without haptics.
	if( @available( macOS 11.0, * ) )
	{
		GCDeviceHaptics* haptics = controller.haptics;
		if( haptics == nil )
		{
			CCP_LOGNOTICE( "InputHandlerApple: '%s' reports no GCDeviceHaptics support",
				identifier.name.c_str() );
		}
		else
		{
			NSSet<GCHapticsLocality>* localities = haptics.supportedLocalities;
			NSMutableString* dump = [NSMutableString stringWithString:@""];
			for( GCHapticsLocality loc in localities )
			{
				if( dump.length > 0 )
				{
					[dump appendString:@", "];
				}
				[dump appendString:loc];
			}
			CCP_LOGNOTICE( "InputHandlerApple: '%s' haptics localities: [%s]",
				identifier.name.c_str(),
				dump.UTF8String ? dump.UTF8String : "" );

			const bool hasLow = [localities containsObject:GCHapticsLocalityLeftHandle];
			const bool hasHigh = [localities containsObject:GCHapticsLocalityRightHandle];
			const bool hasLeftTrig = [localities containsObject:GCHapticsLocalityLeftTrigger];
			const bool hasRightTrig = [localities containsObject:GCHapticsLocalityRightTrigger];

			identifier.rumbleCapacity.hasLowFrequencyRumble = hasLow;
			identifier.rumbleCapacity.hasHighFrequencyRumble = hasHigh;
			identifier.rumbleCapacity.hasLeftTriggerRumble = hasLeftTrig;
			identifier.rumbleCapacity.hasRightTriggerRumble = hasRightTrig;
			identifier.rumbleCapacity.rumbleMotorCount = hasLow + hasHigh + hasLeftTrig + hasRightTrig;
		}
	}

	return identifier;
}
}

InputHandlerApple::InputHandlerApple() = default;

InputHandlerApple::~InputHandlerApple()
{
	if( !m_initialized )
	{
		return;
	}

	NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
	if( m_connectObserver != nil )
	{
		[center removeObserver:m_connectObserver];
		m_connectObserver = nil;
	}
	if( m_disconnectObserver != nil )
	{
		[center removeObserver:m_disconnectObserver];
		m_disconnectObserver = nil;
	}

	{
		std::unique_lock<std::mutex> lock( m_deviceMutex );
		for( auto& slot : m_deviceSlots )
		{
			ShutdownHapticsForSlot( *slot );
			if( slot->controller != nil )
			{
				slot->controller.physicalInputProfile.valueDidChangeHandler = nil;
			}
			slot->controller = nil;
            slot->buttonSources.clear();
            slot->axisSources.clear();
            slot->switchSources.clear();
		}
		m_deviceSlots.clear();
	}

	m_handlerQueue = nil;

	CCP_LOGNOTICE( "InputHandlerApple: Shut down" );
}

bool InputHandlerApple::Initialize()
{
	if( m_initialized )
	{
		return true;
	}

	NSNotificationCenter* center = [NSNotificationCenter defaultCenter];

	m_handlerQueue = dispatch_queue_create( "com.ccp.carbon-controls.InputHandlerApple", DISPATCH_QUEUE_SERIAL );
	NSOperationQueue* callbackQueue = [[NSOperationQueue alloc] init];
	callbackQueue.underlyingQueue = m_handlerQueue;

	m_connectObserver = [center addObserverForName:GCControllerDidConnectNotification
											object:nil
											 queue:callbackQueue
										usingBlock:^( NSNotification* note ) {
		GCController* controller = (GCController*)note.object;
		this->HandleControllerConnected( controller );
	}];

	m_disconnectObserver = [center addObserverForName:GCControllerDidDisconnectNotification
											   object:nil
												queue:callbackQueue
										   usingBlock:^( NSNotification* note ) {
		GCController* controller = (GCController*)note.object;
		this->HandleControllerDisconnected( controller );
	}];

	m_initialized = true;

	for( GCController* controller in [GCController controllers] )
	{
		HandleControllerConnected( controller );
	}

	CCP_LOGNOTICE( "InputHandlerApple: Initialized successfully" );
	return true;
}

void InputHandlerApple::RegisterForDeviceAdded( DeviceChangedCallback callback )
{
	m_deviceAddedCallback = callback;
}

void InputHandlerApple::RegisterForDeviceRemoved( DeviceChangedCallback callback )
{
	m_deviceRemovedCallback = callback;
}

void InputHandlerApple::HandleControllerConnected( GCController* controller )
{
	if( controller == nil )
	{
		return;
	}

	// Redirect this controller's own callbacks (physicalInputProfile.valueDidChangeHandler, etc.) off the
	// main queue too, so activation doesn't silently depend on the host app pumping the main run loop.
	controller.handlerQueue = m_handlerQueue;

	auto buttonSources = ButtonHandling::GetButtonSources( controller );
	auto axisSources = AxisHandling::GetAxisSources( controller );
	auto switchSources = SwitchHandling::GetSwitchSources( controller );

	const uint64_t counter = m_deviceCounter.fetch_add( 1 );
	DeviceEnums::DeviceIdentifier identifier = BuildIdentifier( controller, counter, buttonSources, axisSources, switchSources );

	bool shouldFireAdded = false;
	DeviceEnums::DeviceIdentifier addedIdentifier;

	{
		std::unique_lock<std::mutex> lock( m_deviceMutex );
		auto existing = std::find_if( m_deviceSlots.begin(), m_deviceSlots.end(),
			[controller]( const std::unique_ptr<DeviceSlot>& slot ) {
				return slot->controller == controller;
			} );

		if( existing != m_deviceSlots.end() )
		{
			( *existing )->pendingRemoval = false;
			CCP_LOGNOTICE( "InputHandlerApple: Device '%s' reconnected", ( *existing )->identifier.name.c_str() );
		}
		else
        {
			// A controller reconnecting over a different transport (e.g. a Bluetooth-paired controller
			// plugged in over USB) is handed to us as a brand-new GCController instance, so the pointer
			// check above never matches it - the framework exposes no persistent hardware identifier to
			// correlate the two instances. Best-effort recognize it as the same physical unit if a slot
			// disconnected very recently (still pendingRemoval - i.e. Update() hasn't finalized/erased it
			// yet) has the same product category and element layout, so its identity/deviceID carries over
			// instead of showing up as a second device. This can't be exact (two identical controllers
			// swapping at the exact same instant would be indistinguishable), and it never delays a real
			// disconnect: once Update() finalizes a slot it's gone and a later reconnect is just a new device.
			auto revived = std::find_if( m_deviceSlots.begin(), m_deviceSlots.end(),
				[&identifier]( const std::unique_ptr<DeviceSlot>& slot ) {
					return slot->pendingRemoval
						&& slot->identifier.productID == identifier.productID
						&& slot->identifier.buttonElements == identifier.buttonElements
						&& slot->identifier.axisElements == identifier.axisElements
						&& slot->identifier.switchElements == identifier.switchElements;
				} );

			if( revived != m_deviceSlots.end() )
			{
				DeviceSlot* rawSlot = revived->get();
				rawSlot->controller = controller;
				rawSlot->buttonSources = std::move( buttonSources );
				rawSlot->axisSources = std::move( axisSources );
				rawSlot->switchSources = std::move( switchSources );
				rawSlot->pendingRemoval = false;

				// Preserve the original identity so anything upstream keyed on deviceID keeps working.
				identifier.deviceID = rawSlot->identifier.deviceID;
				identifier.name = rawSlot->identifier.name;
				rawSlot->identifier = identifier;

				if( rawSlot->identifier.rumbleCapacity.rumbleMotorCount > 0 )
				{
					InitializeHapticsForSlot( *rawSlot );
				}
				// The old handler died with the old GCController instance; if the caller had this device
				// active, resume delivering events on the new instance without requiring a re-activation call.
				if( rawSlot->active )
				{
					ActivateSlotHandler( *rawSlot );
				}
				CCP_LOGNOTICE( "InputHandlerApple: Device '%s' reconnected on a different transport, keeping its identity",
					rawSlot->identifier.name.c_str() );
			}
			else
			{
				auto slot = std::make_unique<DeviceSlot>();
				slot->controller = controller;
				slot->buttonSources = std::move( buttonSources );
				slot->axisSources = std::move( axisSources );
				slot->switchSources = std::move( switchSources );
				slot->identifier = identifier;

				DeviceSlot* rawSlot = slot.get();
				m_deviceSlots.push_back( std::move( slot ) );
				if( rawSlot->identifier.rumbleCapacity.rumbleMotorCount > 0 )
				{
					InitializeHapticsForSlot( *rawSlot );
				}
				addedIdentifier = rawSlot->identifier;
				shouldFireAdded = true;
				CCP_LOGNOTICE( "InputHandlerApple: Device '%s' connected", rawSlot->identifier.name.c_str() );
			}
		}
	}

	if( shouldFireAdded && m_deviceAddedCallback )
	{
		m_deviceAddedCallback( addedIdentifier );
	}
}

void InputHandlerApple::HandleControllerDisconnected( GCController* controller )
{
	if( controller == nil )
	{
		return;
	}

	// The removed callback isn't fired here: Update() fires it only once a disconnect is actually
	// finalized (see below), so a matching reconnect (HandleControllerConnected re-adopting this slot)
	// never produces a spurious removed+added pair for what's really the same physical controller.
	std::unique_lock<std::mutex> lock( m_deviceMutex );
	for( auto& slot : m_deviceSlots )
	{
		if( slot->controller == controller )
		{
			ShutdownHapticsForSlot( *slot );
			slot->pendingRemoval = true;
			// Deliberately not resetting `active` here: it records whether the caller wants events
			// flowing, and a transport-swap reconnect (HandleControllerConnected) needs it to decide
			// whether to reinstall the handler on the new GCController instance. A real disconnect just
			// erases the whole slot in Update(), so there's nothing to leave in a stale state.
			slot->controller.physicalInputProfile.valueDidChangeHandler = nil;
			// Release these now, while the profile they came from is still around to release cleanly -
			// a matching reconnect rebuilds fresh ones for the new GCController instance, and otherwise
			// they'd sit untouched until the slot is destroyed, releasing stale element references into
			// a controller that's already torn down.
			slot->buttonSources.clear();
			slot->axisSources.clear();
			slot->switchSources.clear();
			CCP_LOGNOTICE( "InputHandlerApple: Device '%s' disconnected", slot->identifier.name.c_str() );
			break;
		}
	}
}

void InputHandlerApple::SetDeviceActivation( BlueSharedString deviceId, bool activate )
{
	// slot->active, slot->controller's valueDidChangeHandler and the input queue depth are also
	// written from HandleControllerConnected/HandleControllerDisconnected on m_handlerQueue, so
	// every touch of those fields here must happen under m_deviceMutex - otherwise this (game)
	// thread and the GameController notification thread can race on the same GCController
	// property/slot field with no ordering guarantee.
	std::unique_lock<std::mutex> lock( m_deviceMutex );
	auto* slot = FindDeviceSlotLocked( deviceId );
	if( slot == nullptr )
	{
		CCP_LOGERR( "InputHandlerApple: Could not find device with ID '%s' to set activation to %d",
			deviceId.c_str(), activate );
		return;
	}

	if( activate )
	{
		ActivateSlotHandler( *slot );
	}
	else
	{
		if( slot->controller != nil )
		{
			slot->controller.physicalInputProfile.valueDidChangeHandler = nil;
			id<GCDevicePhysicalInput> physicalInput = slot->controller.input;
			if( physicalInput != nil )
			{
				physicalInput.inputStateQueueDepth = 1;
			}
		}
		slot->active = false;

		// SendChannelIntensity takes m_deviceMutex itself (it has to stay unlocked while it makes
		// blocking CoreHaptics calls), so it can't be called while this scope still holds it.
		DeviceSlot* rawSlot = slot;
		lock.unlock();
		for( int i = 0; i < ChannelCount; ++i )
		{
			SendChannelIntensity( *rawSlot, static_cast<HapticsChannelIndex>( i ), 0.0f );
		}
	}
}

Events::State InputHandlerApple::SampleSlotState( const DeviceSlot& slot )
{
	Events::State state = {};
	state.timestamp = Events::GetTimestamp();

	// Sources hold a dictionary key rather than a live element (see ButtonHandling::ButtonSource),
	// so each sample re-resolves elements from the controller's current profile.
	GCPhysicalInputProfile* profile = slot.controller != nil ? slot.controller.physicalInputProfile : nil;

	// Mirrors InputHandlerWin::ReadDeviceState: sources were resolved once on connect,
	// so each sample only walks the precomputed extraction plan.
	for( const auto& source : slot.buttonSources )
	{
		state.buttons.insert( { static_cast<uint32_t>( source.descriptor ) + source.elementIndex, ButtonHandling::Handle( source, profile ) } );
	}
	for( const auto& source : slot.axisSources )
	{
		state.axis.insert( { static_cast<uint32_t>( source.descriptor ) + source.index, AxisHandling::Handle( source, profile ) } );
	}
	for( const auto& source : slot.switchSources )
	{
		state.switches.insert( { static_cast<uint32_t>( source.descriptor ) + source.index, SwitchHandling::Handle( source, profile ) } );
	}
	return state;
}

void InputHandlerApple::ActivateSlotHandler( DeviceSlot& slot )
{
	// nextInputState only returns queued snapshots when the queue is > 1 (default 1 = no buffering).
	id<GCDevicePhysicalInput> physicalInput = slot.controller.input;
	if( physicalInput != nil )
	{
		physicalInput.inputStateQueueDepth = 20;
	}

	DeviceSlot* rawSlot = &slot;
	slot.controller.physicalInputProfile.valueDidChangeHandler = ^( GCPhysicalInputProfile* profile, GCControllerElement* element ) {
		(void)profile;
		(void)element;
		Events::State state = SampleSlotState( *rawSlot );

		std::unique_lock<std::mutex> lock( this->m_readingMutex );
		rawSlot->accumulatedStates.push_back( std::move( state ) );
	};
	slot.active = true;

	// The handler above only fires on the next physical change, so a caller that activates a
	// device and immediately calls Update() (or activates one with a button already held) would
	// otherwise see nothing until the player touches the controller again. Seed one snapshot of
	// the current state right away.
	{
		std::unique_lock<std::mutex> lock( this->m_readingMutex );
		slot.accumulatedStates.push_back( SampleSlotState( slot ) );
	}
}

std::vector<Events::State> InputHandlerApple::Update( BlueSharedString deviceId )
{
	if( !m_initialized )
	{
		return {};
	}

	// Finalize any pending removals discovered since the last Update(). A slot only reaches here if no
	// matching reconnect re-adopted it (HandleControllerConnected clears pendingRemoval on adoption), so
	// this is a real disconnect and the removed callback fires exactly once, right here.
	std::vector<DeviceEnums::DeviceIdentifier> removedDevices;
	{
		std::unique_lock<std::mutex> lock( m_deviceMutex );
		for( auto it = m_deviceSlots.begin(); it != m_deviceSlots.end(); )
		{
			if( ( *it )->pendingRemoval )
			{
				CCP_LOGNOTICE( "InputHandlerApple: Device '%s' final removal", ( *it )->identifier.name.c_str() );
				removedDevices.push_back( ( *it )->identifier );
				ShutdownHapticsForSlot( **it );
				if( ( *it )->controller != nil )
				{
					( *it )->controller.physicalInputProfile.valueDidChangeHandler = nil;
					( *it )->controller = nil;
				}
				// Already cleared in HandleControllerDisconnected; cleared again defensively in case a
				// slot ever reaches finalization without going through that path.
				( *it )->buttonSources.clear();
				( *it )->axisSources.clear();
				( *it )->switchSources.clear();
				it = m_deviceSlots.erase( it );
			}
			else
			{
				++it;
			}
		}
	}

	if( m_deviceRemovedCallback )
	{
		for( auto& removedIdentifier : removedDevices )
		{
			m_deviceRemovedCallback( removedIdentifier );
		}
	}

	std::vector<Events::State> statesForDevice;
	{
		std::unique_lock<std::mutex> deviceLock( m_deviceMutex );
		auto* slot = FindDeviceSlotLocked( deviceId );
		if( slot == nullptr )
		{
			return statesForDevice;
		}

		// accumulatedStates is also pushed to from the valueDidChangeHandler block on
		// m_handlerQueue (guarded there by m_readingMutex) - swapping it out here without the
		// same lock is a data race on the vector's internals between this thread and that one.
		std::unique_lock<std::mutex> readingLock( m_readingMutex );
		std::swap( statesForDevice, slot->accumulatedStates );
	}
	return statesForDevice;
}

void InputHandlerApple::Rumble( BlueSharedString deviceId, Events::Rumble rumble )
{
	DeviceSlot* slot = nullptr;
	{
		std::unique_lock<std::mutex> lock( m_deviceMutex );
		slot = FindDeviceSlotLocked( deviceId );
		if( slot == nullptr || slot->haptics == nil )
		{
			return;
		}
	}

	const float requested[ChannelCount] = {
		rumble.lowFrequency,
		rumble.highFrequency,
		rumble.leftTrigger,
		rumble.rightTrigger,
	};

	// Released m_deviceMutex above: SendChannelIntensity takes it itself and must stay unlocked
	// while it makes blocking CoreHaptics calls. Safe to keep using the raw slot pointer here -
	// Update() is the only thing that erases slots, and it runs on this same calling thread.
	for( int i = 0; i < ChannelCount; ++i )
	{
		SendChannelIntensity( *slot, static_cast<HapticsChannelIndex>( i ), requested[i] );
	}
}

InputHandlerApple::DeviceSlot* InputHandlerApple::FindDeviceSlotLocked( BlueSharedString deviceId )
{
	auto it = std::find_if( m_deviceSlots.begin(), m_deviceSlots.end(),
		[deviceId]( const std::unique_ptr<DeviceSlot>& slot ) {
			return slot->controller != nil && !slot->pendingRemoval && slot->identifier.deviceID == deviceId;
		} );
	return it != m_deviceSlots.end() ? it->get() : nullptr;
}

InputHandlerApple::DeviceSlot* InputHandlerApple::GetDeviceSlot( GCController* controller )
{
	std::unique_lock<std::mutex> lock( m_deviceMutex );
	auto it = std::find_if( m_deviceSlots.begin(), m_deviceSlots.end(),
		[controller]( const std::unique_ptr<DeviceSlot>& slot ) {
			return slot->controller == controller;
		} );
	return it != m_deviceSlots.end() ? it->get() : nullptr;
}

void InputHandlerApple::SetBackgroundEventsEnabled( bool enabled )
{
	if( enabled )
	{
		CCP_LOGNOTICE( "InputHandlerApple: Enabling background event monitoring" );
        GCController.shouldMonitorBackgroundEvents = YES;
	}
	else
	{
		CCP_LOGNOTICE( "InputHandlerApple: Disabling background event monitoring" );
		GCController.shouldMonitorBackgroundEvents = NO;
	}
}

void InputHandlerApple::InitializeHapticsForSlot( DeviceSlot& slot )
{
	if( !( @available( macOS 11.0, * ) ) )
	{
		slot.identifier.rumbleCapacity = DeviceEnums::RumbleCapacity{};
		return;
	}

	if( slot.controller == nil )
	{
		slot.identifier.rumbleCapacity = DeviceEnums::RumbleCapacity{};
		return;
	}

	GCDeviceHaptics* haptics = slot.controller.haptics;
	if( haptics == nil )
	{
		slot.identifier.rumbleCapacity = DeviceEnums::RumbleCapacity{};
		return;
	}
	slot.haptics = haptics;

	const bool wants[ChannelCount] = {
		slot.identifier.rumbleCapacity.hasLowFrequencyRumble,
		slot.identifier.rumbleCapacity.hasHighFrequencyRumble,
		slot.identifier.rumbleCapacity.hasLeftTriggerRumble,
		slot.identifier.rumbleCapacity.hasRightTriggerRumble,
	};

	DeviceSlot* rawSlot = &slot;
	for( int i = 0; i < ChannelCount; ++i )
	{
		if( !wants[i] )
		{
			continue;
		}

		HapticsChannel& channel = slot.hapticsChannels[i];
		NSError* err = nil;
		channel.engine = [haptics createEngineWithLocality:LocalityForChannel( i )];
		if( channel.engine == nil )
		{
			CCP_LOGWARN( "InputHandlerApple: Failed to create haptics engine for channel %d on device '%s'",
				i, slot.identifier.name.c_str() );
			ClearCapacityForChannel( slot.identifier.rumbleCapacity, i );
			continue;
		}

		channel.engine.autoShutdownEnabled = NO;

		const int channelIndex = i;
		channel.engine.resetHandler = ^{
			CCP_LOGWARN( "InputHandlerApple: Haptics engine reset on channel %d, restarting", channelIndex );
			std::unique_lock<std::mutex> lock( this->m_deviceMutex );
			bool slotAlive = false;
			for( auto& s : m_deviceSlots )
			{
				if( s.get() == rawSlot )
				{
					slotAlive = true;
					break;
				}
			}
			if( !slotAlive )
			{
				return;
			}
			NSError* restartErr = nil;
			if( ![rawSlot->hapticsChannels[channelIndex].engine startAndReturnError:&restartErr] )
			{
				CCP_LOGWARN( "InputHandlerApple: Failed to restart engine on channel %d: %s",
					channelIndex,
					restartErr.localizedDescription.UTF8String ? restartErr.localizedDescription.UTF8String : "(no message)" );
				return;
			}
			// The previous player belongs to the old engine instance; drop it and force the next Rumble() to rebuild.
			rawSlot->hapticsChannels[channelIndex].player = nil;
			rawSlot->hapticsChannels[channelIndex].lastIntensity = -1.0f;
		};

		channel.engine.stoppedHandler = ^( CHHapticEngineStoppedReason reason ) {
			CCP_LOGWARN( "InputHandlerApple: Haptics engine stopped on channel %d (reason=%ld)",
				channelIndex, (long)reason );
		};

		if( ![channel.engine startAndReturnError:&err] )
		{
			CCP_LOGWARN( "InputHandlerApple: Failed to start haptics engine on channel %d for device '%s': %s",
				i, slot.identifier.name.c_str(),
				err.localizedDescription.UTF8String ? err.localizedDescription.UTF8String : "(no message)" );
			channel.engine = nil;
			ClearCapacityForChannel( slot.identifier.rumbleCapacity, i );
			continue;
		}

		channel.supported = true;
	}

	if( slot.identifier.rumbleCapacity.rumbleMotorCount == 0 )
	{
		slot.haptics = nil;
	}
}

void InputHandlerApple::SendChannelIntensity( DeviceSlot& slot, HapticsChannelIndex channel, float intensity )
{
	if( !( @available( macOS 11.0, * ) ) )
	{
		return;
	}

	const float clamped = std::clamp( intensity, 0.0f, 1.0f );

	CHHapticEngine* engine = nil;
	id<CHHapticPatternPlayer> oldPlayer = nil;
	{
		std::unique_lock<std::mutex> lock( m_deviceMutex );
		HapticsChannel& ch = slot.hapticsChannels[channel];
		if( !ch.supported || ch.engine == nil )
		{
			return;
		}
		if( ch.lastIntensity == clamped )
		{
			return;
		}
		ch.lastIntensity = clamped;
		oldPlayer = ch.player;
		ch.player = nil;
		engine = ch.engine;
	}

	if( oldPlayer != nil )
	{
		[oldPlayer stopAtTime:0 error:nil];
	}

	if( clamped <= 0.0f )
	{
		return;
	}

	NSError* err = nil;
	CHHapticEventParameter* intensityParam = [[CHHapticEventParameter alloc]
		initWithParameterID:CHHapticEventParameterIDHapticIntensity value:clamped];
	CHHapticEventParameter* sharpnessParam = [[CHHapticEventParameter alloc]
		initWithParameterID:CHHapticEventParameterIDHapticSharpness value:SharpnessForChannel( channel )];

	// GCHapticDurationInfinite is the game-controller-safe way to hold a continuous event open until we stop it.
	CHHapticEvent* event = [[CHHapticEvent alloc]
		initWithEventType:CHHapticEventTypeHapticContinuous
			   parameters:@[intensityParam, sharpnessParam]
			 relativeTime:0.0
				 duration:GCHapticDurationInfinite];

	CHHapticPattern* pattern = [[CHHapticPattern alloc] initWithEvents:@[event] parameters:@[] error:&err];
	if( pattern == nil )
	{
		CCP_LOGWARN( "InputHandlerApple: Failed to build haptics pattern on channel %d: %s",
			(int)channel,
			err.localizedDescription.UTF8String ? err.localizedDescription.UTF8String : "(no message)" );
		return;
	}

	// GCDeviceHaptics engines reject createAdvancedPlayerWithPattern:; only the basic player is supported.
	id<CHHapticPatternPlayer> player = [engine createPlayerWithPattern:pattern error:&err];
	if( player == nil )
	{
		CCP_LOGWARN( "InputHandlerApple: Failed to create player on channel %d: %s",
			(int)channel,
			err.localizedDescription.UTF8String ? err.localizedDescription.UTF8String : "(no message)" );
		return;
	}

	if( ![player startAtTime:0 error:&err] )
	{
		CCP_LOGWARN( "InputHandlerApple: Failed to start player on channel %d: %s",
			(int)channel,
			err.localizedDescription.UTF8String ? err.localizedDescription.UTF8String : "(no message)" );
		return;
	}

	std::unique_lock<std::mutex> lock( m_deviceMutex );
	slot.hapticsChannels[channel].player = player;
}

void InputHandlerApple::ShutdownHapticsForSlot( DeviceSlot& slot )
{
	if( !( @available( macOS 11.0, * ) ) )
	{
		return;
	}

	for( auto& ch : slot.hapticsChannels )
	{
		// Nil first so any queued reset/stopped callback that reaches m_deviceMutex sees the channel already torn down.
		id<CHHapticPatternPlayer> player = ch.player;
		CHHapticEngine* engine = ch.engine;
		ch.player = nil;
		ch.engine = nil;
		ch.supported = false;
		ch.lastIntensity = 0.0f;

		if( player != nil )
		{
			[player stopAtTime:0 error:nil];
		}
		if( engine != nil )
		{
			[engine stopWithCompletionHandler:nil];
		}
	}
	slot.haptics = nil;
}
#endif // __APPLE__
