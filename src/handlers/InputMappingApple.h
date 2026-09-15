#pragma once
#ifdef __APPLE__
#include "IInputHandler.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

#import <Foundation/Foundation.h>
#import <GameController/GameController.h>
#import <CoreHaptics/CoreHaptics.h>

#include <array>

/**
 * @brief Apple Game Controller framework implementation of IInputHandler.
 *
 * Uses GCController for device discovery and the physicalInputProfile
 * `valueDidChangeHandler` for state accumulation. Device state snapshots
 * are pushed into a per-slot vector by the handler and consumed on Update().
 *
 * Rumble is delivered through GCDeviceHaptics + CoreHaptics: one persistent
 * looping advanced pattern player is created per supported locality on
 * connect, and each Rumble() call updates that channel's intensity via a
 * dynamic-parameter send.
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

	/** @copydoc IInputHandler::SetBackgroundEventsEnabled() */
	void SetBackgroundEventsEnabled( bool enabled ) override;

private:
	/// @brief Ordinal index of a rumble channel; matches the four fields of Events::Rumble.
	enum HapticsChannelIndex
	{
		LowFrequency = 0,
		HighFrequency,
		LeftTrigger,
		RightTrigger,
		ChannelCount
	};

	/// @brief Per-locality CoreHaptics state: the engine, the currently-playing basic pattern player (rebuilt on every intensity change), and the last intensity we sent.
	struct HapticsChannel
	{
		__strong CHHapticEngine* engine = nil;
		__strong id<CHHapticPatternPlayer> player = nil;
		float lastIntensity = 0.0f;
		bool supported = false;
	};

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
		__strong GCDeviceHaptics* haptics = nil;                        ///< Non-nil when the controller exposes any rumble locality (macOS 11+).
		std::array<HapticsChannel, ChannelCount> hapticsChannels{};     ///< Per-channel engines/players/state, indexed by HapticsChannelIndex.
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

	/// @brief Brings up per-locality CoreHaptics engines and looping players; downgrades slot->identifier.rumbleCapacity on any per-channel failure.
	void InitializeHapticsForSlot( DeviceSlot& slot );

	/// @brief Stops and releases every haptics engine/player attached to the slot; safe to call on a partially-initialized or empty slot.
	void ShutdownHapticsForSlot( DeviceSlot& slot );

	/// @brief Stops the channel's current player and (if intensity > 0) creates a fresh basic player carrying the new intensity. Basic CHHapticPatternPlayer has no in-place intensity update, so we rebuild.
	void SendChannelIntensity( DeviceSlot& slot, HapticsChannelIndex channel, float intensity );

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
