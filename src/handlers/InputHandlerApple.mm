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

// Sanitize a value so it can appear in a device ID string (strip spaces / punctuation).
std::string SanitizeForDeviceID( NSString* input )
{
	if( input == nil )
	{
		return std::string();
	}
	std::string result;
	result.reserve( [input length] );
	const char* utf8 = [input UTF8String];
	if( utf8 == nullptr )
	{
		return result;
	}
	for( const char* c = utf8; *c != '\0'; ++c )
	{
		const unsigned char uc = static_cast<unsigned char>( *c );
		if( ( uc >= 'A' && uc <= 'Z' ) || ( uc >= 'a' && uc <= 'z' ) || ( uc >= '0' && uc <= '9' ) )
		{
			result.push_back( static_cast<char>( uc ) );
		}
		else if( uc == '-' || uc == '_' )
		{
			result.push_back( static_cast<char>( uc ) );
		}
	}
	return result;
}

// A button is treated as an analog trigger if it exposes an analog value.
bool IsAnalogTriggerButton( GCControllerButtonInput* button )
{
	if( button == nil )
	{
		return false;
	}
	return button.isAnalog ? YES : NO;
}

BlueSharedString SharedStringFromNSString( NSString* string )
{
	if( string == nil )
	{
		return BlueSharedString();
	}

	const char* utf8 = [string UTF8String];
	return utf8 == nullptr ? BlueSharedString() : BlueSharedString( utf8 );
}

// Elements are normally named from their localizedName. Fall back to a synthesized positional name
// so an element never surfaces to script as an empty string.
BlueSharedString NamedElement( NSString* localizedName, const char* typeDescriptor, size_t index )
{
	BlueSharedString name = SharedStringFromNSString( localizedName );
	if( !name.empty() )
	{
		return name;
	}

	const std::string fallback = std::string( "Custom " ) + typeDescriptor + " " + std::to_string( index );
	return BlueSharedString( fallback );
}

Events::SwitchPosition MapDpadPosition( GCControllerDirectionPad* dpad )
{
	if( dpad == nil )
	{
		return Events::SwitchPosition::Center;
	}

    const bool up = dpad.up.isPressed;
    const bool down = dpad.down.isPressed;
    const bool right = dpad.right.isPressed;
	const bool left = dpad.left.isPressed;

	if( up && right ) return Events::SwitchPosition::UpRight;
	if( up && left ) return Events::SwitchPosition::UpLeft;
	if( down && right ) return Events::SwitchPosition::DownRight;
	if( down && left ) return Events::SwitchPosition::DownLeft;
	if( up ) return Events::SwitchPosition::Up;
	if( down ) return Events::SwitchPosition::Down;
	if( right ) return Events::SwitchPosition::Right;
	if( left ) return Events::SwitchPosition::Left;
	return Events::SwitchPosition::Center;
}

// Build the ordered element arrays from a controller's physicalInputProfile.
void CollectProfileElements(
	GCController* controller,
	std::vector<GCControllerButtonInput*>& outButtons,
    std::vector<GCControllerAxisInput*>& outAxes,
    std::vector<GCControllerButtonInput*>& outTriggerAxes,
    std::vector<GCControllerDirectionPad*>&outSwitches )
{
    GCPhysicalInputProfile* profile = controller.physicalInputProfile;

    // .allButtons/.allAxes/.allDpads are NSSets with undefined enumeration order. Instead iterate the
    // .buttons/.axes/.dpads dictionaries by sorted key so the resulting element order is deterministic.
    // An element can be reachable under several keys at once (see GCControllerElement.aliases), so we
    // also dedupe by identity, keeping only the earliest (alphabetically smallest) key for each element.
    NSArray<NSString*>* buttonKeys = [profile.buttons.allKeys sortedArrayUsingSelector:@selector( compare: )];
    for( NSString* key in buttonKeys )
    {
        GCControllerButtonInput* button = profile.buttons[key];
        // we don't want buttons that are part of a collection, like a dpad group
        if( button.collection == nil )
        {
            if( IsAnalogTriggerButton( button ) )
            {
                if( std::find( outTriggerAxes.begin(), outTriggerAxes.end(), button ) == outTriggerAxes.end() )
                {
                    outTriggerAxes.push_back( button );
                }
            }
            else
            {
                if( std::find( outButtons.begin(), outButtons.end(), button ) == outButtons.end() )
                {
                    outButtons.push_back( button );
                }
            }
        }
    }

    NSArray<NSString*>* axisKeys = [profile.axes.allKeys sortedArrayUsingSelector:@selector( compare: )];
    for( NSString* key in axisKeys )
    {
        GCControllerAxisInput* axis = profile.axes[key];
        // Skip axes that belong to a collection (e.g. dpad X/Y components) — the parent
        // element is handled in the dpad pass below, which decides whether it becomes
        // a switch or contributes its child axes here.
        if( axis.collection == nil && std::find( outAxes.begin(), outAxes.end(), axis ) == outAxes.end() )
        {
            outAxes.push_back( axis );
        }
    }

    NSArray<NSString*>* dpadKeys = [profile.dpads.allKeys sortedArrayUsingSelector:@selector( compare: )];
    for( NSString* key in dpadKeys )
    {
        GCControllerDirectionPad* dpad = profile.dpads[key];
        if( std::find( outSwitches.begin(), outSwitches.end(), dpad ) != outSwitches.end() )
        {
            continue;
        }
        // Apple models thumbsticks and touchpads as direction pads too. Those report analog child
        // axes, whereas a real d-pad is digital. Expose the analog ones as a pair of axes and keep
        // only digital d-pads in the switch dimension.
        if( dpad.xAxis != nil && dpad.yAxis != nil && dpad.xAxis.isAnalog )
        {
            if( std::find( outAxes.begin(), outAxes.end(), dpad.xAxis ) == outAxes.end() )
            {
                outAxes.push_back( dpad.xAxis );
            }
            if( std::find( outAxes.begin(), outAxes.end(), dpad.yAxis ) == outAxes.end() )
            {
                outAxes.push_back( dpad.yAxis );
            }
        }
        else
        {
            outSwitches.push_back( dpad );
        }
    }
}

// Build a DeviceIdentifier for a freshly-connected controller.
DeviceEnums::DeviceIdentifier BuildIdentifier(
	GCController* controller,
	uint64_t counter,
	const std::vector<GCControllerButtonInput*>& buttons,
    const std::vector<GCControllerAxisInput*>& axes,
    const std::vector<GCControllerButtonInput*>& triggerAxes,
    const std::vector<GCControllerDirectionPad*>& dpads )
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

	const std::string vendorSan = SanitizeForDeviceID( vendorName );
	const std::string productSan = SanitizeForDeviceID( productCategory );
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

	for( const GCControllerButtonInput* button : buttons )
	{
		if( button )
		{
			identifier.buttons.push_back( NamedElement( button.localizedName, "Button", identifier.buttons.size() ) );
		}
	}
	for( const GCControllerAxisInput* axis : axes )
	{
		if( axis )
		{
			identifier.axes.push_back( NamedElement( axis.localizedName, "Axis", identifier.axes.size() ) );
		}
	}
	for( const GCControllerButtonInput* axis : triggerAxes )
	{
		if( axis )
		{
			identifier.axes.push_back( NamedElement( axis.localizedName, "Axis", identifier.axes.size() ) );
		}
	}
	for( const GCControllerDirectionPad* dpad : dpads )
	{
		if( dpad )
		{
			identifier.switches.push_back( NamedElement( dpad.localizedName, "Dpad", identifier.switches.size() ) );
		}
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
            slot->buttons.clear();
            slot->axes.clear();
            slot->triggerAxes.clear();
            slot->switches.clear();
		}
		m_deviceSlots.clear();
	}

	CCP_LOGNOTICE( "InputHandlerApple: Shut down" );
}

bool InputHandlerApple::Initialize()
{
	if( m_initialized )
	{
		return true;
	}

	NSNotificationCenter* center = [NSNotificationCenter defaultCenter];

	m_connectObserver = [center addObserverForName:GCControllerDidConnectNotification
											object:nil
											 queue:[NSOperationQueue mainQueue]
										usingBlock:^( NSNotification* note ) {
		GCController* controller = (GCController*)note.object;
		this->HandleControllerConnected( controller );
	}];

	m_disconnectObserver = [center addObserverForName:GCControllerDidDisconnectNotification
											   object:nil
												queue:[NSOperationQueue mainQueue]
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

    std::vector<GCControllerButtonInput*> buttons = {};
    std::vector<GCControllerAxisInput*> axes = {};
    std::vector<GCControllerButtonInput*> triggerAxes = {};
    std::vector<GCControllerDirectionPad*> switches = {};
	CollectProfileElements( controller, buttons, axes, triggerAxes, switches );

	const uint64_t counter = m_deviceCounter.fetch_add( 1 );
	DeviceEnums::DeviceIdentifier identifier = BuildIdentifier(
		controller, counter, buttons, axes, triggerAxes, switches );

	{
		std::unique_lock<std::mutex> lock( m_deviceMutex );
		auto existing = std::find_if( m_deviceSlots.begin(), m_deviceSlots.end(),
			[controller]( const std::unique_ptr<DeviceSlot>& slot ) {
				return slot->controller == controller;
			} );

		if( existing != m_deviceSlots.end() )
		{
			( *existing )->pendingRemoval = false;
			CCP_LOGNOTICE( "InputHandlerApple: Device '%s' reconnected", identifier.name.c_str() );
		}
		else
        {
			auto slot = std::make_unique<DeviceSlot>();
			slot->controller = controller;
			slot->buttons = buttons;
			slot->axes = axes;
			slot->triggerAxes = triggerAxes;
			slot->switches = switches;
			slot->identifier = identifier;
            
			DeviceSlot* rawSlot = slot.get();
			m_deviceSlots.push_back( std::move( slot ) );
			if( rawSlot->identifier.rumbleCapacity.rumbleMotorCount > 0 )
			{
				InitializeHapticsForSlot( *rawSlot );
			}
			identifier = rawSlot->identifier;
			CCP_LOGNOTICE( "InputHandlerApple: Device '%s' connected", identifier.name.c_str() );
		}
	}

	if( m_deviceAddedCallback )
	{
		m_deviceAddedCallback( identifier );
	}
}

void InputHandlerApple::HandleControllerDisconnected( GCController* controller )
{
	if( controller == nil )
	{
		return;
	}

	DeviceEnums::DeviceIdentifier removedIdentifier;
	bool found = false;
	{
		std::unique_lock<std::mutex> lock( m_deviceMutex );
		for( auto& slot : m_deviceSlots )
		{
			if( slot->controller == controller )
			{
				ShutdownHapticsForSlot( *slot );
				slot->pendingRemoval = true;
				slot->active = false;
				slot->controller.physicalInputProfile.valueDidChangeHandler = nil;
				removedIdentifier = slot->identifier;
				found = true;
				CCP_LOGNOTICE( "InputHandlerApple: Device '%s' disconnected", slot->identifier.name.c_str() );
				break;
			}
		}
	}

	if( found && m_deviceRemovedCallback )
	{
		m_deviceRemovedCallback( removedIdentifier );
	}
}

void InputHandlerApple::SetDeviceActivation( BlueSharedString deviceId, bool activate )
{
	auto* slot = GetDeviceSlot( deviceId );
	if( slot == nullptr )
	{
		CCP_LOGERR( "InputHandlerApple: Could not find device with ID '%s' to set activation to %d",
			deviceId.c_str(), activate );
		return;
	}

	if( activate )
	{
		// nextInputState only returns queued snapshots when the queue is > 1 (default 1 = no buffering).
		id<GCDevicePhysicalInput> physicalInput = slot->controller.input;
		if( physicalInput != nil )
		{
			physicalInput.inputStateQueueDepth = 20;
		}

		DeviceSlot* rawSlot = slot;
		slot->controller.physicalInputProfile.valueDidChangeHandler = ^( GCPhysicalInputProfile* profile, GCControllerElement* element ) {
			(void)profile;
			(void)element;
			Events::State state = {};
			state.timestamp = Events::GetTimestamp();

			const auto buttonCount = rawSlot->buttons.size();
			if( buttonCount > 0 )
			{
				state.buttons.resize( buttonCount );
				for( NSUInteger i = 0; i < buttonCount; ++i )
				{
					GCControllerButtonInput* b = rawSlot->buttons[i];
					state.buttons[i].pressed = b.isPressed ? true : false;
				}
			}

			const auto axisCount = rawSlot->axes.size();
			const auto triggerCount = rawSlot->triggerAxes.size();
			if( axisCount + triggerCount > 0 )
			{
				state.axis.resize( axisCount + triggerCount );
				for( NSUInteger i = 0; i < axisCount; ++i )
				{
					GCControllerAxisInput* a = rawSlot->axes[i];
					state.axis[i].value = a.value;
				}
				for( NSUInteger i = 0; i < triggerCount; ++i )
				{
					GCControllerButtonInput* t = rawSlot->triggerAxes[i];
					state.axis[axisCount + i].value = t.value;
				}
			}

			const NSUInteger switchCount = rawSlot->switches.size();
			if( switchCount > 0 )
			{
				state.switches.resize( switchCount );
				for( NSUInteger i = 0; i < switchCount; ++i )
				{
					state.switches[i].position = MapDpadPosition( rawSlot->switches[i] );
				}
			}

			std::unique_lock<std::mutex> lock( this->m_readingMutex );
			rawSlot->accumulatedStates.push_back( std::move( state ) );
		};
		slot->active = true;
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
		for( int i = 0; i < ChannelCount; ++i )
		{
			SendChannelIntensity( *slot, static_cast<HapticsChannelIndex>( i ), 0.0f );
		}
	}
}

std::vector<Events::State> InputHandlerApple::Update( BlueSharedString deviceId )
{
	if( !m_initialized )
	{
		return {};
	}

	// Finalize any pending removals discovered since the last Update().
	{
		std::unique_lock<std::mutex> lock( m_deviceMutex );
		for( auto it = m_deviceSlots.begin(); it != m_deviceSlots.end(); )
		{
			if( ( *it )->pendingRemoval )
			{
				CCP_LOGNOTICE( "InputHandlerApple: Device '%s' final removal", ( *it )->identifier.name.c_str() );
				ShutdownHapticsForSlot( **it );
				if( ( *it )->controller != nil )
				{
					( *it )->controller.physicalInputProfile.valueDidChangeHandler = nil;
					( *it )->controller = nil;
				}
				it = m_deviceSlots.erase( it );
			}
			else
			{
				++it;
			}
		}
	}

	std::vector<Events::State> statesForDevice;
	auto* slot = GetDeviceSlot( deviceId );
	if( slot == nullptr )
	{
		return statesForDevice;
	}
    
	std::swap( statesForDevice, slot->accumulatedStates );
	return statesForDevice;
}

void InputHandlerApple::Rumble( BlueSharedString deviceId, Events::Rumble rumble )
{
	auto* slot = GetDeviceSlot( deviceId );
	if( slot == nullptr || slot->haptics == nil )
	{
		return;
	}

	const float requested[ChannelCount] = {
		rumble.lowFrequency,
		rumble.highFrequency,
		rumble.leftTrigger,
		rumble.rightTrigger,
	};

	for( int i = 0; i < ChannelCount; ++i )
	{
		SendChannelIntensity( *slot, static_cast<HapticsChannelIndex>( i ), requested[i] );
	}
}

InputHandlerApple::DeviceSlot* InputHandlerApple::GetDeviceSlot( BlueSharedString deviceId )
{
	std::unique_lock<std::mutex> lock( m_deviceMutex );
	auto it = std::find_if( m_deviceSlots.begin(), m_deviceSlots.end(),
		[deviceId]( const std::unique_ptr<DeviceSlot>& slot ) {
			return slot->controller != nil && slot->identifier.deviceID == deviceId;
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
			CCP_LOGWARN( "InputHandlerApple: Haptics engine reset on channel %d, rebuilding", channelIndex );
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
			this->RebuildChannelPlayer( *rawSlot, static_cast<HapticsChannelIndex>( channelIndex ) );
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

		if( !RebuildChannelPlayer( slot, static_cast<HapticsChannelIndex>( i ) ) )
		{
			[channel.engine stopWithCompletionHandler:nil];
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

bool InputHandlerApple::RebuildChannelPlayer( DeviceSlot& slot, HapticsChannelIndex channel )
{
	if( !( @available( macOS 11.0, * ) ) )
	{
		return false;
	}

	HapticsChannel& ch = slot.hapticsChannels[channel];
	if( ch.engine == nil )
	{
		return false;
	}

	NSError* err = nil;
	CHHapticEventParameter* intensity = [[CHHapticEventParameter alloc]
		initWithParameterID:CHHapticEventParameterIDHapticIntensity value:1.0f];
	CHHapticEventParameter* sharpness = [[CHHapticEventParameter alloc]
		initWithParameterID:CHHapticEventParameterIDHapticSharpness value:SharpnessForChannel( channel )];

	// 30s is CoreHaptics' documented maximum event duration; loopEnabled on the player extends it indefinitely.
	CHHapticEvent* event = [[CHHapticEvent alloc]
		initWithEventType:CHHapticEventTypeHapticContinuous
			   parameters:@[intensity, sharpness]
			 relativeTime:0.0
				 duration:30.0];

	CHHapticPattern* pattern = [[CHHapticPattern alloc] initWithEvents:@[event] parameters:@[] error:&err];
	if( pattern == nil )
	{
		CCP_LOGWARN( "InputHandlerApple: Failed to build haptics pattern on channel %d: %s",
			(int)channel,
			err.localizedDescription.UTF8String ? err.localizedDescription.UTF8String : "(no message)" );
		return false;
	}

	id<CHHapticAdvancedPatternPlayer> player = [ch.engine createAdvancedPlayerWithPattern:pattern error:&err];
	if( player == nil )
	{
		CCP_LOGWARN( "InputHandlerApple: Failed to create advanced player on channel %d: %s",
			(int)channel,
			err.localizedDescription.UTF8String ? err.localizedDescription.UTF8String : "(no message)" );
		return false;
	}
	player.loopEnabled = YES;

	if( ![player startAtTime:0 error:&err] )
	{
		CCP_LOGWARN( "InputHandlerApple: Failed to start advanced player on channel %d: %s",
			(int)channel,
			err.localizedDescription.UTF8String ? err.localizedDescription.UTF8String : "(no message)" );
		return false;
	}

	ch.player = player;

	CHHapticDynamicParameter* param = [[CHHapticDynamicParameter alloc]
		initWithParameterID:CHHapticDynamicParameterIDHapticIntensityControl
					  value:std::clamp( ch.lastIntensity, 0.0f, 1.0f )
			   relativeTime:0.0];
	NSError* sendErr = nil;
	if( ![player sendParameters:@[param] atTime:0 error:&sendErr] )
	{
		CCP_LOGWARN( "InputHandlerApple: Failed to send initial intensity on channel %d: %s",
			(int)channel,
			sendErr.localizedDescription.UTF8String ? sendErr.localizedDescription.UTF8String : "(no message)" );
	}
	return true;
}

void InputHandlerApple::SendChannelIntensity( DeviceSlot& slot, HapticsChannelIndex channel, float intensity )
{
	if( !( @available( macOS 11.0, * ) ) )
	{
		return;
	}

	const float clamped = std::clamp( intensity, 0.0f, 1.0f );

	id<CHHapticAdvancedPatternPlayer> player = nil;
	{
		std::unique_lock<std::mutex> lock( m_deviceMutex );
		HapticsChannel& ch = slot.hapticsChannels[channel];
		if( !ch.supported )
		{
			return;
		}
		if( ch.lastIntensity == clamped )
		{
			return;
		}
		ch.lastIntensity = clamped;
		player = ch.player;
	}

	if( player == nil )
	{
		return;
	}

	CHHapticDynamicParameter* param = [[CHHapticDynamicParameter alloc]
		initWithParameterID:CHHapticDynamicParameterIDHapticIntensityControl
					  value:clamped
			   relativeTime:0.0];
	NSError* err = nil;
	if( ![player sendParameters:@[param] atTime:0 error:&err] )
	{
		CCP_LOGNOTICE( "InputHandlerApple: sendParameters failed on channel %d: %s",
			(int)channel,
			err.localizedDescription.UTF8String ? err.localizedDescription.UTF8String : "(no message)" );
	}
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
		id<CHHapticAdvancedPatternPlayer> player = ch.player;
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
