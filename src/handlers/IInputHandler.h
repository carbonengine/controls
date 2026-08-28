#pragma once
#include "../StdAfx.h"
#include "../DeviceEnums.h"
#include "../events/IInputEvent.h"

/// @brief Callback function type invoked when devices are added or removed.
typedef std::function<void( DeviceEnums::DeviceIdentifier& )> DeviceChangedCallback;

/**
 * @brief Abstract interface for platform-specific input device handling.
 *
 * Implementations are responsible for enumerating devices, reading their
 * hardware state, and sending rumble commands. The ControlManager owns
 * exactly one IInputHandler instance.
 */
class IInputHandler
{
public:
	/**
	 * @brief Virtual destructor to ensure proper cleanup of derived classes.
	 */
	virtual ~IInputHandler() = default;

	/**
	 * @brief Initializes the input handler.
	 * @return true if initialization succeeded, false otherwise.
	 */
	virtual bool Initialize() = 0;

	/**
	 * @brief Registers a callback that is invoked when a new device is connected.
	 * @param callback The function to call with the new device's identifier.
	 */
	virtual void RegisterForDeviceAdded( DeviceChangedCallback callback ) = 0;

	/**
	 * @brief Registers a callback that is invoked when a device is disconnected.
	 * @param callback The function to call with the removed device's identifier.
	 */
	virtual void RegisterForDeviceRemoved( DeviceChangedCallback callback ) = 0;

	/**
	 * @brief Activates or deactivates reading input from a specific device.
	 * @param deviceId Unique identifier of the device.
	 * @param activate true to start reading, false to stop.
	 */
	virtual void SetDeviceActivation( BlueSharedString deviceId, bool activate ) = 0;

	/**
	 * @brief Polls and returns accumulated device states since the last call.
	 * @param deviceId Unique identifier of the device to poll.
	 * @return A vector of state snapshots; empty if no new data is available.
	 */
	virtual std::vector<Events::State> Update( BlueSharedString deviceId ) = 0;

	/**
	 * @brief Sends a rumble command to the specified device.
	 * @param deviceId Unique identifier of the target device.
	 * @param rumble Rumble motor intensities to apply.
	 */
	virtual void Rumble( BlueSharedString deviceId, Events::Rumble rumble ) = 0;
	
	/**
	 * @brief Enables or disables background event processing for the specified device. When enabled, the handler will continue 
	 * to process input events even when the application is not in focus.
	 */
	virtual void SetBackgroundEventsEnabled( bool enabled ) = 0;
};
