#pragma once
#ifdef WIN32
#include "IInputHandler.h"
#include <gameinput.h>

#include <array>
#include <mutex>
#include "../events/IInputEvent.h"

using namespace GameInput::v3;

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

private:
	/**
	 * @brief Per-device bookkeeping slot.
	 */
	struct DeviceSlot
	{
		CComPtr<IGameInputDevice> device = nullptr;   ///< COM pointer to the GameInput device.
		bool pendingRemoval = false;                   ///< True when a disconnect event has been received but not yet processed.
		DeviceEnums::DeviceIdentifier identifier{};    ///< Device metadata.
		std::vector<Events::State> accumulatedStates{}; ///< States accumulated from reading callbacks, consumed by Update().
		GameInputCallbackToken readCallbackToken = 0;  ///< Token for the registered reading callback.
	};

	/**
	 * @brief Finds a device slot by device ID string.
	 * @param deviceID The unique device identifier.
	 * @return Pointer to the matching DeviceSlot, or nullptr if not found.
	 */
	DeviceSlot* GetDeviceSlot( BlueSharedString deviceID );

	/**
	 * @brief Finds a device slot by GameInput device pointer.
	 * @param device The GameInput device COM pointer.
	 * @return Pointer to the matching DeviceSlot, or nullptr if not found.
	 */
	DeviceSlot* GetDeviceSlot( CComPtr<IGameInputDevice> device );

	/**
	 * @brief Shuts down GameInput and releases all resources.
	 */
	void ShutdownGameInput();

	/**
	 * @brief Reads the current hardware state from a single GameInput reading.
	 * @param reading The GameInput reading to process.
	 * @return An Events::State snapshot populated from the reading.
	 */
	Events::State ReadDeviceState( IGameInputReading* reading );

	/**
	 * @brief Static callback invoked by GameInput when a device connects or disconnects.
	 */
	static void CALLBACK OnDeviceStatusChanged(
		_In_ GameInputCallbackToken callbackToken,
		_In_ void* context,
		_In_ IGameInputDevice* device,
		_In_ uint64_t timestamp,
		_In_ GameInputDeviceStatus currentStatus,
		_In_ GameInputDeviceStatus previousStatus ) noexcept;

	/**
	 * @brief Static callback invoked by GameInput when a new reading is available.
	 */
	static void CALLBACK OnDeviceRead(
		_In_ GameInputCallbackToken callbackToken,
		_In_ void* context,
		_In_ IGameInputReading* reading ) noexcept;

	/**
	 * @brief Builds a DeviceIdentifier from a GameInput device.
	 * @param device The GameInput device to query.
	 * @return A populated DeviceIdentifier.
	 */
	static DeviceEnums::DeviceIdentifier GetIdentifier( IGameInputDevice* device );

	mutable std::shared_mutex m_deviceMutex;  ///< Protects m_deviceSlots.
	mutable std::shared_mutex m_readingMutex; ///< Protects per-device accumulated readings.

	GameInputCallbackToken m_deviceCallbackToken = 0; ///< Token for the device status callback.

	CComPtr<IGameInput> m_gameInput = nullptr;     ///< The GameInput interface.
	std::vector<DeviceSlot> m_deviceSlots = {};     ///< All recognized devices.

	DeviceChangedCallback m_deviceAddedCallback = nullptr;   ///< Callback for device connection events.
	DeviceChangedCallback m_deviceRemovedCallback = nullptr; ///< Callback for device disconnection events.

	bool m_initialized = false;   ///< Whether Initialize() has completed successfully.
	bool m_devicesRemoved = false; ///< Flag indicating pending device removals.

	/// @brief Supported GameInput device kinds.
	static const GameInputKind SUPPORTED_INPUTS = static_cast<GameInputKind>(
	GameInputKindGamepad |
	GameInputKindController );
};
#endif