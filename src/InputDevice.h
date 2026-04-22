#pragma once
#include "StdAfx.h"
#include "DeviceEnums.h"
#include "events/Events.h"
#include "events/InputEventTrigger.h"
#include <string>
#include "handlers/IInputHandler.h"

BLUE_DECLARE_VECTOR( InputEventTrigger );

/// @brief Type alias for a raw portion of a device identifier.
typedef uint32_t RawDeviceIdPart;

BLUE_DECLARE_STRUCTURE_LIST( RawDeviceIdPart );

/**
 * @brief Represents a single connected input device and its event triggers.
 *
 * An InputDevice holds the device's identity, its current hardware state,
 * and a sorted list of InputEventTrigger objects. On each Update(), the
 * device reads new states from the IInputHandler and processes them
 * through its triggers in priority order (most-specific first).
 *
 * Rumble output is written back to the handler when rumble values change.
 */
BLUE_CLASS( InputDevice ) :
	public INotify,
	public IListNotify
{
public:
	EXPOSE_TO_BLUE();

	/**
	 * @brief Constructs an InputDevice.
	 * @param lockobj Optional parent lock object for thread safety.
	 */
	InputDevice( IRoot* lockobj = nullptr );

	/**
	 * @brief Notification callback when the trigger list is modified.
	 *
	 * Marks the sorted trigger cache as dirty so it is rebuilt on the next Update().
	 */
	void OnListModified(
		long event,
		ssize_t key,
		ssize_t key2,
		IRoot* value,
		const struct IList* theList ) override;

	/**
	 * @brief Notification callback when a Blue variable is modified.
	 *
	 * Detects changes to rumble values and flags them for the next Update().
	 *
	 * @param value The modified variable.
	 * @return true always.
	 */
	bool OnModified( Be::Var * value ) override;

	/**
	 * @brief Sets the device identifier metadata.
	 * @param identifier The DeviceIdentifier describing this device.
	 */
	void SetIdentifier( DeviceEnums::DeviceIdentifier identifier );

	/**
	 * @brief Reads new input states from the handler and processes all triggers.
	 * @param inputHandler The platform input handler to poll.
	 */
	void Update( IInputHandler* inputHandler );

	/**
	 * @brief Returns the unique device identifier string.
	 * @return The device ID.
	 */
	BlueSharedString GetDeviceID() const;

	/**
	 * @brief Returns the human-readable device name.
	 * @return The device name.
	 */
	BlueSharedString GetName() const;

private:
	/**
	 * @brief Processes a single state snapshot through all sorted triggers.
	 * @param state The device state to process.
	 */
	void UpdateState( const Events::State& state );

	DeviceEnums::DeviceIdentifier m_deviceIdentifier {}; ///< Device metadata.
	PRawDeviceIdPartStructureList m_rawDeviceId;          ///< Raw device ID parts exposed to script.

	PInputEventTriggerVector m_triggers;                  ///< Event triggers attached to this device.
	std::vector<InputEventTrigger*> m_sortedTriggers;     ///< Triggers sorted by event count (descending).
	Events::State m_currentState;                         ///< Latest device state.
	bool m_triggersDirty = false;                          ///< Whether the sorted trigger cache needs rebuilding.

	Events::Rumble m_rumble{};   ///< Current rumble motor intensities.
	bool m_updateRumble = false; ///< Whether rumble values have changed since last Update().
};

TYPEDEF_BLUECLASS( InputDevice );
