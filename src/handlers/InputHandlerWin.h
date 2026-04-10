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
	std::vector<DeviceEnums::DeviceIdentifier> GetAllDeviceIdentifiers() override;
	void RegisterForDeviceChange( std::function<void( std::vector<DeviceEnums::DeviceIdentifier> )> callback ) override;
	Events::State Update( DeviceEnums::DeviceId deviceId ) override;

private:
	// Per-device bookkeeping
	struct DeviceSlot
	{
		IGameInputDevice* device = nullptr;
		DeviceEnums::DeviceIdentifier identifier{};
	};

	// GameInput setup / teardown
	bool InitializeGameInput();
	void ShutdownGameInput();

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

	std::function<void( std::vector<DeviceEnums::DeviceIdentifier> )> m_deviceChangedCallback = nullptr;

	static const GameInputKind SUPPORTED_INPUTS = static_cast<GameInputKind>(
	GameInputKindGamepad |
	GameInputKindController );
};
#endif