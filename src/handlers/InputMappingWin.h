#pragma once
#ifdef WIN32
#include "IInputHandler.h"
#include <Windows.h>   
#include <gameinput_v3.h>

#include <array>	
#include <memory>
#include <mutex>
#include <optional>

using namespace GameInput::v3;

namespace InputMapping
{

// Canonical ordering of the gamepad view's buttons.
//
// GameInputGamepadInfo::supportedLayout advertises exactly which of these a device
// actually has - even a generic pad reports a layout - so the published button list is
// built from that mask instead of a hard-coded set. Entries are emitted in this order and
// ReadDeviceState samples the same order, keeping indices aligned.
struct GamepadButtonMapping
{
	GameInputGamepadButtons mask;
	DeviceEnums::InputElementDescriptor element;
};

// GameInput's neutral label families (letters, numbers, arrows and the remaining icons)
// describe controls that are identified by their printed glyph rather than by a slot, so
// they carry over to InputElement one-to-one.
DeviceEnums::InputElementDescriptor ToNeutralElement( GameInputLabel label );

// Folds the several parallel GameInputLabel naming families (Xbox letters, PlayStation
// icons, generic words, LB/L1 style abbreviations) down to the abstract slot the element
// occupies. The glyph flavour the driver reported is deliberately discarded: identity is
// positional, and the flavour is re-applied at display time from the device family.
DeviceEnums::InputElementDescriptor ToElement( GameInputLabel label );

// Hardware families are resolved from the USB vendor ID rather than from the labels the
// driver reports, so the same controller yields the same glyph flavour regardless of how
// the OS chose to describe it.
DeviceEnums::DeviceFamily GetDeviceFamily( uint16_t vendorId );

constexpr GamepadButtonMapping GAMEPAD_BUTTONS[] = {
	{ GameInputGamepadMenu, DeviceEnums::InputElementDescriptor::Start },
	{ GameInputGamepadView, DeviceEnums::InputElementDescriptor::Select },
	{ GameInputGamepadA, DeviceEnums::InputElementDescriptor::FaceSouth },
	{ GameInputGamepadB, DeviceEnums::InputElementDescriptor::FaceEast },
	{ GameInputGamepadX, DeviceEnums::InputElementDescriptor::FaceWest },
	{ GameInputGamepadY, DeviceEnums::InputElementDescriptor::FaceNorth },
	// C and Z occupy no canonical slot, so they have no canonical identity. They are still
	// published so state indices stay aligned with the rest of the layout.
	{ GameInputGamepadC, DeviceEnums::InputElementDescriptor::Unknown },
	{ GameInputGamepadZ, DeviceEnums::InputElementDescriptor::Unknown },
	{ GameInputGamepadLeftShoulder, DeviceEnums::InputElementDescriptor::LeftShoulder },
	{ GameInputGamepadRightShoulder, DeviceEnums::InputElementDescriptor::RightShoulder },
	{ GameInputGamepadLeftTriggerButton, DeviceEnums::InputElementDescriptor::LeftTriggerButton },
	{ GameInputGamepadRightTriggerButton, DeviceEnums::InputElementDescriptor::RightTriggerButton },
	{ GameInputGamepadLeftThumbstick, DeviceEnums::InputElementDescriptor::LeftStickButton },
	{ GameInputGamepadRightThumbstick, DeviceEnums::InputElementDescriptor::RightStickButton },
	{ GameInputGamepadPaddleLeft1, DeviceEnums::InputElementDescriptor::PaddleLeft1 },
	{ GameInputGamepadPaddleLeft2, DeviceEnums::InputElementDescriptor::PaddleLeft2 },
	{ GameInputGamepadPaddleRight1, DeviceEnums::InputElementDescriptor::PaddleRight1 },
	{ GameInputGamepadPaddleRight2, DeviceEnums::InputElementDescriptor::PaddleRight2 },
	{ GameInputGamepadDPadDown, DeviceEnums::InputElementDescriptor::DPadDown },
	{ GameInputGamepadDPadUp, DeviceEnums::InputElementDescriptor::DPadUp },
	{ GameInputGamepadDPadLeft, DeviceEnums::InputElementDescriptor::DPadLeft },
	{ GameInputGamepadDPadRight, DeviceEnums::InputElementDescriptor::DPadRight }
};

// The subset of GAMEPAD_BUTTONS the device actually exposes, in publication order.
std::vector<GameInputGamepadButtons> GetGamepadButtonMasks( GameInputGamepadButtons supportedLayout );
DeviceEnums::InputElementDescriptor GetGamepadButtonDescriptor( GameInputGamepadButtons buttonMask );

// Converts the 32 byte device ID from GameInput into a string to be used as a unique identifier for devices.BlueSharedString GetDeviceIDAsString( APP_LOCAL_DEVICE_ID deviceId )
BlueSharedString GetDeviceIDAsString( APP_LOCAL_DEVICE_ID deviceId );
std::string GetStringValueFromHKLM( const std::string& regSubKey, const std::string& regValue );
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
	/// @brief The reading view supplying this button.
	enum class Kind : uint8_t
	{
		None, ///< Not present on this device; always reads as unpressed.
		GamepadMask, ///< Sample the gamepad state's button mask.
		RawIndex ///< Sample the raw controller button array.
	};

	Kind kind = Kind::None;
	GameInputGamepadButtons mask = GameInputGamepadNone; ///< Mask to test when kind is GamepadMask.
	uint32_t rawIndex = 0; ///< Raw controller index when kind is RawIndex.
	uint32_t elementIndex = 0; ///< Published index; only Unknown descriptors are numbered, everything else is 0.
	DeviceEnums::InputElementDescriptor descriptor = DeviceEnums::InputElementDescriptor::Unknown;
};

std::vector<ButtonSource> GetButtonSources( const GameInputControllerInfo* controllerInfo, const GameInputGamepadInfo* gamepadInfo );
Events::Button Handle( const ButtonSource& source, const GameInputGamepadState& gamepadState, const bool* rawButtons, uint32_t buttonCount );
}

namespace AxisHandling
{
/**
* @brief Where a single published axis is sampled from.
*/
struct AxisSource
{
	/// @brief The reading view supplying this axis.
	enum class Kind : uint8_t
	{
		GamepadField, ///< Read a named GameInputGamepadState field.
		RawIndex ///< Sample the raw controller axis array.
	};

	Kind kind = Kind::RawIndex;
	uint32_t rawIndex = 0; ///< Raw controller index when kind is RawIndex.
	uint32_t elementIndex = 0; ///< Published index; only Unknown descriptors are numbered, everything else is 0.
	DeviceEnums::InputElementDescriptor descriptor = DeviceEnums::InputElementDescriptor::Unknown;
};

std::vector<AxisSource> GetAxisSources( const GameInputControllerInfo* controllerInfo, const GameInputGamepadInfo* gamepadInfo );
Events::Axis Handle( const AxisSource& source, const GameInputGamepadState& gamepadState, const float* rawAxes, uint32_t axisCount );
}

namespace SwitchHandling
{
std::vector<uint32_t> GetSwitchSources( const GameInputControllerInfo* controllerInfo );
Events::Switch Handle( const uint32_t& sourceIndex, const GameInputSwitchPosition* rawSwitches, uint32_t switchCount );
}


#endif
