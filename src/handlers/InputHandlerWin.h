#pragma once
#ifdef WIN32
#include "IInputHandler.h"
#include <Windows.h>   
#include <gameinput_v3.h>

#include <array>	
#include <mutex>
#include "../events/IInputEvent.h"

using namespace GameInput::v3;

namespace ButtonHandling
{
/**
* @brief Where a single published button is sampled from.
*
* Plain data: resolved once when the device connects, then walked per reading.
*/
struct ButtonSource
{
	/// @brief The reading view supplying this button.
	enum class Kind : uint8_t
	{
		None, ///< Not present on this device; always reads as unpressed.
		GamepadMask, ///< Sample the gamepad state's button mask.
		RawIndex ///< Sample the raw controller button array.
	};

	Kind kind = Kind::None;
	GameInputGamepadButtons mask = GameInputGamepadNone; ///< Mask to test when kind is GamepadMask.
	uint32_t rawIndex = 0; ///< Raw controller index when kind is RawIndex.
	DeviceEnums::InputElementDescriptor descriptor = DeviceEnums::InputElementDescriptor::Unknown;
};

std::vector<ButtonSource> GetButtonSources( const GameInputControllerInfo* controllerInfo, const GameInputGamepadInfo* gamepadInfo );
Events::Button Handle( const ButtonSource& source, const GameInputGamepadState& gamepadState, const bool* rawButtons, uint32_t buttonCount );
}

namespace AxisHandling
{
/**
* @brief Where a single published axis is sampled from.
*/
struct AxisSource
{
	/// @brief The reading view supplying this axis.
	enum class Kind : uint8_t
	{
		GamepadField, ///< Read a named GameInputGamepadState field.
		RawIndex ///< Sample the raw controller axis array.
	};

	Kind kind = Kind::RawIndex;
	uint32_t rawIndex = 0; ///< Raw controller index when kind is RawIndex.
	DeviceEnums::InputElementDescriptor descriptor = DeviceEnums::InputElementDescriptor::Unknown;
};

std::vector<AxisSource> GetAxisSources( const GameInputControllerInfo* controllerInfo, const GameInputGamepadInfo* gamepadInfo );
Events::Axis Handle( const AxisSource& source, const GameInputGamepadState& gamepadState );
}

namespace SwitchHandling
{
std::vector<uint32_t> GetSwitchSources( const GameInputControllerInfo* controllerInfo );
Events::Switch Handle( uint32_t switchSource, const GameInputGamepadState& gamepadState );
}


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

		/// @name Extraction plan
		/// Resolved once by ConfigureDeviceSlot() when the device connects and
		/// treated as immutable afterwards, because readings are decoded on the
		/// GameInput callback thread. Each vector is index-aligned with the
		/// matching identifier element list.
		/// @{
		std::vector<ButtonHandling::ButtonSource> buttonSources{}; ///< How to sample each published button.
		std::vector<AxisHandling::AxisSource> axisSources{};     ///< How to sample each published axis.
		std::vector<uint32_t> switchSources{};     ///< Raw controller switch index for each published switch.

		bool needsGamepadState = false; ///< True when any source reads the gamepad view.
		bool needsRawButtons = false;   ///< True when any source reads the raw button array.
		bool needsRawAxes = false;      ///< True when any source reads the raw axis array.
		bool needsRawSwitches = false;  ///< True when any source reads the raw switch array.
		/// @}
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
	 *
	 * Walks the slot's precomputed extraction plan; no layout decisions are made
	 * here, so the cost per reading is proportional to the published element count.
	 *
	 * @param reading The GameInput reading to process.
	 * @param slot The device slot the reading belongs to, supplying the resolved
	 * extraction plan.
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
	 * @brief Resolves the device's fixed extraction plan and publishes matching identifiers.
	 *
	 * Builds slot.buttonPlan, slot.axisPlan and slot.switchPlan from the device's
	 * layout information, normalizes a button-reported DPad into a single switch,
	 * and rewrites the identifier element lists so they stay index-aligned with
	 * the states ReadDeviceState() will emit. Called once per device connection.
	 *
	 * @param slot The slot to configure; its identifier must already be populated.
	 * @param device The GameInput device, may be null.
	 */
	static void ConfigureDeviceSlot( DeviceSlot& slot, IGameInputDevice* device );

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
