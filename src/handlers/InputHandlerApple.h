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

#include "InputMappingApple.h"

/**
 * @brief Apple Game Controller framework implementation of IInputHandler.
 *
 * Uses GCController for device discovery and the physicalInputProfile
 * `valueDidChangeHandler` for state accumulation. Device state snapshots
 * are pushed into a per-slot vector by the handler and consumed on Update().
 * All GameController callbacks are routed through m_handlerQueue instead of
 * the default main queue, so delivery doesn't depend on the host app pumping
 * the main run loop.
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
	 * Slots are shared-owned so a caller that resolves one under m_deviceMutex, and the Obj-C
	 * blocks that observe one, both keep it alive independently of Update() erasing it.
	 */
	struct DeviceSlot
	{
		__strong GCController* controller = nil;                              ///< Owned reference to the underlying GCController.
		std::vector<ButtonHandling::ButtonSource> buttonSources {};   ///< How to sample each published button.
		std::vector<AxisHandling::AxisSource> axisSources {};        ///< How to sample each published axis.
		std::vector<SwitchHandling::SwitchSource> switchSources {};  ///< How to sample each published switch.
		DeviceEnums::DeviceIdentifier identifier{};    ///< Cached device metadata.
		std::vector<Events::State> accumulatedStates; ///< States accumulated by the value-change handler.
		bool pendingRemoval = false;                   ///< True when a disconnect notification has fired but the slot hasn't been finalized.
		bool active = false;                            ///< True when the caller wants the value-change handler installed; survives a transport-swap reconnect so input resumes without the caller having to reactivate.
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
	 *
	 * The caller must hold m_deviceMutex for the lookup itself, but the returned owner keeps
	 * the slot alive afterwards, so it stays valid even if Update() erases it from the list.
	 */
	std::shared_ptr<DeviceSlot> FindDeviceSlotLocked( BlueSharedString deviceId );

	/// @brief Finds a still-pendingRemoval slot that looks like the same physical controller reconnecting
	/// on a different transport (no GCController pointer or persistent hardware ID survives that). Caller
	/// must hold m_deviceMutex.
	std::shared_ptr<DeviceSlot> FindRevivedSlotLocked( const DeviceEnums::DeviceIdentifier& identifier );

	/// @brief Installs controller and the resolved element sources onto slot, then brings up haptics if
	/// slot.identifier (set by the caller beforehand) reports any rumble motors. Shared by the
	/// transport-swap-reconnect and brand-new-device paths in HandleControllerConnected. Caller must hold
	/// m_deviceMutex.
	void AdoptSourcesIntoSlot( const std::shared_ptr<DeviceSlot>& slot, GCController* controller,
		std::vector<ButtonHandling::ButtonSource> buttonSources,
		std::vector<AxisHandling::AxisSource> axisSources,
		std::vector<SwitchHandling::SwitchSource> switchSources );

	/// @brief Installs the input-queueing depth and valueDidChangeHandler on slot.controller, marks the slot active, and seeds accumulatedStates with a snapshot of the controller's current state so a caller doesn't have to wait for the next physical change to learn where it already is. Shared by SetDeviceActivation(activate=true) and by a transport-swap reconnect that revives a slot which was active before it disconnected. Caller must hold m_deviceMutex.
	void ActivateSlotHandler( const std::shared_ptr<DeviceSlot>& slot );

	/// @brief Reads every source's current value into one state snapshot. Shared by ActivateSlotHandler's initial read and the live valueDidChangeHandler.
	static Events::State SampleSlotState( const DeviceSlot& slot );

	/// @brief Brings up per-locality CoreHaptics engines and looping players; downgrades slot->identifier.rumbleCapacity on any per-channel failure.
	void InitializeHapticsForSlot( const std::shared_ptr<DeviceSlot>& slot );

	/// @brief Stops and releases every haptics engine/player attached to the slot; safe to call on a partially-initialized or empty slot.
	void ShutdownHapticsForSlot( DeviceSlot& slot );

	/// @brief Stops the channel's current player and (if intensity > 0) creates a fresh basic player carrying the new intensity. Basic CHHapticPatternPlayer has no in-place intensity update, so we rebuild.
	void SendChannelIntensity( DeviceSlot& slot, HapticsChannelIndex channel, float intensity );

	mutable std::mutex m_deviceMutex;                              ///< Protects m_deviceSlots.
	mutable std::mutex m_readingMutex;                             ///< Protects per-slot accumulated readings.
	std::vector<std::shared_ptr<DeviceSlot>> m_deviceSlots;        ///< All recognized devices.

	DeviceChangedCallback m_deviceAddedCallback = nullptr;         ///< Callback invoked when a device connects.
	DeviceChangedCallback m_deviceRemovedCallback = nullptr;       ///< Callback invoked when a device disconnects.

	__strong id m_connectObserver = nil;                           ///< NSNotificationCenter observer token for connect events.
	__strong id m_disconnectObserver = nil;                        ///< NSNotificationCenter observer token for disconnect events.

	// GCController delivers connect/disconnect notifications and physicalInputProfile value changes on
	// controller.handlerQueue, which defaults to the main queue; a host app whose main loop doesn't pump
	// the run loop/main queue would then never receive them. Routing everything through our own serial
	// queue makes delivery independent of the host app's run loop.
	__strong dispatch_queue_t m_handlerQueue = nil;                ///< Serial queue GameController callbacks are delivered on.

	std::atomic<uint64_t> m_deviceCounter{ 0 };                    ///< Monotonic counter used to disambiguate identical controller names.
	bool m_initialized = false;                                    ///< Whether Initialize() has completed successfully.
};

#endif // __APPLE__
