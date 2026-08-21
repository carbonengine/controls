#ifdef __APPLE__
#include "InputHandlerApple.h"

#import <Foundation/Foundation.h>
#import <GameController/GameController.h>

#include <algorithm>
#include <sstream>
#include <string>

#include "../ControlManager.h"

namespace
{
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
    
    for( GCControllerButtonInput* button in profile.allButtons )
    {
        // we don't want buttons that are part of a collection, like a dpad group
        if( button.collection == nil )
        {
            if( IsAnalogTriggerButton( button ) )
            {
                outTriggerAxes.push_back( button );
            }
            else
            {
                outButtons.push_back( button );
            }
        }
    }
    
    for( GCControllerAxisInput* axis in profile.allAxes )
    {
        // Skip axes that belong to a collection (e.g. dpad X/Y components) — the parent
        // element (dpad, thumbstick, touchpad) is already exposed via allDpads as a switch.
        if( axis.collection == nil )
        {
            outAxes.push_back( axis );
        }
    }
    for( GCControllerDirectionPad* dpad in profile.allDpads )
    {
        outSwitches.push_back( dpad );
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

    identifier.buttonCount = static_cast<uint32_t>( buttons.size() );
    identifier.axisCount = static_cast<uint32_t>( axes.size() + triggerAxes.size() );
	identifier.switchCount = static_cast<uint32_t>( dpads.size() );
    
	// Rumble is stubbed; report no capacity so callers do not attempt to drive motors.
	identifier.rumbleCapacity = DeviceEnums::RumbleCapacity{};

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
            for( const GCControllerButtonInput* button : buttons )
            {
                if( button )
                {
                    slot->buttonNames.push_back( SharedStringFromNSString( button.localizedName ) );
                }
            }
            for( const GCControllerAxisInput* axis : axes )
            {
                if( axis )
                {
                    slot->axisNames.push_back( SharedStringFromNSString( axis.localizedName ) );
                }
            }
            for( const GCControllerButtonInput* axis : triggerAxes )
            {
                if( axis )
                {
                    slot->axisNames.push_back( SharedStringFromNSString( axis.localizedName ) );
                }
            }
            for( const GCControllerDirectionPad* dpad : switches )
            {
                if( dpad )
                {
                    slot->switchNames.push_back( SharedStringFromNSString( dpad.localizedName ) );
                }
            }
			m_deviceSlots.push_back( std::move( slot ) );
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
	// Stubbed: CoreHaptics integration is deferred.
	CCP_LOGNOTICE(
		"InputHandlerApple: Rumble stub for device '%s' (low=%.2f high=%.2f leftTrig=%.2f rightTrig=%.2f)",
		deviceId.c_str(),
		rumble.lowFrequency,
		rumble.highFrequency,
		rumble.leftTrigger,
		rumble.rightTrigger );
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

const std::vector<BlueSharedString> InputHandlerApple::GetButtonNames( BlueSharedString deviceId )
{
    auto deviceSlot = GetDeviceSlot( deviceId) ;
    if( deviceSlot )
    {
        return deviceSlot->buttonNames;
    }
    return {};
}

const std::vector<BlueSharedString> InputHandlerApple::GetAxisNames( BlueSharedString deviceId )
{
    auto deviceSlot = GetDeviceSlot( deviceId) ;
    if( deviceSlot )
    {
        return deviceSlot->axisNames;
    }
    return {};
}

const std::vector<BlueSharedString> InputHandlerApple::GetSwitchNames( BlueSharedString deviceId )
{
    auto deviceSlot = GetDeviceSlot( deviceId) ;
    if( deviceSlot )
    {
        return deviceSlot->switchNames;
    }
    return {};
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
#endif // __APPLE__
