#pragma once
#ifdef __APPLE__
#include "IInputHandler.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

#import <Foundation/Foundation.h>
#import <GameController/GameController.h>

/**
 * @brief Apple Game Controller framework implementation of IInputHandler.
 *
 * Uses GCController for device discovery and the physicalInputProfile
 * `valueDidChangeHandler` for state accumulation. Device state snapshots
 * are pushed into a per-slot vector by the handler and consumed on Update().
 *
 * Rumble is stubbed and reported as unsupported in the DeviceIdentifier.
 */
class InputHandlerApple : public IInputHandler
{
public:
	InputHandlerApple();
	~InputHandlerApple();

	/** @copydoc IInputHandler::Initialize() */
	bool Initialize() override;

	/** @copydoc IInputHandler::RegisterForDeviceAdded() */
	void RegisterForDeviceAdded( DeviceChangedCallback callback ) override;

	/** @copydoc IInputHandler::RegisterForDeviceRemoved() */
	void RegisterForDeviceRemoved( DeviceChangedCallback callback ) override;

	/** @copydoc IInputHandler::SetDeviceActivation() */
	void SetDeviceActivation( BlueSharedString deviceId, bool activate ) override;

	/** @copydoc IInputHandler::Update() */
	std::vector<Events::State> Update( BlueSharedString deviceId ) override;

	/** @copydoc IInputHandler::Rumble() */
	void Rumble( BlueSharedString deviceId, Events::Rumble rumble ) override;
	
	/** @copydoc IInputHandler::GetButtonNames() */
	const std::vector<BlueSharedString> GetButtonNames( BlueSharedString deviceId ) override;
	
	/** @copydoc IInputHandler::GetAxisNames() */
	const std::vector<BlueSharedString> GetAxisNames( BlueSharedString deviceId ) override;
	
	/** @copydoc IInputHandler::GetSwitchNames() */
	const std::vector<BlueSharedString> GetSwitchNames( BlueSharedString deviceId ) override;

	/** @copydoc IInputHandler::SetBackgroundEventsEnabled() */
	void SetBackgroundEventsEnabled( bool enabled ) override;

private:
	/**
	 * @brief Per-device bookkeeping slot.
	 *
	 * Slots are heap-allocated (unique_ptr) so raw pointers captured by
	 * Obj-C blocks remain valid even when the containing vector reallocates.
	 */
	struct DeviceSlot
	{
		__strong GCController* controller = nil;                              ///< Owned reference to the underlying GCController.
		std::vector<GCControllerButtonInput*> buttons {};            ///< Ordered digital buttons (analog triggers excluded).
		std::vector<GCControllerAxisInput*> axes {};                 ///< Ordered analog axes.
		std::vector<GCControllerButtonInput*> triggerAxes {};        ///< Analog trigger buttons appended after `axes` in the axis dimension.
		std::vector<GCControllerDirectionPad*> switches {};             ///< Ordered d-pad / hat switches.
		DeviceEnums::DeviceIdentifier identifier{};    ///< Cached device metadata.
		std::vector<Events::State> accumulatedStates; ///< States accumulated by the value-change handler.
		bool pendingRemoval = false;                   ///< True when a disconnect notification has fired but the slot hasn't been finalized.
		bool active = false;                            ///< True when the value-change handler is installed.
		std::vector<BlueSharedString> buttonNames = {};
		std::vector<BlueSharedString> axisNames = {};
		std::vector<BlueSharedString> switchNames = {};
	};

	/**
	 * @brief Handles a GCController connection notification.
	 */
	void HandleControllerConnected(	GCController* controller );

	/**
	 * @brief Handles a GCController disconnection notification.
	 */
	void HandleControllerDisconnected( GCController* controller );

	/**
	 * @brief Finds a device slot by device ID string.
	 */
	DeviceSlot* GetDeviceSlot( BlueSharedString deviceId );

	/**
	 * @brief Finds a device slot by GCController pointer.
	 */
	DeviceSlot* GetDeviceSlot( GCController* controller	);

	mutable std::mutex m_deviceMutex;                              ///< Protects m_deviceSlots.
	mutable std::mutex m_readingMutex;                             ///< Protects per-slot accumulated readings.
	std::vector<std::unique_ptr<DeviceSlot>> m_deviceSlots;        ///< All recognized devices.

	DeviceChangedCallback m_deviceAddedCallback = nullptr;         ///< Callback invoked when a device connects.
	DeviceChangedCallback m_deviceRemovedCallback = nullptr;       ///< Callback invoked when a device disconnects.

	__strong id m_connectObserver = nil;                           ///< NSNotificationCenter observer token for connect events.
	__strong id m_disconnectObserver = nil;                        ///< NSNotificationCenter observer token for disconnect events.

	std::atomic<uint64_t> m_deviceCounter{ 0 };                    ///< Monotonic counter used to disambiguate identical controller names.
	bool m_initialized = false;                                    ///< Whether Initialize() has completed successfully.
};

#endif // __APPLE__
