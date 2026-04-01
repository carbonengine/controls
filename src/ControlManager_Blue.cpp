#include "ControlManager.h"

BLUE_DEFINE( ControlManager );

const Be::ClassInfo* ControlManager::ExposeToBlue()
{
	EXPOSURE_BEGIN( ControlManager, "Input Controller Manager" )
		MAP_INTERFACE( ControlManager )

		MAP_METHOD_AND_WRAP( "Connect", Connect, "Connects the control manager to the input system" )
		MAP_METHOD_AND_WRAP( "Disconnect", Disconnect, "Disconnects the control manager from the input system" )
		MAP_METHOD_AND_WRAP( "Update", Update, "Queries the state of the controllers and stores their state until the next update" )
		MAP_PROPERTY( "holdTimeMs", GetHoldTimeInMs, SetHoldTimeInMs, "The time in milliseconds that dictates whether a button is held or not" )
		MAP_ATTRIBUTE( "activeDevice", m_activeDevice, "The active device", Be::READ )
		MAP_ATTRIBUTE( "devices", m_deviceIdentifiers, "The device identifiers", Be::READ )
		MAP_ATTRIBUTE( "activeDeviceLostCallback", m_activeDeviceLostCallback, "The callback that will be executed when the active device is lost", Be::READWRITE )
		MAP_ATTRIBUTE( "deviceConnectedCallback", m_deviceConnectedCallback, "The callback that will be executed when a device is connected", Be::READWRITE )
	EXPOSURE_END()
}