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

	ControllerAxisInputEvent( IRoot* lockobj = nullptr );

	/** @copydoc IInputEvent::Match() */
	bool Match( const Events::State& state ) override;

	/** @copydoc IInputEvent::Own() */
	void Own( Events::State & state ) override;

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