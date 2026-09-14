#pragma once
#include "../StdAfx.h"
#include "../DeviceEnums.h"


/**
 * @brief Namespace containing event types and state structures for input devices.
 */
namespace Events
{
/// @brief Threshold below which axis value changes are ignored.
const float AXIS_THRESHOLD = 0.01f;

/// @brief The time in microseconds after which a button state changes from Pressed to Held.
static uint64_t g_holdTimeInMicroSeconds = 300 * 1000;

/**
 * @brief Represents the logical state of a button.
 *
 * State transitions:
 * - Up: Button is not pressed and was not pressed previously.
 * - Down: Button is pressed and was pressed previously.
 * - Released: Button was released after being held longer than the hold threshold.
 * - Held: Button has been held down longer than the hold threshold.
 * - Pressed: Button was released before the hold threshold elapsed (a short press/tap).
 */
enum class ButtonState
{
	Up,       ///< Button is not pressed.
	Down,     ///< Button is continuously held down.
	Released, ///< Button was released after being held.
	Held,     ///< Button has been held past the hold threshold.
	Pressed   ///< Button was tapped (released before hold threshold).
};

/**
 * @brief Represents a hat/d-pad switch position.
 */
enum class SwitchPosition : uint32_t
{
	Center,    ///< Centered (neutral) position.
	Up,        ///< Up position.
	UpRight,   ///< Up-right diagonal position.
	Right,     ///< Right position.
	DownRight, ///< Down-right diagonal position.
	Down,      ///< Down position.
	DownLeft,  ///< Down-left diagonal position.
	Left,      ///< Left position.
	UpLeft,    ///< Up-left diagonal position.
	Any        ///< Matches any non-center position.
};

/**
 * @brief Snapshot of a single button's raw hardware state.
 */
struct Button
{
	bool matched = false;  ///< Whether this button has already been claimed by an event trigger.
	bool pressed = false;  ///< Whether the button is currently pressed.
	DeviceEnums::InputElementDescriptor descriptor = DeviceEnums::InputElementDescriptor::Unknown; ///< Descriptor for the button's input element.
	uint32_t index = 0;    ///< Index of the button within the device.
};

/**
 * @brief Snapshot of a single analog axis's raw hardware state.
 */
struct Axis
{
	bool matched = false; ///< Whether this axis has already been claimed by an event trigger.
	float value = 0.0f;   ///< Current axis value, typically in the range [-1.0, 1.0].
	DeviceEnums::InputElementDescriptor descriptor = DeviceEnums::InputElementDescriptor::Unknown; ///< Descriptor for the axis's input element.
	uint32_t index = 0;    ///< Index of the axis within the device.
};

/**
 * @brief Snapshot of a single hat/d-pad switch's raw hardware state.
 */
struct Switch
{
	bool matched = false;                              ///< Whether this switch has already been claimed by an event trigger.
	SwitchPosition position = SwitchPosition::Center;  ///< Current switch position.
	DeviceEnums::InputElementDescriptor descriptor = DeviceEnums::InputElementDescriptor::Unknown; ///< Descriptor for the switch's input element.
	uint32_t index = 0; ///< Index of the switch within the device.
};

/**
 * @brief Complete snapshot of an input device's state at a point in time.
 */
struct State
{
	uint64_t timestamp = 0;            ///< Timestamp in microseconds when this state was captured.
	std::vector<Button> buttons;       ///< Button states for all buttons on the device.
	std::vector<Axis> axis;            ///< Axis states for all analog axes on the device.
	std::vector<Switch> switches;      ///< Switch states for all hat/d-pad switches on the device.
};

/**
 * @brief Rumble/vibration motor intensities to apply to a device.
 *
 * All values are normalized in the range [0.0, 1.0].
 */
struct Rumble
{
	float lowFrequency = 0.0f;  ///< Low-frequency rumble motor intensity.
	float highFrequency = 0.0f; ///< High-frequency rumble motor intensity.
	float leftTrigger = 0.0f;   ///< Left trigger rumble motor intensity.
	float rightTrigger = 0.0f;  ///< Right trigger rumble motor intensity.

	/**
	 * @brief Checks whether all rumble values are zero.
	 * @return true if all motor intensities are zero, false otherwise.
	 */
	bool empty() const;
};

/**
 * @brief Assigns the disambiguating index for an input element.
 *
 * Named elements are uniquely identified by their descriptor alone, so they always
 * get index 0. Only Unknown elements need an index to tell them apart; they are
 * numbered from 0 in publication order via @p unknownCounter.
 *
 * Must be used by both the DeviceIdentifier element list and the per-reading state
 * snapshot so the two agree on what index means.
 *
 * @param descriptor The element's descriptor.
 * @param unknownCounter Running count of Unknown elements published so far; incremented when used.
 * @return The index to publish for this element.
 */
inline uint32_t AssignElementIndex( DeviceEnums::InputElementDescriptor descriptor, uint32_t& unknownCounter )
{
    return descriptor == DeviceEnums::InputElementDescriptor::Unknown ? unknownCounter++ : 0;
}

/**
 * @brief Returns the current timestamp in microseconds.
 * @return Current steady-clock time in microseconds since epoch.
 */
uint64_t GetTimestamp();

extern const Be::VarChooser ButtonStateChooser[];
}
