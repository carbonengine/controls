#include "InputDevice.h"

BLUE_DEFINE( InputDevice );

const Be::ClassInfo* InputDevice::ExposeToBlue()
{
	EXPOSURE_BEGIN( InputDevice, "Input Device" )
		MAP_INTERFACE( InputDevice )
		MAP_ATTRIBUTE( "name", m_deviceIdentifier.name, "The name of the device", Be::READ )
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
		MAP_METHOD_AND_WRAP( "GetButtonNames", GetButtonNames, "Gets the names of all buttons on the device" )
		MAP_METHOD_AND_WRAP( "GetAxisNames", GetAxisNames, "Gets the names of all axes on the device" )
		MAP_METHOD_AND_WRAP( "GetSwitchNames", GetSwitchNames, "Gets the names of all switches on the device" )

	EXPOSURE_END()
}
