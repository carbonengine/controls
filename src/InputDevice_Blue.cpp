#include "InputDevice.h"

BLUE_DEFINE( InputDevice );

const Be::ClassInfo* InputDevice::ExposeToBlue()
{
	EXPOSURE_BEGIN( InputDevice, "Input Device" )
		MAP_INTERFACE( InputDevice )
		MAP_INTERFACE( INotify )
		MAP_ATTRIBUTE( "name", m_deviceIdentifier.name, "The name of the device", Be::READ )
		MAP_ATTRIBUTE( "deviceID", m_deviceIdentifier.deviceID, "The ID of the device", Be::READ )
		MAP_ATTRIBUTE( "vendorID", m_deviceIdentifier.vendorID, "The manufacturer of the device", Be::READ )
		MAP_ATTRIBUTE( "productID", m_deviceIdentifier.productID, "The product name of the device", Be::READ )
		MAP_ATTRIBUTE( "buttonCount", m_deviceIdentifier.buttonCount, "The number of buttons on the device", Be::READ )
		MAP_ATTRIBUTE( "axisCount", m_deviceIdentifier.axisCount, "The number of axes on the device", Be::READ )
		MAP_ATTRIBUTE( "switchCount", m_deviceIdentifier.switchCount, "The number of switches on the device", Be::READ )
		MAP_ATTRIBUTE( "triggers", m_triggers, "The triggers associated with this device", Be::READ )
		MAP_ATTRIBUTE( "rumbleMotorCount", m_deviceIdentifier.rumbleCapacity.rumbleMotorCount, "The number of rumble motors on the device", Be::READ )
		MAP_ATTRIBUTE( "hasHighFrequencyRumble", m_deviceIdentifier.rumbleCapacity.hasHighFrequencyRumble, "Indicates if the device has a high frequency rumble motor", Be::READ )
		MAP_ATTRIBUTE( "hasLowFrequencyRumble", m_deviceIdentifier.rumbleCapacity.hasLowFrequencyRumble, "Indicates if the device has a low frequency rumble motor", Be::READ )
		MAP_ATTRIBUTE( "hasLeftTriggerRumble", m_deviceIdentifier.rumbleCapacity.hasLeftTriggerRumble, "Indicates if the device has a left trigger rumble motor", Be::READ )
		MAP_ATTRIBUTE( "hasRightTriggerRumble", m_deviceIdentifier.rumbleCapacity.hasRightTriggerRumble, "Indicates if the device has a right trigger rumble motor", Be::READ )
		MAP_ATTRIBUTE( "highFrequencyRumble", m_rumble.highFrequency, "The intensity of the high frequency rumble motor (0-1)", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "lowFrequencyRumble", m_rumble.lowFrequency, "The intensity of the low frequency rumble motor (0-1)", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "leftTriggerRumble", m_rumble.leftTrigger, "The intensity of the left trigger rumble motor (0-1)", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "rightTriggerRumble", m_rumble.rightTrigger, "The intensity of the right trigger rumble motor (0-1)", Be::READWRITE | Be::NOTIFY )
	EXPOSURE_END()
}