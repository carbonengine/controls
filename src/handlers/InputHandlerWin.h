#pragma once
#ifdef WIN32
#include "IInputHandler.h"
#include <gameinput.h>

#include <array>
#include <mutex>
#include "../events/IInputEvent.h"

using namespace GameInput::v3;

class InputHandlerWin : public IInputHandler
{
public:
	InputHandlerWin();
	~InputHandlerWin();
	bool Initialize() override;
	void RegisterForDeviceAdded( DEVICE_CHANGED_CALLBACK callback ) override;
	void RegisterForDeviceRemoved( DEVICE_CHANGED_CALLBACK callback ) override;
	std::vector<Events::State> Update( DeviceEnums::DeviceId deviceId ) override;
	void SetDeviceActivation( DeviceEnums::DeviceId deviceId, bool activate ) override;
	void Rumble( DeviceEnums::DeviceId deviceId, Events::Rumble rumble ) override;

private:
	// Per-device bookkeeping
	struct DeviceSlot
	{
		IGameInputDevice* device = nullptr;
		bool pendingRemoval = false; // set to true when we receive a disconnect event, until the slot is cleaned up on the next Update()
		DeviceEnums::DeviceIdentifier identifier{};
	};

	// GameInput setup / teardown
	void ShutdownGameInput();

	// Reads the current hardware state for a single device into its slot
	Events::State ReadDeviceState( IGameInputReading* reading );

	// The static callback forwarded from GameInput when devices connect / disconnect
	static void CALLBACK OnDeviceStatusChanged(
		_In_ GameInputCallbackToken callbackToken,
		_In_ void* context,
		_In_ IGameInputDevice* device,
		_In_ uint64_t timestamp,
		_In_ GameInputDeviceStatus currentStatus,
		_In_ GameInputDeviceStatus previousStatus ) noexcept;

	static void CALLBACK OnDeviceRead(
		_In_ GameInputCallbackToken callbackToken,
		_In_ void* context,
		_In_ IGameInputReading* reading ) noexcept;

	// Builds a DeviceIdentifier from a GameInput device
	static DeviceEnums::DeviceIdentifier GetIdentifier( IGameInputDevice* device );

	// mutex to protect m_deviceSlots
	std::mutex m_deviceMutex;
	// mutext to protect readings of devices
	std::mutex m_readingMutex;

	// tokens for registered GameInput callbacks, so we can unregister them on teardown
	std::vector<std::pair<DeviceEnums::DeviceId, GameInputCallbackToken>> m_deviceReadCallbackTokens = {};
	GameInputCallbackToken m_deviceCallbackToken = 0;

	// the GameInput interface
	IGameInput* m_gameInput = nullptr;
	// all recognized devices will be stored in this vector.
	std::vector<DeviceSlot> m_deviceSlots = {};

	// states that have been accumulated for each device, indexed by deviceID. Updated on each reading callback, and read by Update() to return the latest state for a device.
	std::unordered_map<DeviceEnums::DeviceId, std::vector<Events::State>> m_accumulatedStates = {};

	DEVICE_CHANGED_CALLBACK m_deviceAddedCallback = nullptr;
	DEVICE_CHANGED_CALLBACK m_deviceRemovedCallback = nullptr;

	bool m_initialized = false;
	bool m_devicesRemoved = false;

	static const GameInputKind SUPPORTED_INPUTS = static_cast<GameInputKind>(
	GameInputKindGamepad |
	GameInputKindController );
};
#endif