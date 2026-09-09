#pragma once

#include "../../StdAfx.h"
#include "../IInputEvent.h"
#include "../../InputElement.h"

/**
 * @brief Input event that matches when a controller analog axis changes value.
 *
 * Tracks the current and previous axis value and computes a delta.
 * Small changes below Events::AXIS_THRESHOLD are ignored to filter noise.
 */
BLUE_CLASS( ControllerAxisInputEvent ) :
	public IInputEvent
{
public:
	EXPOSE_TO_BLUE();

	/**
	 * @brief Constructs a ControllerAxisInputEvent.
	 * @param lockobj Optional parent lock object for thread safety.
	 */
	ControllerAxisInputEvent( IRoot* lockobj = nullptr );

	/**
	 * @brief Tests whether the configured axis has changed beyond the threshold.
	 * @param state The current device state.
	 * @return true if the axis value changed significantly, false otherwise.
	 */
	bool Match( const Events::State& state ) override;

	/**
	 * @brief Marks the matched axis as owned and updates the stored value and delta.
	 * @param state The device state to modify.
	 */
	void Own( Events::State& state ) override;

	void AttachTo( const InputElement* input );

private:
	bool m_initialized = false; ///< Whether the initial axis value has been captured.

	DeviceEnums::InputElementDescriptor m_element{ DeviceEnums::InputElementDescriptor::Unknown }; ///< Input element to monitor for switch position changes.
	uint32_t m_index{ 0 }; ///< Index of the input element in the device's element array.
	bool m_attached{ false }; ///< Whether the input element has been attached to a physical input.

	float m_value = 0.0f; ///< Last known axis value.
	float m_delta = 0.0f;       ///< Change since last update.
};

TYPEDEF_BLUECLASS( ControllerAxisInputEvent );