#pragma once
#include "StdAfx.h"
#include <numeric>

namespace DeviceEnums
{
typedef uint32_t DeviceId;

enum DeviceType
{
	DeviceType_Unknown = 0,
	DeviceType_Gamepad,
	DeviceType_Controller,
};

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

struct DeviceIdentifier
{
	BlueSharedStringW name;
	DeviceId deviceID = static_cast<DeviceId>( -1 );
	RawDeviceId rawDeviceId;
	DeviceType deviceType = DeviceType_Unknown;
	BlueSharedString manufacturer;
	BlueSharedString product;

	uint32_t buttonCount;
	uint32_t axisCount;
	uint32_t switchCount;

	bool rumbleSupported;

	DeviceIdentifier() = default;
	DeviceIdentifier( const DeviceIdentifier& other ):
		name(other.name),
		deviceID( other.deviceID ),
		rawDeviceId( other.rawDeviceId ),
		deviceType(other.deviceType),
		manufacturer(other.manufacturer),
		product(other.product),
		buttonCount(other.buttonCount),
		axisCount(other.axisCount),
		switchCount(other.switchCount),
		rumbleSupported(other.rumbleSupported) {};

};

}