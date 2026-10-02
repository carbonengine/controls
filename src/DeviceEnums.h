// Copyright © 2026 CCP ehf.

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
enum class DeviceFamily : uint32_t
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
enum class InputElementDescriptor : uint32_t
{
	Unknown = 0,

	// Face buttons, by compass slot.
	FaceSouth = 140,
	FaceEast,
	FaceWest,
	FaceNorth,

	// Shoulders, triggers and stick buttons.
	LeftShoulder = 240,
	LeftTriggerButton,
	LeftStickButton,
	RightShoulder,
	RightTriggerButton,
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
	LetterB,
	LetterC,
	LetterD,
	LetterE,
	LetterF,
	LetterG,
	LetterH,
	LetterI,
	LetterJ,
	LetterK,
	LetterL,
	LetterM,
	LetterN,
	LetterO,
	LetterP,
	LetterQ,
	LetterR,
	LetterS,
	LetterT,
	LetterU,
	LetterV,
	LetterW,
	LetterX,
	LetterY,
	LetterZ,

	// Vendor-neutral numeric labels.
	Number0 = 800,
	Number1,
	Number2,
	Number3,
	Number4,
	Number5,
	Number6,
	Number7,
	Number8,
	Number9,

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
 * @brief Identifies one published element on a device.
 *
 * The descriptor alone identifies a named element. @c index disambiguates elements that
 * would otherwise share a descriptor: Unknown elements, and the d-pad switches of a device
 * that exposes more than one.
 *
 * A handler resolves the key once when a device connects and publishes it in
 * DeviceIdentifier, so consumers read the same identity the state snapshots are keyed by
 * instead of re-deriving it from the descriptor list.
 */
struct ElementKey
{
	InputElementDescriptor descriptor = InputElementDescriptor::Unknown;
	uint32_t index = 0;
};

inline bool operator<( const ElementKey& lhs, const ElementKey& rhs )
{
	return lhs.descriptor != rhs.descriptor ? lhs.descriptor < rhs.descriptor : lhs.index < rhs.index;
}

inline bool operator==( const ElementKey& lhs, const ElementKey& rhs )
{
	return lhs.descriptor == rhs.descriptor && lhs.index == rhs.index;
}

/**
 * @brief Builds the key for the next button or axis a handler publishes.
 *
 * Named elements are identified by their descriptor alone and always get index 0. Unknown
 * elements are numbered from 0 in publication order via @p unknownCounter.
 */
inline ElementKey MakeElementKey( InputElementDescriptor descriptor, uint32_t& unknownCounter )
{
	return { descriptor, descriptor == InputElementDescriptor::Unknown ? unknownCounter++ : 0 };
}

/**
 * @brief Describes the rumble/vibration capabilities of an input device.
 */
struct RumbleCapacity
{
	bool hasLowFrequencyRumble = false; ///< Whether the device supports low-frequency rumble.
	bool hasHighFrequencyRumble = false; ///< Whether the device supports high-frequency rumble.
	bool hasLeftTriggerRumble = false; ///< Whether the left trigger supports rumble.
	bool hasRightTriggerRumble = false; ///< Whether the right trigger supports rumble.
	uint32_t rumbleMotorCount = 0; ///< Number of rumble motors available on the device.
};

/**
 * @brief Identifies an input device and describes its capabilities.
 *
 * Contains metadata such as the device name, vendor/product IDs, and
 * the number of buttons, axes, and switches the device exposes.
 */
struct DeviceIdentifier
{
	BlueSharedString name; ///< Human-readable device name.
	BlueSharedString deviceID; ///< Unique identifier for this device instance.
	BlueSharedString vendorID; ///< Vendor identifier.
	BlueSharedString productID; ///< Product identifier.

	/// @brief Hardware family. Presentation metadata only; element identity
	/// does not depend on it.
	DeviceFamily family = DeviceFamily::Unknown;

	/// Published button keys, in the order the handler emits them into Events::State.
	std::vector<ElementKey> buttonElements;
	/// Published axis keys, in the order the handler emits them into Events::State.
	std::vector<ElementKey> axisElements;
	/// Published switch keys, in the order the handler emits them into Events::State.
	std::vector<ElementKey> switchElements;

	RumbleCapacity rumbleCapacity{}; ///< Rumble capabilities of the device.
};

}
