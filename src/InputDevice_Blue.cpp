#include "InputDevice.h"

BLUE_DEFINE( InputDevice );
BLUE_DEFINE( InputDeviceIdentifier );

namespace
{
Be::VarChooser DeviceTypeChooser[] = {
	{ "Gamepad", BeCast( DeviceEnums::DeviceType::DeviceType_Gamepad ), "A gamepad device" },
	{ "Flight Stick", BeCast( DeviceEnums::DeviceType::DeviceType_FlightStick ), "A flight stick device" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "DeviceType", DeviceEnums::DeviceType, DeviceTypeChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );
}

const Be::ClassInfo* InputDeviceIdentifier::ExposeToBlue()
{
	EXPOSURE_BEGIN( InputDeviceIdentifier, "Input Device Identifier" )
		MAP_INTERFACE( InputDeviceIdentifier )
		MAP_ATTRIBUTE( "name", identifier.name, "The name of the device", Be::READ )
		MAP_ATTRIBUTE( "deviceID", identifier.deviceID, "The ID of the device", Be::READ )
		MAP_ATTRIBUTE_WITH_CHOOSER( "deviceType", identifier.deviceType, "The type of the device", Be::READ | Be::ENUM, DeviceTypeChooser )
		MAP_ATTRIBUTE( "manufacturer", identifier.manufacturer, "The manufacturer of the device", Be::READ )
		MAP_ATTRIBUTE( "product", identifier.product, "The product name of the device", Be::READ )
	EXPOSURE_END()
}

const Be::ClassInfo* InputDevice::ExposeToBlue()
{
	EXPOSURE_BEGIN( InputDevice, "Input Device" )
		MAP_INTERFACE( InputDevice )
		MAP_METHOD_AND_WRAP( "GetStateAsJson", GetStateAsJson, "Gets the current state of the device as a JSON string" )
		MAP_ATTRIBUTE( "identifier", m_deviceIdentifier, "The identifier of the device", Be::READ )
		MAP_ATTRIBUTE( "triggers", m_triggers, "The triggers associated with this device", Be::READ )
		
	EXPOSURE_END()
}