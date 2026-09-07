#pragma once

#include "../../StdAfx.h"
#include "../IInputEvent.h"
#include "../../InputElement.h"

/**
 * @brief Input event that matches a controller hat/d-pad switch reaching a specific position.
 *
 * Supports matching a specific SwitchPosition or Any (any non-center position).
 */
BLUE_CLASS( ControllerSwitchInputEvent ) :
	public IInputEvent
{
public:

	EXPOSE_TO_BLUE();

	/**
	 * @brief Constructs a ControllerSwitchInputEvent.
	 * @param lockobj Optional parent lock object for thread safety.
	 */
	ControllerSwitchInputEvent( IRoot* lockobj = nullptr );

	/**
	 * @brief Tests whether the configured switch is in the target position.
	 * @param state The current device state.
	 * @return true if the switch position matches, false otherwise.
	 */
	bool Match( const Events::State& state ) override;

	/**
	 * @brief Marks the matched switch as owned and stores the current position.
	 * @param state The device state to modify.
	 */
	void Own( Events::State& state ) override;

	void SetInput( InputElement * input );
	InputElement* GetInput() const;

private:
	InputElement* m_input; ///< The input element to monitor.
	Events::SwitchPosition m_event{ Events::SwitchPosition::Any };    ///< Target switch position to match.
	Events::SwitchPosition m_state{ Events::SwitchPosition::Center }; ///< Last matched switch position.
};

TYPEDEF_BLUECLASS( ControllerSwitchInputEvent );