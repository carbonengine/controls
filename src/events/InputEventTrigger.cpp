#include "InputEventTrigger.h"

InputEventTrigger::InputEventTrigger( IRoot* lockobj ) :
	PARENTLOCK( m_events )
{
}

void InputEventTrigger::Process( Events::State& state )
{
	if( !m_callback )
	{
		return;
	}

	bool matches = true;
	bool justMatched = false;
	// need to check all events, even if one fails, to properly update their internal state (e.g. for held events)
	for( auto &event: m_events )
	{
		matches &= event->Match( state );
		justMatched |= event->JustMatched();
	}

	if( !matches )
	{
		m_triggered = false;
		return;
	}

	// the combination of events only just became true if at least one of them changed to matching,
	// otherwise this is a continuation of a match that was already reported
	if( !m_repeat && !justMatched )
	{
		// the state is deliberately left unowned because this trigger is not acting on it,
		// so other triggers may still consume it
		return;
	}

	// tag the state with the events that matched so that they won't be considered for identical events
	for( auto& event : m_events )
	{
		event->Own( state );
	}

	m_triggered = true;
	m_callback.CallVoid( m_events.GetRawRoot() );
}

size_t InputEventTrigger::GetEventCount() const
{
	return m_events.size();
}