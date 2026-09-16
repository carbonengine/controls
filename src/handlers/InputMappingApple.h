#pragma once
#ifdef __APPLE__
#include "IInputHandler.h"
#include "../events/IInputEvent.h"

#import <Foundation/Foundation.h>
#import <GameController/GameController.h>

#include <string>
#include <vector>

namespace InputMapping
{
// Apple's GCInput* dictionary keys are stable API constants, unlike localizedName which is
// user-locale display text. Fold them to the abstract slot the control occupies; the glyph
// flavour is a display concern, so the result matches what the Windows handler produces.
DeviceEnums::InputElementDescriptor ElementForKey( NSString* key );

// Thumbstick child axes are reached through their parent d-pad element, so their identity
// depends on which parent they came from and which component they are.
DeviceEnums::InputElementDescriptor ElementForThumbstickAxis( NSString* parentKey, bool isXAxis );

// Windows reports L2/R2 as both a digital press and an analog value; this maps the axis
// descriptor to its button counterpart so Apple can mirror that dual representation.
DeviceEnums::InputElementDescriptor TriggerButtonElement( DeviceEnums::InputElementDescriptor axisElement );

// Family is resolved from the hardware's product category rather than from the element keys,
// which Apple normalizes to Xbox-style names for every controller.
DeviceEnums::DeviceFamily GetDeviceFamily( GCController* controller );

// Sanitize a value so it can appear in a device ID string (strip spaces / punctuation).
std::string SanitizeForDeviceID( NSString* input );
}

namespace ButtonHandling
{
/**
* @brief Where a single published button is sampled from.
*
* Plain data: resolved once when the device connects, then walked per reading.
*/
struct ButtonSource
{
	__strong GCControllerButtonInput* button = nil; ///< The digital button element this source samples.
	uint32_t elementIndex = 0; ///< Published index; only Unknown descriptors are numbered, everything else is 0.
	DeviceEnums::InputElementDescriptor descriptor = DeviceEnums::InputElementDescriptor::Unknown;
};

std::vector<ButtonSource> GetButtonSources( GCController* controller );
Events::Button Handle( const ButtonSource& source );
}

namespace AxisHandling
{
/**
* @brief Where a single published axis is sampled from.
*/
struct AxisSource
{
	__strong GCControllerAxisInput* axis = nil;           ///< Non-nil for a plain analog axis or thumbstick component.
	__strong GCControllerButtonInput* triggerButton = nil; ///< Non-nil for an analog trigger button sampled as an axis.
	uint32_t index = 0; ///< Published index; position within the device's axis list.
	DeviceEnums::InputElementDescriptor descriptor = DeviceEnums::InputElementDescriptor::Unknown;
};

std::vector<AxisSource> GetAxisSources( GCController* controller );
Events::Axis Handle( const AxisSource& source );
}

namespace SwitchHandling
{
/**
* @brief Where a single published switch is sampled from.
*/
struct SwitchSource
{
	__strong GCControllerDirectionPad* dpad = nil;
	uint32_t index = 0; ///< Published index; position within the device's switch list.
	DeviceEnums::InputElementDescriptor descriptor = DeviceEnums::InputElementDescriptor::DPad;
};

std::vector<SwitchSource> GetSwitchSources( GCController* controller );
Events::Switch Handle( const SwitchSource& source );
}

#endif // __APPLE__
