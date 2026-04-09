#include "ControlManager.h"

BLUE_DEFINE( ControlManager );

const Be::ClassInfo* ControlManager::ExposeToBlue()
{
	EXPOSURE_BEGIN( ControlManager, "Input Controller Manager" )
		MAP_INTERFACE( ControlManager )

		MAP_METHOD_AND_WRAP( "Activate", Activate, "Activates the control manager to the input system" )
		MAP_METHOD_AND_WRAP( "Deactivate", Deactivate, "Deactivates the control manager from the input system" )
		MAP_METHOD_AND_WRAP( "Update", Update, "Updates the active devices and calls their callbacks if applicable" )
		MAP_PROPERTY( "holdTimeMs", GetHoldTimeInMs, SetHoldTimeInMs, "The time in milliseconds that dictates whether a button is held or not" )
		MAP_ATTRIBUTE( "activeDevices", m_activeDevices, "The active devices", Be::READ )
		MAP_ATTRIBUTE( "devices", m_devices, "The device identifiers", Be::READ )
		MAP_ATTRIBUTE( "activeDeviceLostCallback", m_activeDeviceLostCallback, "The callback that will be executed when the active device is lost. The callback needs to accept a deviceid as a parameter", Be::READWRITE )
		MAP_ATTRIBUTE( "deviceConnectedCallback", m_deviceConnectedCallback, "The callback that will be executed when a device is connected", Be::READWRITE )
	EXPOSURE_END()
}