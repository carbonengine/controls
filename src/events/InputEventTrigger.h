#pragma once
#include "../StdAfx.h"
#include <BlueScriptCallback.h>

#include "IInputEvent.h"

/**
 * @brief Groups one or more IInputEvent conditions and fires a callback when all match.
 *
 * An InputEventTrigger owns a list of IInputEvent objects. During processing,
 * each event is evaluated against the current device state. If every event
 * matches, the trigger's script callback is invoked and the matched portions
 * of the state are marked as owned to prevent duplicate firing.
 *
 * Triggers with more events are processed first so that more-specific
 * combinations take priority over less-specific ones.
 */
BLUE_CLASS( InputEventTrigger ) : public IRoot
{
public:
	EXPOSE_TO_BLUE();

	/**
	 * @brief Constructs an InputEventTrigger.
	 * @param lockobj Optional parent lock object for thread safety.
	 */
	InputEventTrigger( IRoot* lockobj = nullptr );

	/**
	 * @brief Evaluates all owned events against the device state and fires the callback on match.
	 *
	 * All events are always evaluated (even after a mismatch) so that their
	 * internal state (e.g. held-button timers) stays up to date.
	 *
	 * @param state The current device state; matched parts will be marked as owned.
	 */
	void Process( Events::State& state );

	/**
	 * @brief Returns the number of IInputEvent conditions in this trigger.
	 * @return Event count.
	 */
	size_t GetEventCount() const;

private:
	BlueScriptCallback m_callback;  ///< Script callback invoked when all events match.
	PIInputEventVector m_events;    ///< Collection of input event conditions.
};

TYPEDEF_BLUECLASS( InputEventTrigger );