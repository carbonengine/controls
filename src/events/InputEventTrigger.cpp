#include "InputEventTrigger.h"

InputEventTrigger::InputEventTrigger( IRoot* lockobj ) :
	PARENTLOCK( m_events )
{
}

void InputEventTrigger::Process( Events::State state )
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

	if( m_callback && matches )
	{
		m_callback.CallVoid();
	}
}