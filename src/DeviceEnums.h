#pragma once

#include "StdAfx.h"
#include <numeric>

/**
 * @brief Enumerations and structures describing input device properties.
 */
namespace DeviceEnums
{

/**
 * @brief Describes the rumble/vibration capabilities of an input device.
 */
struct RumbleCapacity
{
	bool hasLowFrequencyRumble = false;  ///< Whether the device supports low-frequency rumble.
	bool hasHighFrequencyRumble = false; ///< Whether the device supports high-frequency rumble.
	bool hasLeftTriggerRumble = false;   ///< Whether the left trigger supports rumble.
	bool hasRightTriggerRumble = false;  ///< Whether the right trigger supports rumble.

	RumbleCapacity() = default;

	/**
	 * @brief Copy constructor.
	 * @param other The RumbleCapacity to copy from.
	 */
	RumbleCapacity( const RumbleCapacity& other ) :
		hasLowFrequencyRumble( other.hasLowFrequencyRumble ),
		hasHighFrequencyRumble( other.hasHighFrequencyRumble ),
		hasLeftTriggerRumble( other.hasLeftTriggerRumble ),
		hasRightTriggerRumble( other.hasRightTriggerRumble );
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

	uint32_t buttonCount = 0; ///< Number of buttons on the device.
	uint32_t axisCount = 0;   ///< Number of analog axes on the device.
	uint32_t switchCount = 0; ///< Number of hat/d-pad switches on the device.

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
		buttonCount( other.buttonCount ),
		axisCount( other.axisCount ),
		switchCount( other.switchCount ),
		rumbleCapacity( other.rumbleCapacity ) {};
};

}