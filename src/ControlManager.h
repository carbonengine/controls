#pragma once
#include "StdAfx.h"
#include "handlers/IInputHandler.h"
#include "InputDevice.h"
#include "DeviceEnums.h"

BLUE_DECLARE( InputDevice );
BLUE_DECLARE_VECTOR( InputDevice );
BLUE_DECLARE( InputDeviceIdentifier );
BLUE_DECLARE_VECTOR( InputDeviceIdentifier );

/**
 * @brief Central manager for discovering, activating, and polling input devices.
 *
 * ControlManager owns the platform-specific IInputHandler and maintains two
 * device lists: all known (connected) devices and the subset that have been
 * explicitly activated by the application. When Update() is called, it processes
 * device connect/disconnect events and polls active devices for new input.
 *
 * Script callbacks are provided for:
 * - @c m_deviceAddedCallback   \u2013 a new device was connected.
 * - @c m_deviceRemovedCallback \u2013 an inactive device was disconnected.
 * - @c m_activeDeviceLostCallback \u2013 an *active* device was disconnected.
 */
BLUE_CLASS( ControlManager ) : public IRoot
{
public:
	EXPOSE_TO_BLUE();

	/**
	 * @brief Constructs the ControlManager and initializes the platform input handler.
	 * @param lockobj Optional Blue lock object
	 */
	ControlManager( IRoot* lockobj = nullptr );

	/**
	 * @brief Activates an input device so that it is polled each frame.
	 * @param deviceID Unique string identifier of the device to activate.
	 * @return A pointer to the activated InputDevice, or nullptr if the device is not connected.
	 */
	IRootPtr Activate( BlueSharedString deviceID );

	/**
	 * @brief Processes pending device changes and polls all active devices
	 *
	 * This function does two things:
	 * - It process any queued device connection or disconnection events, invoking the appropriate callbacks.
	 * - It iterates through the list of active devices and calls their Update() method to read new input states and trigger appropriate events.
	 *
	 * Should be called from the main game loop.
	 */
	void Update();

	/**
	 * @brief Deactivates an input device so that it is no longer polled.
	 * @param deviceID Unique string identifier of the device to deactivate.
	 */
	void Deactivate( BlueSharedString deviceID );

private:
	/**
	 * @brief Sets the hold-time threshold for button state transitions.
	 * @param holdTime Time in milliseconds after which a pressed button transitions to Held.
	 */
	void SetHoldTimeInMs( uint64_t holdTime );

	/**
	 * @brief Gets the current hold-time threshold.
	 * @return Hold time in milliseconds.
	 */
	uint64_t GetHoldTimeInMs();

	/**
	 * @brief Called from the device update thread when a new device is detected.
	 * @param deviceIdentifier Identifier of the added device.
	 */
	void OnDeviceAdded( DeviceEnums::DeviceIdentifier& deviceIdentifier );

	/**
	 * @brief Called from the device update thread when a device is disconnected.
	 * @param deviceIdentifier Identifier of the removed device.
	 */
	void OnDeviceRemoved( DeviceEnums::DeviceIdentifier& deviceIdentifier );

	/**
	 * @brief Processes queued device add/remove events on the main thread.
	 */
	void ProcessChangedDevices();

	/**
	 * @brief Searches the full device list for a device by ID.
	 * @param deviceID The device identifier to search for.
	 * @return Pointer to the InputDevice, or nullptr if not found.
	 */
	InputDevicePtr FindDevice( BlueSharedString deviceID ) const;

	/**
	 * @brief Searches the active device list for a device by ID.
	 * @param deviceID The device identifier to search for.
	 * @return Pointer to the InputDevice, or nullptr if not found.
	 */
	InputDevicePtr FindActiveDevice( BlueSharedString deviceID ) const;

	PInputDeviceVector m_devices;                     ///< All known (connected) devices.
	std::unique_ptr<IInputHandler> m_inputHandler;    ///< Platform-specific input handler.
	PInputDeviceVector m_activeDevices;                ///< Devices currently being polled.
	BlueScriptCallback m_activeDeviceLostCallback;    ///< Callback when an active device disconnects.
	BlueScriptCallback m_deviceAddedCallback;         ///< Callback when a new device is connected.
	BlueScriptCallback m_deviceRemovedCallback;       ///< Callback when an inactive device disconnects.
	std::mutex m_deviceChangedMutex;                  ///< Protects the add/remove queues.

	std::vector<DeviceEnums::DeviceIdentifier> m_addedDevices;   ///< Queued device-added events.
	std::vector<DeviceEnums::DeviceIdentifier> m_removedDevices; ///< Queued device-removed events.

	bool m_initialDevicesProcessed = false; ///< Whether the initial device enumeration has been processed.
};

TYPEDEF_BLUECLASS( ControlManager );