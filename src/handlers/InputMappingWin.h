#pragma once
#ifdef WIN32
#include "IInputHandler.h"
#include <Windows.h>
#include <gameinput_v3.h>

#include <array>
#include <memory>
#include <mutex>
#include <optional>

/// Single alias rather than `using namespace GameInput::v3;` so including this header does not
/// pull the whole GameInput v3 namespace into unrelated translation units.
namespace GameInputV3 = GameInput::v3;

namespace InputMapping
{

// Folds the several parallel GameInputLabel naming families (Xbox letters, PlayStation
// icons, generic words, LB/L1 style abbreviations) down to the abstract slot the element
// occupies. The glyph flavour the driver reported is deliberately discarded: identity is
// positional, and the flavour is re-applied at display time from the device family.
DeviceEnums::InputElementDescriptor ToElement( GameInputV3::GameInputLabel label );

// Hardware families are resolved from the USB vendor ID rather than from the labels the
// driver reports, so the same controller yields the same glyph flavour regardless of how
// the OS chose to describe it.
DeviceEnums::DeviceFamily GetDeviceFamily( uint16_t vendorId );

// Converts the 32 byte device ID from GameInput into a string to be used as a unique identifier for devices.
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
	GameInputV3::GameInputGamepadButtons mask = GameInputV3::GameInputGamepadNone; ///< Mask to test when kind is GamepadMask.
	uint32_t rawIndex = 0; ///< Raw controller index when kind is RawIndex.
	DeviceEnums::ElementKey key{}; ///< Identity this button is published under.
};

std::vector<ButtonSource> GetButtonSources( const GameInputV3::GameInputControllerInfo* controllerInfo, const GameInputV3::GameInputGamepadInfo* gamepadInfo );
Events::Button Handle( const ButtonSource& source, const GameInputV3::GameInputGamepadState& gamepadState, const bool* rawButtons, uint32_t buttonCount );
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
		GamepadField, ///< Read the GameInputGamepadState member named by @c gamepadField.
		RawIndex ///< Sample the raw controller axis array.
	};

	Kind kind = Kind::RawIndex;
	float GameInputV3::GameInputGamepadState::* gamepadField = nullptr; ///< Member to read when kind is GamepadField.
	uint32_t rawIndex = 0; ///< Raw controller index when kind is RawIndex.
	DeviceEnums::ElementKey key{}; ///< Identity this axis is published under.
};

std::vector<AxisSource> GetAxisSources( const GameInputV3::GameInputControllerInfo* controllerInfo, const GameInputV3::GameInputGamepadInfo* gamepadInfo );
Events::Axis Handle( const AxisSource& source, const GameInputV3::GameInputGamepadState& gamepadState, const float* rawAxes, uint32_t axisCount );
}

namespace SwitchHandling
{
/**
* @brief Where a single published switch is sampled from.
*/
struct SwitchSource
{
	uint32_t rawIndex = 0; ///< Raw controller switch index.
	DeviceEnums::ElementKey key{}; ///< Identity this switch is published under.
};

std::vector<SwitchSource> GetSwitchSources( const GameInputV3::GameInputControllerInfo* controllerInfo );
Events::Switch Handle( const SwitchSource& source, const GameInputV3::GameInputSwitchPosition* rawSwitches, uint32_t switchCount );
}


#endif
