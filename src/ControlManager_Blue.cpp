// Copyright © 2026 CCP ehf.

#include "ControlManager.h"

BLUE_DEFINE( ControlManager );



const Be::ClassInfo* ControlManager::ExposeToBlue()
{
	EXPOSURE_BEGIN( ControlManager, "Input Controller Manager" )
		MAP_INTERFACE( ControlManager )

		MAP_METHOD_AND_WRAP( "Initialize", Initialize, "Initializes the control manager" )
		MAP_METHOD_AND_WRAP( "Activate", Activate, "Activates the control manager to the input system" )
		MAP_METHOD_AND_WRAP( "Deactivate", Deactivate, "Deactivates the control manager from the input system" )
		MAP_METHOD_AND_WRAP( "Update", Update, "Updates the active devices and calls their callbacks if applicable" )
		MAP_METHOD_AND_WRAP( "SetBackgroundEventsEnabled", SetBackgroundEventsEnabled, "Enables or disables background events for the control handlers" )
		MAP_PROPERTY( "holdTimeMs", GetHoldTimeInMs, SetHoldTimeInMs, "The time in milliseconds that dictates whether a button is held or not" )
		MAP_ATTRIBUTE( "activeDevices", m_activeDevices, "The active devices", Be::READ )
		MAP_ATTRIBUTE( "devices", m_devices, "The device identifiers", Be::READ )
		MAP_ATTRIBUTE( "activeDeviceLostCallback", m_activeDeviceLostCallback, "The callback that will be executed when the active device is lost. The callback needs to accept a deviceid as a parameter", Be::READWRITE )
		MAP_ATTRIBUTE( "deviceAddedCallback", m_deviceAddedCallback, "The callback that will be executed when a device is added. The callback needs to accept a deviceid as a parameter", Be::READWRITE )
		MAP_ATTRIBUTE( "deviceRemovedCallback", m_deviceRemovedCallback, "The callback that will be executed when a device is removed. The callback needs to accept a deviceid as a parameter", Be::READWRITE )


		MAP_METHOD_AND_WRAP( "_EnableMockInputHandler", EnableMockInputHandler, "Enables the mock input handler" )
		MAP_METHOD_AND_WRAP( "_MockAddDevice", MockAddDevice, "Connects a fake gamepad with a standard layout. Takes a device ID and a name" )
		MAP_METHOD_AND_WRAP( "_MockRemoveDevice", MockRemoveDevice, "Disconnects a fake device" )
		MAP_METHOD_AND_WRAP( "_MockSetButton", MockSetButton, "Sets the pressed state of a button. Takes a device ID, an InputElement and a bool" )
		MAP_METHOD_AND_WRAP( "_MockSetAxis", MockSetAxis, "Sets the value of an axis. Takes a device ID, an InputElement and a float" )
		MAP_METHOD_AND_WRAP( "_MockSetSwitch", MockSetSwitch, "Sets the position of a switch. Takes a device ID, an InputElement and a SwitchPosition value" )
		MAP_METHOD_AND_WRAP( "_MockAdvanceTimeMs", MockAdvanceTimeMs, "Advances the mock clock used to timestamp input changes" )
		MAP_METHOD_AND_WRAP( "_MockIsDeviceActive", MockIsDeviceActive, "Returns whether the mock input handler considers the device active" )

	EXPOSURE_END()
}