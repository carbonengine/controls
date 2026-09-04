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
 * @brief Abstract physical slot an element occupies, independent of glyph.
 *
 * This is an intermediate representation: platform handlers fold their native
 * labels down to a position, and ResolveElement() then combines the position
 * with the DeviceFamily to produce the glyph-flavoured InputElement.
 */
enum class ElementPosition : uint16_t
{
    Unknown = 0,

    FaceSouth,
    FaceEast,
    FaceWest,
    FaceNorth,

    LeftShoulder,
    LeftTrigger,
    LeftStickButton,
    RightShoulder,
    RightTrigger,
    RightStickButton,

    Start,
    Select,
    Guide,

    DPadUp,
    DPadDown,
    DPadLeft,
    DPadRight,

    PaddleLeft1,
    PaddleLeft2,
    PaddleRight1,
    PaddleRight2,

    LeftStickX,
    LeftStickY,
    RightStickX,
    RightStickY,
    LeftTriggerAxis,
    RightTriggerAxis,

    DPad
};

/**
 * @brief Canonical, deterministic identifier for a single input element.
 *
 * These values are stable localization keys. A given physical controller
 * resolves to the same InputElement on every supported platform, so the value
 * (or its ToKeyString() form) can be used directly as a localization lookup.
 *
 * @warning Never reorder or renumber these values: they are persisted in
 * localization data. Only append new values at the end of a group.
 */
enum class InputElement : uint16_t
{
    Unknown = 0,

    // Face buttons, PlayStation glyphs.
    FaceButtonCross = 100,
    FaceButtonCircle,
    FaceButtonSquare,
    FaceButtonTriangle,

    // Face buttons, Xbox / generic letter glyphs.
    FaceButtonA = 120,
    FaceButtonB,
    FaceButtonX,
    FaceButtonY,

    // Shoulders, triggers and stick buttons, PlayStation glyphs.
    L1 = 200,
    L2,
    L3,
    R1,
    R2,
    R3,

    // Shoulders, triggers and stick buttons, Xbox glyphs.
    LB = 220,
    LT,
    LSB,
    RB,
    RT,
    RSB,

    // System buttons.
    Start = 300,
    Back,
    Select,
    Menu,
    View,
    Options,
    Share,
    Guide,
    Home,
    Mode,

    // Directional pad.
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

    // Vendor-neutral letter labels.
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
 * @brief Returns the stable string form of an InputElement.
 *
 * The returned text matches the enumerator name (for example
 * "FaceButtonCross") and is suitable for use as a localization key.
 * Unmapped elements return "Unknown".
 */
const char* ToKeyString( InputElement element );

/**
 * @brief Combines an abstract element position with a device family to produce
 * the glyph-flavoured canonical element.
 *
 * Because @p family is derived from hardware identity rather than from the
 * label the OS reported, a DualSense resolves to FaceButtonCross on both
 * Windows and macOS even though the two platforms report different labels.
 *
 * @param position The abstract slot the element occupies.
 * @param family The hardware family of the owning device.
 * @return The canonical element, or InputElement::Unknown if unresolvable.
 */
InputElement ResolveElement( ElementPosition position, DeviceFamily family );

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

	/// @brief Hardware family, used to resolve glyph-flavoured element names.
	DeviceFamily family = DeviceFamily::Unknown;

	/// Canonical button identifiers; InputElement::Unknown marks an unmapped button.
	std::vector<InputElement> buttonElements;
	/// Canonical axis identifiers; InputElement::Unknown marks an unmapped axis.
	std::vector<InputElement> axisElements;
	/// Canonical switch identifiers; InputElement::Unknown marks an unmapped switch.
	std::vector<InputElement> switchElements;

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
