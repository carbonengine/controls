// Copyright © 2026 CCP ehf.

#include "InputDevice.h"


namespace
{

Be::VarChooser DeviceFamilyChooser[] = {
	{ "Unknown", BeCast( DeviceEnums::DeviceFamily::Unknown ), "Unknown" },
	{ "Generic", BeCast( DeviceEnums::DeviceFamily::Generic ), "Generic" },
	{ "PlayStation", BeCast( DeviceEnums::DeviceFamily::PlayStation ), "PlayStation" },
	{ "Xbox", BeCast( DeviceEnums::DeviceFamily::Xbox ), "Xbox" },
	{ "Nintendo", BeCast( DeviceEnums::DeviceFamily::Nintendo ), "Nintendo" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "DeviceFamily", DeviceEnums::DeviceFamily, DeviceFamilyChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );
}

BLUE_DEFINE( InputDevice );

const Be::ClassInfo* InputDevice::ExposeToBlue()
{
	EXPOSURE_BEGIN( InputDevice, "Input Device" )
		MAP_INTERFACE( InputDevice )
		MAP_ATTRIBUTE( "name", m_deviceIdentifier.name, "The name of the device", Be::READ )
		MAP_ATTRIBUTE_WITH_CHOOSER( "family", m_deviceIdentifier.family, "The family of the device", Be::READ | Be::ENUM, DeviceFamilyChooser )
		MAP_ATTRIBUTE( "deviceID", m_deviceIdentifier.deviceID, "The ID of the device", Be::READ )
		MAP_ATTRIBUTE( "vendorID", m_deviceIdentifier.vendorID, "The manufacturer of the device", Be::READ )
		MAP_ATTRIBUTE( "productID", m_deviceIdentifier.productID, "The product name of the device", Be::READ )
		MAP_ATTRIBUTE( "triggers", m_triggers, "The triggers associated with this device", Be::READ )
		MAP_ATTRIBUTE( "rumbleMotorCount", m_deviceIdentifier.rumbleCapacity.rumbleMotorCount, "The number of rumble motors on the device", Be::READ )
		MAP_ATTRIBUTE( "hasHighFrequencyRumble", m_deviceIdentifier.rumbleCapacity.hasHighFrequencyRumble, "Indicates if the device has a high frequency rumble motor", Be::READ )
		MAP_ATTRIBUTE( "hasLowFrequencyRumble", m_deviceIdentifier.rumbleCapacity.hasLowFrequencyRumble, "Indicates if the device has a low frequency rumble motor", Be::READ )
		MAP_ATTRIBUTE( "hasLeftTriggerRumble", m_deviceIdentifier.rumbleCapacity.hasLeftTriggerRumble, "Indicates if the device has a left trigger rumble motor", Be::READ )
		MAP_ATTRIBUTE( "hasRightTriggerRumble", m_deviceIdentifier.rumbleCapacity.hasRightTriggerRumble, "Indicates if the device has a right trigger rumble motor", Be::READ )
		MAP_PROPERTY( "highFrequencyRumble", GetHighFrequencyRumble, SetHighFrequencyRumble, "The intensity of the high frequency rumble motor (0-1)" )
		MAP_PROPERTY( "lowFrequencyRumble", GetLowFrequencyRumble, SetLowFrequencyRumble, "The intensity of the low frequency rumble motor (0-1)" )
		MAP_PROPERTY( "leftTriggerRumble", GetLeftTriggerRumble, SetLeftTriggerRumble, "The intensity of the left trigger rumble motor (0-1)" )
		MAP_PROPERTY( "rightTriggerRumble", GetRightTriggerRumble, SetRightTriggerRumble, "The intensity of the right trigger rumble motor (0-1)" )
		MAP_METHOD_AND_WRAP( "ResetRumble", ResetRumble, "Resets all rumble motors to 0 intensity" )

		MAP_ATTRIBUTE( "buttons", m_buttons, "The buttons associated with this device", Be::READ )
		MAP_ATTRIBUTE( "axes", m_axes, "The axes associated with this device", Be::READ )
		MAP_ATTRIBUTE( "switches", m_switches, "The switches associated with this device", Be::READ )

	EXPOSURE_END()
}
