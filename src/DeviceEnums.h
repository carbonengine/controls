#pragma once

#include "StdAfx.h"
#include <numeric>

namespace DeviceEnums
{

struct RumbleCapacity
{
	bool hasLowFrequencyRumble = false;
	bool hasHighFrequencyRumble = false;
	bool hasLeftTriggerRumble = false;
	bool hasRightTriggerRumble = false;
	uint32_t rumbleMotorCount = 0;

	RumbleCapacity() = default;
	RumbleCapacity( const RumbleCapacity& other ) :
		hasLowFrequencyRumble( other.hasLowFrequencyRumble ),
		hasHighFrequencyRumble( other.hasHighFrequencyRumble ),
		hasLeftTriggerRumble( other.hasLeftTriggerRumble ),
		hasRightTriggerRumble( other.hasRightTriggerRumble ),
		rumbleMotorCount( other.rumbleMotorCount ) {};
};

struct DeviceIdentifier
{
	BlueSharedString name;
	BlueSharedString deviceID;
	BlueSharedString vendorID;
	BlueSharedString productID;

	uint32_t buttonCount = 0;
	uint32_t axisCount = 0;
	uint32_t switchCount = 0;

	RumbleCapacity rumbleCapacity {};

	DeviceIdentifier() = default;
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