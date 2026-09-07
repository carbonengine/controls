#pragma once

#include "StdAfx.h"
#include <numeric>

/**
 * @brief Enumerations and structures describing input device properties.
 */
namespace DeviceEnums
{

/**
 * @brief Hardware family an input device belongs to.
 *
 * Resolved from hardware identity (vendor/product ID on Windows, product
 * category on macOS) rather than from whatever label the OS reports, so the
 * same physical controller resolves to the same family on every platform.
 */
enum class DeviceFamily : uint8_t
{
    Unknown = 0,
    Generic,
    Xbox,
    PlayStation,
    Nintendo
};

/**
 * @brief Canonical, deterministic identifier for a single input element.
 *
 * Values identify the abstract slot an element occupies, never the glyph
 * printed on it. A DualSense and an Xbox pad both report FaceSouth for their
 * lower face button, so bindings persisted against these values stay valid
 * when the user swaps controllers mid-session.
 *
 * Glyph flavour is a presentation concern, applied by ToGlyphKeyString().
 *
 * @warning Game clients may use this for mapping/localization etc. Be careful when changing these values.
 */
enum class InputElementDescriptor : uint16_t
{
    Unknown = 0,

    // Face buttons, by compass slot.
    FaceSouth = 140,
    FaceEast,
    FaceWest,
    FaceNorth,

    // Shoulders, triggers and stick buttons.
    LeftShoulder = 240,
    LeftTrigger,
    LeftStickButton,
    RightShoulder,
    RightTrigger,
    RightStickButton,

    // System buttons.
    Start = 320,
    Select,
    Guide,

    // Directional pad. No family variation.
    DPadUp = 400,
    DPadDown,
    DPadLeft,
    DPadRight,
    DPad,

    // Paddles.
    PaddleLeft1 = 500,
    PaddleLeft2,
    PaddleRight1,
    PaddleRight2,

    // Analog axes.
    LeftStickX = 600,
    LeftStickY,
    RightStickX,
    RightStickY,
    LeftTriggerAxis,
    RightTriggerAxis,

    // Label-defined elements: devices whose controls are identified purely by
    // the glyph printed on them (arcade sticks, flight gear). Not positional,
    // and carrying no family variation.
    LetterA = 700,
    LetterB, LetterC, LetterD, LetterE, LetterF, LetterG, LetterH, LetterI,
    LetterJ, LetterK, LetterL, LetterM, LetterN, LetterO, LetterP, LetterQ,
    LetterR, LetterS, LetterT, LetterU, LetterV, LetterW, LetterX, LetterY,
    LetterZ,

    // Vendor-neutral numeric labels.
    Number0 = 800,
    Number1, Number2, Number3, Number4,
    Number5, Number6, Number7, Number8, Number9,

    // Vendor-neutral arrow labels.
    ArrowUp = 900,
    ArrowUpRight,
    ArrowRight,
    ArrowDownRight,
    ArrowDown,
    ArrowDownLeft,
    ArrowLeft,
    ArrowUpLeft,
    ArrowUpDown,
    ArrowLeftRight,
    ArrowUpDownLeftRight,
    ArrowClockwise,
    ArrowCounterClockwise,
    ArrowReturn,

    // Vendor-neutral icon labels.
    IconBranding = 1000,
    IconStar,
    IconPlus,
    IconMinus,
    IconSuspension,
    IconDialClockwise,
    IconDialCounterClockwise,
    IconSliderLeftRight,
    IconSliderUpDown,
    IconWheelUpDown
};

/**
 * @brief Returns the stable, family-agnostic string form of an InputElement.
 *
 * Matches the enumerator name (for example "FaceSouth"). This is the form to
 * persist. Unmapped elements return "Unknown".
 */
const char* ToKeyString( InputElementDescriptor element );

/**
 * @brief Returns the glyph-flavoured localization key for an element as it is
 * printed on a given family of hardware.
 *
 * FaceSouth yields "FaceButtonCross" on PlayStation and "FaceButtonA"
 * elsewhere. Elements with no family variation fall through to ToKeyString(),
 * so this is safe to call for any element.
 *
 * Presentation only: never persist the result, and never use it to identify
 * an element.
 *
 * @param element The abstract element.
 * @param family The hardware family of the owning device.
 */
const char* ToGlyphKeyString( InputElementDescriptor element, DeviceFamily family );

/**
 * @brief Describes the rumble/vibration capabilities of an input device.
 */
struct RumbleCapacity
{
	bool hasLowFrequencyRumble = false;  ///< Whether the device supports low-frequency rumble.
	bool hasHighFrequencyRumble = false; ///< Whether the device supports high-frequency rumble.
	bool hasLeftTriggerRumble = false;   ///< Whether the left trigger supports rumble.
	bool hasRightTriggerRumble = false;  ///< Whether the right trigger supports rumble.
	uint32_t rumbleMotorCount = 0;       ///< Number of rumble motors available on the device.

	RumbleCapacity() = default;

	/**
	 * @brief Copy constructor.
	 * @param other The RumbleCapacity to copy from.
	 */
	RumbleCapacity( const RumbleCapacity& other ) :
		hasLowFrequencyRumble( other.hasLowFrequencyRumble ),
		hasHighFrequencyRumble( other.hasHighFrequencyRumble ),
		hasLeftTriggerRumble( other.hasLeftTriggerRumble ),
		hasRightTriggerRumble( other.hasRightTriggerRumble ),
		rumbleMotorCount( other.rumbleMotorCount ) {};
};

/**
 * @brief Identifies an input device and describes its capabilities.
 *
 * Contains metadata such as the device name, vendor/product IDs, and
 * the number of buttons, axes, and switches the device exposes.
 */
struct DeviceIdentifier
{
	BlueSharedString name;      ///< Human-readable device name.
	BlueSharedString deviceID;  ///< Unique identifier for this device instance.
	BlueSharedString vendorID;  ///< Vendor identifier.
	BlueSharedString productID; ///< Product identifier.

	/// @brief Hardware family. Presentation metadata only; element identity
	/// does not depend on it.
	DeviceFamily family = DeviceFamily::Unknown;

	/// Canonical button identifiers; InputElement::Unknown marks an unmapped button.
	std::vector<InputElementDescriptor> buttonElements;
	/// Canonical axis identifiers; InputElement::Unknown marks an unmapped axis.
	std::vector<InputElementDescriptor> axisElements;
	/// Canonical switch identifiers; InputElement::Unknown marks an unmapped switch.
	std::vector<InputElementDescriptor> switchElements;

	RumbleCapacity rumbleCapacity {}; ///< Rumble capabilities of the device.

	DeviceIdentifier() = default;

	/**
	 * @brief Copy constructor.
	 * @param other The DeviceIdentifier to copy from.
	 */
	DeviceIdentifier( const DeviceIdentifier& other ) :
		name( other.name ),
		deviceID( other.deviceID ),
		vendorID( other.vendorID ),
		productID( other.productID ),
		family( other.family ),
		buttonElements( other.buttonElements ),
		axisElements( other.axisElements ),
		switchElements( other.switchElements ),
		rumbleCapacity( other.rumbleCapacity )
	{};
};

}
