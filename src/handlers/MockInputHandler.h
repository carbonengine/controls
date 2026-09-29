#pragma once
#include "IInputHandler.h"

/**
 * @brief Test implementation of IInputHandler whose devices and input state are driven by the caller.
 *
 * Installed at runtime through ControlManager::EnableMockInputHandler(), exposed to Blue as _EnableMockInputHandler. Devices are
 * added and removed explicitly, and every input change produces a state snapshot that is
 * returned from the next Update() call. Time is driven by an internal clock that only moves
 * when AdvanceTime() is called, so hold-time based events are deterministic.
 */
class MockInputHandler : public IInputHandler
{
public:
	MockInputHandler();
	~MockInputHandler() override;

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

	/**
	 * @brief Connects a fake device and notifies the ControlManager.
	 * @return false if a device with the same ID is already connected.
	 */
	bool AddDevice( const DeviceEnums::DeviceIdentifier& identifier );

	/**
	 * @brief Disconnects a fake device and notifies the ControlManager.
	 * @return false if no device with the given ID is connected.
	 */
	bool RemoveDevice( BlueSharedString deviceId );

	/// @brief Sets the pressed state of a button and queues a state snapshot.
	bool SetButton( BlueSharedString deviceId, DeviceEnums::ElementKey key, bool pressed );

	/// @brief Sets the value of an axis and queues a state snapshot.
	bool SetAxis( BlueSharedString deviceId, DeviceEnums::ElementKey key, float value );

	/// @brief Sets the position of a switch and queues a state snapshot.
	bool SetSwitch( BlueSharedString deviceId, DeviceEnums::ElementKey key, Events::SwitchPosition position );

	/// @brief Advances the mock clock used to timestamp state snapshots.
	void AdvanceTime( uint64_t microseconds );

	/// @brief Returns the current mock clock value in microseconds.
	uint64_t GetTime() const;

	/// @brief Returns whether the given device has been activated by the ControlManager.
	bool IsDeviceActive( BlueSharedString deviceId ) const;

	/// @brief Returns the last rumble sent to the given device.
	Events::Rumble GetRumble( BlueSharedString deviceId ) const;

	/// @brief Returns whether background events were enabled.
	bool GetBackgroundEventsEnabled() const;

private:
	struct MockDevice
	{
		DeviceEnums::DeviceIdentifier identifier;
		Events::State current;
		std::vector<Events::State> pending;
		Events::Rumble rumble;
		bool active = false;
	};

	MockDevice* FindDevice( BlueSharedString deviceId );
	const MockDevice* FindDevice( BlueSharedString deviceId ) const;
	void QueueSnapshot( MockDevice& device );

	std::map<std::string, MockDevice> m_devices;
	DeviceChangedCallback m_deviceAdded;
	DeviceChangedCallback m_deviceRemoved;
	uint64_t m_time = 0;
	bool m_backgroundEventsEnabled = false;
};
