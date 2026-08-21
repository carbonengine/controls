#pragma once
#include "IInputHandler.h"

/**
 * @brief Stub implementation of IInputHandler that performs no real I/O.
 *
 * Used on platforms where no native input backend is available (e.g. non-Windows builds).
 * All methods are no-ops and Update() always returns an empty state vector.
 */
class InputHandlerStub : public IInputHandler
{
public:
	InputHandlerStub() = default;

	/** @copydoc IInputHandler::Initialize() */
	bool Initialize() override;

	/** @copydoc IInputHandler::SetDeviceActivation() */
	void SetDeviceActivation( BlueSharedString deviceId, bool activate ) override;

	/** @copydoc IInputHandler::Update() */
	std::vector<Events::State> Update( BlueSharedString deviceId ) override;

	/** @copydoc IInputHandler::RegisterForDeviceAdded() */
	void RegisterForDeviceAdded( DeviceChangedCallback callback ) override;

	/** @copydoc IInputHandler::RegisterForDeviceRemoved() */
	void RegisterForDeviceRemoved( DeviceChangedCallback callback ) override;

	/** @copydoc IInputHandler::Rumble() */
	void Rumble( BlueSharedString deviceId, Events::Rumble rumble ) override;
	
	/** @copydoc IInputHandler::GetButtonNames() */
	const std::vector<BlueSharedString> GetButtonNames( BlueSharedString deviceId ) override;
	
	/** @copydoc IInputHandler::GetAxisNames() */
	const std::vector<BlueSharedString> GetAxisNames( BlueSharedString deviceId ) override;
	
	/** @copydoc IInputHandler::GetSwitchNames() */
	const std::vector<BlueSharedString> GetSwitchNames( BlueSharedString deviceId ) override;

	/** @copydoc IInputHandler::SetBackgroundEventsEnabled() */
	void SetBackgroundEventsEnabled( bool enabled ) override;
};
