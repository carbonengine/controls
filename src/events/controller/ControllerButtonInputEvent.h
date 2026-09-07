#pragma once

#include "../../StdAfx.h"
#include "../IInputEvent.h"
#include "../../InputElement.h"

/**
 * @brief Input event that matches a controller button reaching a specific ButtonState.
 *
 * Tracks press/release transitions and timestamps to distinguish between
 * Pressed (short tap), Held (long press), Released, Up, and Down states.
 */
BLUE_CLASS( ControllerButtonInputEvent ) :
	public IInputEvent
{
public:

	EXPOSE_TO_BLUE();

	/**
	 * @brief Constructs a ControllerButtonInputEvent.
	 * @param lockobj Optional parent lock object for thread safety.
	 */
	ControllerButtonInputEvent( IRoot* lockobj = nullptr );

	/**
	 * @brief Tests whether the configured button has reached the target ButtonState.
	 * @param state The current device state.
	 * @return true if the button matches the configured event state, false otherwise.
	 */
	bool Match( const Events::State& state ) override;

	/**
	 * @brief Marks the matched button as owned in the device state.
	 * @param state The device state to modify.
	 */
	void Own( Events::State& state ) override;
	
	void SetInput( InputElement * input );
	InputElement* GetInput() const;

private:
	InputElement* m_input; ///< The input element to monitor.
	Events::ButtonState m_event{ Events::ButtonState::Pressed }; ///< Target button state to match.

	bool m_previouslyPressed = false;          ///< Whether the button was pressed on the previous update.
	uint64_t m_previousStateChangeTimestamp = 0; ///< Timestamp of the last press/release transition.
};

TYPEDEF_BLUECLASS( ControllerButtonInputEvent );