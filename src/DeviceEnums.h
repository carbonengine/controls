#pragma once
#include "StdAfx.h"

namespace DeviceEnums
{
typedef uint32_t DeviceId;

enum DeviceType
{
	DeviceType_Gamepad,
	DeviceType_FlightStick
};

struct DeviceIdentifier
{
	BlueSharedString name;
	DeviceId deviceID;
	DeviceType deviceType;
	BlueSharedString manufacturer;
	BlueSharedString product;

	DeviceIdentifier() = default;
	DeviceIdentifier( const DeviceIdentifier& other ): name(other.name), deviceID(other.deviceID), deviceType(other.deviceType), manufacturer(other.manufacturer), product(other.product) {};
};

}