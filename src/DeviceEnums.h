#pragma once
#include "StdAfx.h"
#include <numeric>

namespace DeviceEnums
{
typedef uint32_t DeviceId;

// If we support another os, then we may need to ifdef this struct out to something else
struct RawDeviceId
{
	std::vector<uint32_t> value;
#ifdef WIN32
	RawDeviceId() :
		value( APP_LOCAL_DEVICE_ID_SIZE, 0 ) {};

	auto operator=( const APP_LOCAL_DEVICE_ID deviceID )
	{
		std::copy( deviceID.value, deviceID.value + APP_LOCAL_DEVICE_ID_SIZE, value.begin() );
		return *this;
	}

	auto operator=( const RawDeviceId& other )
	{
		std::copy( other.value.begin(), other.value.end(), value.begin() );
		return *this;
	}
#endif
};

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
	BlueSharedStringW name;
	DeviceId deviceID = static_cast<DeviceId>( -1 );
	RawDeviceId rawDeviceId;
	BlueSharedString vendorID;
	BlueSharedString productID;

	uint32_t buttonCount;
	uint32_t axisCount;
	uint32_t switchCount;

	RumbleCapacity rumbleCapacity;

	DeviceIdentifier() = default;
	DeviceIdentifier( const DeviceIdentifier& other ) :
		name( other.name ),
		deviceID( other.deviceID ),
		rawDeviceId( other.rawDeviceId ),
		vendorID( other.vendorID ),
		productID( other.productID ),
		buttonCount( other.buttonCount ),
		axisCount( other.axisCount ),
		switchCount( other.switchCount ),
		rumbleCapacity( other.rumbleCapacity ) {};
};

}