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
	void RegisterForDeviceAdded( DeviceChangedCallback callback ) override;
	void RegisterForDeviceRemoved( DeviceChangedCallback callback ) override;
	std::vector<Events::State> Update( BlueSharedString deviceId ) override;
	void SetDeviceActivation( BlueSharedString deviceId, bool activate ) override;
	void Rumble( BlueSharedString deviceId, Events::Rumble rumble ) override;

private:
	// Per-device bookkeeping
	struct DeviceSlot
	{
		CComPtr<IGameInputDevice> device = nullptr;
		bool pendingRemoval = false; // set to true when we receive a disconnect event, until the slot is cleaned up on the next Update()
		DeviceEnums::DeviceIdentifier identifier{};
		std::vector<Events::State> accumulatedStates{}; // states that have been accumulated for this device, updated on each reading callback, and read by Update() to return the latest state for the device
		GameInputCallbackToken readCallbackToken = 0; // token for the registered reading callback for this device, so we can unregister it on teardown or when deactivating the device
	};

	DeviceSlot* GetDeviceSlot( BlueSharedString deviceID );
	DeviceSlot* GetDeviceSlot( CComPtr<IGameInputDevice> device );

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
	mutable std::shared_mutex m_deviceMutex;
	// mutex to protect readings of all devices
	mutable std::shared_mutex m_readingMutex; 

	// tokens for registered GameInput callbacks, so we can unregister them on teardown
	GameInputCallbackToken m_deviceCallbackToken = 0;

	// the GameInput interface
	CComPtr<IGameInput> m_gameInput = nullptr;
	// all recognized devices will be stored in this vector.
	std::vector<DeviceSlot> m_deviceSlots = {};

	DeviceChangedCallback m_deviceAddedCallback = nullptr;
	DeviceChangedCallback m_deviceRemovedCallback = nullptr;

	bool m_initialized = false;
	bool m_devicesRemoved = false;

	static const GameInputKind SUPPORTED_INPUTS = static_cast<GameInputKind>(
	GameInputKindGamepad |
	GameInputKindController );
};
#endif