#pragma once

#include "IInputHandler.h"
#include <GameInput.h>
#include <array>
#include <mutex>
#include "../events/IInputEvent.h"

class InputHandlerWin : public IInputHandler
{
public:
	InputHandlerWin();
	~InputHandlerWin();
	Events::State Update( DeviceEnums::DeviceId deviceID ) override;
	std::vector<DeviceEnums::DeviceIdentifier> GetAllDeviceIdentifiers() override;

private:
	// Per-device bookkeeping
	struct DeviceSlot
	{
		IGameInputDevice* device = nullptr;
		bool needDelete = false;
		DeviceEnums::DeviceIdentifier identifier{};
	};

	// GameInput setup / teardown
	bool InitializeGameInput();
	void ShutdownGameInput();

	void GetBatteryState( IGameInputReading* reading, IGameInputDevice* device, Events::BatteryState& state );
	void GetGamePadState( IGameInputReading* reading, IGameInputDevice* device, Events::GamePadState& state );
	void GetFlightStickState( IGameInputReading* reading, IGameInputDevice* device, Events::FlightStickState& state );

	// Reads the current hardware state for a single device into its slot
	Events::State ReadDeviceState( IGameInputDevice* device );

	// The static callback forwarded from GameInput when devices connect / disconnect
	static void CALLBACK OnDeviceStatusChanged(
		_In_ GameInputCallbackToken callbackToken,
		_In_ void* context,
		_In_ IGameInputDevice* device,
		_In_ uint64_t timestamp,
		_In_ GameInputDeviceStatus currentStatus,
		_In_ GameInputDeviceStatus previousStatus ) noexcept;

	// Builds a DeviceIdentifier from a GameInput device
	static DeviceEnums::DeviceIdentifier GetIdentifier( IGameInputDevice* device );

	IGameInput* m_gameInput = nullptr;
	std::vector<DeviceSlot> m_deviceSlots = {};
	std::mutex m_deviceMutex;
	bool m_initialized = false;
	GameInputCallbackToken m_deviceCallbackToken = 0;

	static const GameInputKind SUPPORTED_INPUTS =
		static_cast<GameInputKind>( GameInputKindGamepad | GameInputKindFlightStick );
};