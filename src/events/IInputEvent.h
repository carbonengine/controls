#pragma once
#include "../StdAfx.h"
#include "Events.h"

/**
 * @brief Interface for matching a device state against an input event condition.
 *
 * Implementations check whether a particular part of the device state
 * (e.g. a button press, an axis movement) matches a configured condition.
 */
BLUE_INTERFACE( IInputEvent ) :
	public IRoot
{
public:
	/**
	 * @brief Tests whether the given device state matches this event's condition.
	 * @param state The current device state to evaluate.
	 * @return true if the state satisfies the event condition, false otherwise.
	 */
	virtual bool Match( const Events::State& state ) = 0;

	/**
	 * @brief Marks the matched parts of the state as owned so they are not
	 *        consumed by other event triggers with identical conditions.
	 * @param state The device state to mark.
	 */
	virtual void Own( Events::State& state ) = 0;

	/**
	 * @brief Checks whether the event has just matched since the last evaluation.
	 * @return true if the event has just matched, false otherwise.
	 */
	virtual bool JustMatched( ) = 0;
};
BLUE_DECLARE_INTERFACE( IInputEvent );
BLUE_DECLARE_IVECTOR( IInputEvent );
