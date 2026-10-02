// Copyright © 2026 CCP ehf.

#pragma once
#ifdef __APPLE__
#include "IInputHandler.h"
#include "../events/Events.h"

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
* Plain data: resolved once when the device connects, then walked per reading. Mirrors
* InputHandlerWin's ButtonSource - rather than holding the live GCControllerButtonInput (which
* is only valid for as long as the GCController instance that vended it is alive), this stores
* the dictionary key needed to re-fetch it from a GCPhysicalInputProfile on every sample. The
* keys involved (GCInputButtonA, etc.) are framework-owned constants, not per-controller state.
*/
struct ButtonSource
{
	/// @brief Which profile dictionary - and, for a dpad, which component - supplies this button.
	enum class Kind : uint8_t
	{
		None, ///< Not present on this device; always reads as unpressed.
		ProfileButton, ///< Sample profile.buttons[key].isPressed.
		DpadDirection ///< Sample profile.dpads[key].<direction>.isPressed.
	};

	/// @brief Which digital component of a dpad to sample; only meaningful when kind is DpadDirection.
	enum class DpadDirection : uint8_t
	{
		Up,
		Down,
		Left,
		Right
	};

	Kind kind = Kind::None;
	__strong NSString* key = nil; ///< Key into profile.buttons or profile.dpads identifying this element.
	DpadDirection direction = DpadDirection::Up; ///< Component to sample when kind is DpadDirection.
	DeviceEnums::ElementKey element{}; ///< Identity this button is published under.
};

std::vector<ButtonSource> GetButtonSources( GCController* controller );
Events::Button Handle( const ButtonSource& source, GCPhysicalInputProfile* profile );
}

namespace AxisHandling
{
/**
* @brief Where a single published axis is sampled from.
*
* Stores a dictionary key rather than the live GCControllerAxisInput/GCControllerButtonInput,
* for the same reason as ButtonHandling::ButtonSource above.
*/
struct AxisSource
{
	/// @brief Which profile dictionary - and, for a dpad, which component - supplies this axis.
	enum class Kind : uint8_t
	{
		ProfileAxis, ///< Sample profile.axes[key].value.
		DpadAxis, ///< Sample profile.dpads[key].xAxis/yAxis.value, per isXAxis.
		TriggerButton ///< Sample profile.buttons[key].value (an analog trigger read as an axis).
	};

	Kind kind = Kind::ProfileAxis;
	__strong NSString* key = nil; ///< Key into profile.axes, profile.dpads or profile.buttons identifying this element.
	bool isXAxis = true; ///< Which dpad component to sample when kind is DpadAxis.
	DeviceEnums::ElementKey element{}; ///< Identity this axis is published under.
};

std::vector<AxisSource> GetAxisSources( GCController* controller );
Events::Axis Handle( const AxisSource& source, GCPhysicalInputProfile* profile );
}

namespace SwitchHandling
{
/**
* @brief Where a single published switch is sampled from.
*
* Stores the dpad's dictionary key rather than the live GCControllerDirectionPad, for the same
* reason as ButtonHandling::ButtonSource above.
*/
struct SwitchSource
{
	__strong NSString* key = nil; ///< Key into profile.dpads identifying this element.
	DeviceEnums::ElementKey element{}; ///< Identity this switch is published under.
};

std::vector<SwitchSource> GetSwitchSources( GCController* controller );
Events::Switch Handle( const SwitchSource& source, GCPhysicalInputProfile* profile );
}

#endif // __APPLE__
