// Copyright © 2026 CCP ehf.

#include "InputEventTrigger.h"

InputEventTrigger::InputEventTrigger( IRoot* lockobj ) :
	PARENTLOCK( m_events )
{
}

void InputEventTrigger::Process( Events::State& state )
{
	bool matches = true;
	bool justMatched = false;
	// need to check all events, even if one fails or the trigger is disabled, to properly update their
	// internal state (e.g. for held events) so that re-enabling does not report stale edges
	for( auto& event : m_events )
	{
		matches &= event->Match( state );
		justMatched |= event->JustMatched();
	}

	if( !m_callback || !m_enabled || !matches || m_events.size() == 0 )
	{
		m_firedAndStillMatching = false;
		return;
	}

	const bool fire = m_repeat || justMatched;
	if( !fire && !m_firedAndStillMatching )
	{
		// matching, but the match was never acted on (e.g. it started while another trigger owned the
		// state), so leave the state for other triggers
		return;
	}

	// tag the state with the events that matched so that they won't be considered for identical events.
	// A trigger that already fired keeps owning the state for as long as it keeps matching, so smaller
	// triggers stay blocked while the combination is still active.
	const bool combo = m_events.size() > 1;
	for( auto& event : m_events )
	{
		event->Own( state, combo );
	}
	m_firedAndStillMatching = true;

	if( fire )
	{
		m_callback.CallVoid( m_events.GetRawRoot() );
	}
}

size_t InputEventTrigger::GetEventCount() const
{
	return m_events.size();
}
