#include "InputDevice.h"

BLUE_DEFINE( InputDevice );

namespace
{
Be::VarChooser DeviceTypeChooser[] = {
	{ "Unknown", BeCast( DeviceEnums::DeviceType::DeviceType_Unknown ), "An unknown device" },
	{ "Gamepad", BeCast( DeviceEnums::DeviceType::DeviceType_Gamepad ), "A gamepad device" },
	{ "Controller", BeCast( DeviceEnums::DeviceType::DeviceType_Controller ), "A controller device" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "DeviceType", DeviceEnums::DeviceType, DeviceTypeChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );
}


const Be::ClassInfo* InputDevice::ExposeToBlue()
{
	EXPOSURE_BEGIN( InputDevice, "Input Device" )
		MAP_INTERFACE( InputDevice )
		MAP_INTERFACE( IListNotify )
		MAP_ATTRIBUTE( "name", m_deviceIdentifier.name, "The name of the device", Be::READ )
		MAP_ATTRIBUTE( "deviceID", m_deviceIdentifier.deviceID, "The ID of the device", Be::READ )
		MAP_ATTRIBUTE_WITH_CHOOSER( "deviceType", m_deviceIdentifier.deviceType, "The type of the device", Be::READ | Be::ENUM, DeviceTypeChooser )
		MAP_ATTRIBUTE( "manufacturer", m_deviceIdentifier.manufacturer, "The manufacturer of the device", Be::READ )
		MAP_ATTRIBUTE( "product", m_deviceIdentifier.product, "The product name of the device", Be::READ )
		MAP_ATTRIBUTE( "buttonCount", m_deviceIdentifier.buttonCount, "The number of buttons on the device", Be::READ )
		MAP_ATTRIBUTE( "axisCount", m_deviceIdentifier.axisCount, "The number of axes on the device", Be::READ )
		MAP_ATTRIBUTE( "switchCount", m_deviceIdentifier.switchCount, "The number of switches on the device", Be::READ )
		MAP_ATTRIBUTE( "triggers", m_triggers, "The triggers associated with this device", Be::READ )
		MAP_ATTRIBUTE( "rawDeviceID", m_rawDeviceId, "The raw device id as it comes from the os", Be::READ )
	EXPOSURE_END()
}