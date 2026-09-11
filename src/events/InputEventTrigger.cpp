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
	// need to check all events, even if one fails, to properly update their internal state (e.g. for held events)
	for( auto &event: m_events )
	{
		matches &= event->Match( state );
	}

	if( matches )
	{
		// tag the state with the events that matched so that they won't be considered for identical events
		for( auto& event : m_events )
		{
			event->Own( state );
		}
		m_callback.CallVoid( m_events.GetRawRoot() );
	}
}

size_t InputEventTrigger::GetEventCount() const
{
	return m_events.size();
}