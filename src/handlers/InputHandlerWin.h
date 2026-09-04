#pragma once
#ifdef WIN32
#include "IInputHandler.h"
#include <Windows.h>   
#include <gameinput_v3.h>

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

	/** @copydoc IInputHandler::SetBackgroundEventsEnabled() */
	void SetBackgroundEventsEnabled( bool enabled ) override;
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
		bool supportsGamepad = false;                  ///< True when the device exposes a gamepad view, whose axis values are already correctly signed.
		std::vector<GameInputGamepadButtons> gamepadButtonMasks{}; ///< Gamepad buttons this device supports, in the order they are published.
		std::vector<uint32_t> extraButtonIndices{};    ///< Raw controller indices of the vendor-specific buttons published after the gamepad layout.
		std::vector<uint32_t> extraAxisIndices{};      ///< Raw controller indices of the vendor-specific axes published after the gamepad axes.
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
	 * @param slot The device slot the reading belongs to, supplying the gamepad
	 * capability and the resolved axis roles.
	 * @return An Events::State snapshot populated from the reading.
	 */
	Events::State ReadDeviceState( IGameInputReading* reading, const DeviceSlot& slot );

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

	/**
	 * @brief Reports whether a device exposes a GameInput gamepad view.
	 * @param device The GameInput device to query.
	 * @return true when GameInputKindGamepad is supported.
	 */
	static bool SupportsGamepad( IGameInputDevice* device );

	/**
	 * @brief Returns the gamepad buttons a device supports, in publication order.
	 *
	 * Derived from GameInputGamepadInfo::supportedLayout, so the list matches the button
	 * identifiers published for the device and can be used to sample the button mask.
	 *
	 * @param device The GameInput device, may be null.
	 * @return The supported button masks, or an empty list if the device has no gamepad view.
	 */
	static std::vector<GameInputGamepadButtons> GetGamepadButtonMasks( IGameInputDevice* device );

	/**
	 * @brief Fills the gamepad-view fields of a slot from the device's layout information.
	 * @param slot The slot to configure.
	 * @param device The GameInput device, may be null.
	 */
	static void ConfigureGamepadSlot( DeviceSlot& slot, IGameInputDevice* device );

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
