// Copyright © 2026 CCP ehf.

#pragma once
#include "../StdAfx.h"
#include <BlueScriptCallback.h>

#include "InputEvent.h"

/**
 * @brief Groups one or more InputEvent conditions and fires a callback when all match.
 *
 * An InputEventTrigger owns a list of InputEvent objects. During processing,
 * each event is evaluated against the current device state. If every event
 * matches, the trigger's script callback is invoked and the matched portions
 * of the state are marked as owned to prevent duplicate firing.
 *
 * Triggers with more events are processed first so that more-specific
 * combinations take priority over less-specific ones. Triggers with the same
 * number of events are processed in the order they were added.
 *
 * Ownership rules:
 * - A trigger that fired keeps owning its elements for as long as all of its
 *   events keep matching, so smaller triggers stay blocked while it is active.
 * - A button owned by a trigger with more than one event is "spent": its release
 *   will not produce Pressed/Released matches on any trigger.
 * - A disabled trigger (or one without a callback) keeps tracking its events but
 *   never fires or owns anything.
 *
 * Limitation: every input change is evaluated as its own state, in the order it
 * was reported. Inputs that change "together" are therefore seen one after the
 * other, and a smaller trigger can complete (and fire) before the state that
 * would complete a larger combination arrives.
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
	 * If this is not a repeat trigger, the callback is only invoked on the first
	 * state that matches. The events must stop matching before it can fire again.
	 *
	 * @param state The current device state; matched parts will be marked as owned.
	 */
	void Process( Events::State& state );

	/**
	 * @brief Returns the number of InputEvent conditions in this trigger.
	 * @return Event count.
	 */
	size_t GetEventCount() const;

private:
	BlueScriptCallback m_callback; ///< Script callback invoked when all events match.
	PInputEventVector m_events; ///< Collection of input event conditions.
	bool m_repeat{ false }; ///< Whether the callback fires for every matching state instead of only the first one.
	bool m_enabled{ true }; ///< Whether the trigger is currently enabled and can fire.
	bool m_firedAndStillMatching{ false }; ///< Whether the trigger fired and all of its events have kept matching since.
};

TYPEDEF_BLUECLASS( InputEventTrigger );
