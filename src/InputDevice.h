#pragma once
#include "StdAfx.h"
#include "DeviceEnums.h"
#include "InputElement.h"
#include "events/Events.h"
#include "events/InputEventTrigger.h"
#include "handlers/IInputHandler.h"

#include <set>

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

	/**
	* @brief Gets the current intensity of the high frequency rumble motor.
	*/
	float GetHighFrequencyRumble() const;

	/**
	* @brief Sets the intensity of the high frequency rumble motor.
	* Ensures that the value gets sent to the input handler on the next Update() call.
	*/
	void SetHighFrequencyRumble( float value );

	/**
	* @brief Gets the current intensity of the low frequency rumble motor.
	*/
	float GetLowFrequencyRumble() const;

	/**
	* @brief Sets the intensity of the low frequency rumble motor.
	* Ensures that the value gets sent to the input handler on the next Update() call
	*/
	void SetLowFrequencyRumble( float value );

	/**
	* @brief Gets the current intensity of the left trigger rumble motor.
	*/
	float GetLeftTriggerRumble() const;

	/**
	* @brief Sets the intensity of the left trigger rumble motor.
	* Ensures that the value gets sent to the input handler on the next Update() call.
	*/
	void SetLeftTriggerRumble( float value );

	/**
	* @brief Gets the current intensity of the right trigger rumble motor.
	*/
	float GetRightTriggerRumble() const;

	/**
	* @brief Sets the intensity of the right trigger rumble motor.
	* Ensures that the value gets sent to the input handler on the next Update() call.
	*/
	void SetRightTriggerRumble( float value );

	/// @brief Zeroes all rumble intensities without scheduling a hardware write.
	void ResetRumble();

	/// @brief Hardware family this device belongs to.
	DeviceEnums::DeviceFamily GetDeviceFamily() const;

private:
	/**
	 * @brief Processes a single state snapshot through all sorted triggers.
	 * @param state The device state to process.
	 */
	void UpdateState( const Events::State& state );

	DeviceEnums::DeviceIdentifier m_deviceIdentifier{}; ///< Device metadata.
	PRawDeviceIdPartStructureList m_rawDeviceId; ///< Raw device ID parts exposed to script.

	PInputEventTriggerVector m_triggers; ///< Event triggers attached to this device.
	std::vector<InputEventTrigger*> m_sortedTriggers; ///< Triggers sorted by event count (descending).
	Events::State m_currentState; ///< Latest device state.
	bool m_triggersDirty = false; ///< Whether the sorted trigger cache needs rebuilding.
	std::set<DeviceEnums::ElementKey> m_spentButtons; ///< Held buttons claimed by a combination; their release is suppressed.

	Events::Rumble m_rumble{}; ///< Current rumble motor intensities.
	bool m_updateRumble = false; ///< Whether rumble values have changed since last Update().
	PInputElementVector m_buttons;
	PInputElementVector m_axes;
	PInputElementVector m_switches;
};

TYPEDEF_BLUECLASS( InputDevice );
