#pragma once
#include "StdAfx.h"

namespace DeviceEnums
{
typedef uint32_t DeviceId;

enum DeviceType
{
	DeviceType_Gamepad,
	DeviceType_FlightStick,
	DeviceType_Controller,
};

struct DeviceIdentifier
{
	BlueSharedStringW name;
	DeviceId deviceID;
	DeviceType deviceType;
	BlueSharedString manufacturer;
	BlueSharedString product;

	uint32_t buttonCount;
	uint32_t axisCount;
	uint32_t switchCount;

	bool batteryPowered;
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
		switchCount(other.switchCount) {};

};

}