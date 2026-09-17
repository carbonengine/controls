#pragma once
#ifdef WIN32
#include "IInputHandler.h"
#include <Windows.h>
#include <gameinput_v3.h>

#include <memory>
#include <mutex>
#include <optional>
#include "../events/Events.h"
#include "InputMappingWin.h"


/**
 * @brief Windows implementation of IInputHandler using the GameInput API.
 *
 * Manages device discovery, input reading, and rumble output via the
 * Microsoft GameInput SDK. Device state is accumulated asynchronously
 * through registered reading callbacks and consumed on each Update() call.
 */
class InputHandlerWin : public IInputHandler
{
public:
	InputHandlerWin();
	~InputHandlerWin();

	/** @copydoc IInputHandler::Initialize() */
	bool Initialize() override;

	/** @copydoc IInputHandler::RegisterForDeviceAdded() */
	void RegisterForDeviceAdded( DeviceChangedCallback callback ) override;

	/** @copydoc IInputHandler::RegisterForDeviceRemoved() */
	void RegisterForDeviceRemoved( DeviceChangedCallback callback ) override;

	/** @copydoc IInputHandler::Update() */
	std::vector<Events::State> Update( BlueSharedString deviceId ) override;

	/** @copydoc IInputHandler::SetDeviceActivation() */
	void SetDeviceActivation( BlueSharedString deviceId, bool activate ) override;

	/** @copydoc IInputHandler::Rumble() */
	void Rumble( BlueSharedString deviceId, Events::Rumble rumble ) override;

	/** @copydoc IInputHandler::SetBackgroundEventsEnabled() */
	void SetBackgroundEventsEnabled( bool enabled ) override;

private:
	/**
	 * @brief The fixed recipe for turning one GameInput reading into an Events::State.
	 *
	 * Resolved once per connection and immutable thereafter, so the reading callback can walk
	 * it without a lock while a reconnect builds a replacement alongside it.
	 */
	struct ExtractionPlan
	{
		std::vector<ButtonHandling::ButtonSource> buttonSources; ///< How to sample each published button.
		std::vector<AxisHandling::AxisSource> axisSources; ///< How to sample each published axis.
		std::vector<SwitchHandling::SwitchSource> switchSources; ///< How to sample each published switch.

		bool needsGamepadState = false; ///< True when any source reads the gamepad view.
		bool needsRawButtons = false; ///< True when any source reads the raw button array.
		bool needsRawAxes = false; ///< True when any source reads the raw axis array.
		bool needsRawSwitches = false; ///< True when any source reads the raw switch array.
	};

	/**
	 * @brief Per-device bookkeeping slot.
	 */
	struct DeviceSlot
	{
		CComPtr<GameInputV3::IGameInputDevice> device = nullptr; ///< COM pointer to the GameInput device.
		bool pendingRemoval = false; ///< True when a disconnect event has been received but not yet processed.
		DeviceEnums::DeviceIdentifier identifier{}; ///< Device metadata.
		std::vector<Events::State> accumulatedStates{}; ///< States accumulated from reading callbacks, consumed by Update().
		GameInputV3::GameInputCallbackToken readCallbackToken = 0; ///< Token for the registered reading callback.
		std::shared_ptr<const ExtractionPlan> plan = std::make_shared<const ExtractionPlan>(); ///< Replaced wholesale on (re)connect.
	};


	/**
	 * @brief Finds a device slot by device ID string.
	 *
	 * Returns a shared owner so callers may keep using the slot after the lock is released.
	 *
	 * @param deviceID The unique device identifier.
	 * @return The matching DeviceSlot, or nullptr if not found.
	 */
	std::shared_ptr<DeviceSlot> GetDeviceSlot( BlueSharedString deviceID );

	/**
	 * @brief Finds a device slot by GameInput device pointer.
	 * @param device The GameInput device COM pointer.
	 * @return The matching DeviceSlot, or nullptr if not found.
	 */
	std::shared_ptr<DeviceSlot> GetDeviceSlot( CComPtr<GameInputV3::IGameInputDevice> device );

	/// @brief Lookup by device ID for callers that already hold m_deviceMutex.
	std::shared_ptr<DeviceSlot> FindSlotLocked( BlueSharedString deviceID );

	/**
	 * @brief Reads the current hardware state from a single GameInput reading.
	 *
	 * Walks the plan resolved on connect; no layout decisions are made here, so the cost per
	 * reading is proportional to the published element count.
	 *
	 * @param reading The GameInput reading to process.
	 * @param plan The extraction plan the reading's device was configured with.
	 * @return An Events::State snapshot populated from the reading, or std::nullopt when
	 * the reading could not be decoded and no snapshot should be published.
	 */
	static std::optional<Events::State> ReadDeviceState( GameInputV3::IGameInputReading* reading, const ExtractionPlan& plan );

	/**
	 * @brief Static callback invoked by GameInput when a device connects or disconnects.
	 */
	static void CALLBACK OnDeviceStatusChanged(
		_In_ GameInputV3::GameInputCallbackToken callbackToken,
		_In_ void* context,
		_In_ GameInputV3::IGameInputDevice* device,
		_In_ uint64_t timestamp,
		_In_ GameInputV3::GameInputDeviceStatus currentStatus,
		_In_ GameInputV3::GameInputDeviceStatus previousStatus ) noexcept;

	/**
	 * @brief Static callback invoked by GameInput when a new reading is available.
	 */
	static void CALLBACK OnDeviceRead(
		_In_ GameInputV3::GameInputCallbackToken callbackToken,
		_In_ void* context,
		_In_ GameInputV3::IGameInputReading* reading ) noexcept;

	/**
	 * @brief Builds a DeviceIdentifier from a GameInput device.
	 * @param device The GameInput device to query.
	 * @return A populated DeviceIdentifier.
	 */
	static DeviceEnums::DeviceIdentifier GetIdentifier( GameInputV3::IGameInputDevice* device );

	/**
	 * @brief Resolves a device's extraction plan and rewrites @p identifier to the element keys it publishes.
	 *
	 * Pure: touches no slot state, so the caller can build the replacement plan off-lock and
	 * install it in one assignment. A reconnect therefore never exposes a half-rebuilt plan
	 * to the reading callback.
	 *
	 * @param device The GameInput device, may be null.
	 * @param identifier Identifier whose element key lists are replaced.
	 * @return The immutable plan for this connection.
	 */
	static std::shared_ptr<const ExtractionPlan> ResolvePlan( GameInputV3::IGameInputDevice* device, DeviceEnums::DeviceIdentifier& identifier );

	mutable std::mutex m_deviceMutex; ///< Protects m_deviceSlots and the mutable fields of every slot.
	mutable std::mutex m_readingMutex; ///< Protects per-device accumulated readings.

	GameInputV3::GameInputCallbackToken m_deviceCallbackToken = 0; ///< Token for the device status callback.

	CComPtr<GameInputV3::IGameInput> m_gameInput = nullptr; ///< The GameInput interface.
	// Shared ownership so a slot stays alive for as long as anyone is using it: the reading
	// callback resolves a slot on the GameInput thread and writes into it after releasing
	// m_deviceMutex.
	std::vector<std::shared_ptr<DeviceSlot>> m_deviceSlots = {}; ///< All recognized devices.

	DeviceChangedCallback m_deviceAddedCallback = nullptr; ///< Callback for device connection events.
	DeviceChangedCallback m_deviceRemovedCallback = nullptr; ///< Callback for device disconnection events.

	bool m_initialized = false; ///< Whether Initialize() has completed successfully.
	bool m_devicesRemoved = false; ///< Flag indicating pending device removals.

	/// @brief Supported GameInput device kinds.
	static const GameInputV3::GameInputKind SUPPORTED_INPUTS = static_cast<GameInputV3::GameInputKind>(
		GameInputV3::GameInputKindGamepad |
		GameInputV3::GameInputKindController );
};
#endif
