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

struct DeviceIdentifier
{
	BlueSharedStringW name;
	DeviceId deviceID = static_cast<DeviceId>(-1);
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
		deviceID(other.deviceID),
		deviceType(other.deviceType),
		manufacturer(other.manufacturer),
		product(other.product),
		buttonCount(other.buttonCount),
		axisCount(other.axisCount),
		switchCount(other.switchCount),
		rumbleSupported(other.rumbleSupported) {};

};

}