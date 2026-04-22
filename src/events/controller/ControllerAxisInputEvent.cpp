#include "ControllerAxisInputEvent.h"

ControllerAxisInputEvent::ControllerAxisInputEvent( IRoot* lockobj )
{
}

bool ControllerAxisInputEvent::Match( const Events::State& state )
{
	if( m_axisIndex < state.axis.size() )
	{
		const auto& axis = state.axis[m_axisIndex];

		if( axis.matched )
		{
			return false; // already matched by another event
		}
		if( !m_initialized )
		{
			m_delta = axis.value - m_value;
			m_value = axis.value;
			m_initialized = true;
		}
		if( std::abs( axis.value - m_value ) < Events::AXIS_THRESHOLD )
		{
			return false; // ignore small changes in axis value
		}

		return true;
	}
	return false;
}

void ControllerAxisInputEvent::Own( Events::State& state )
{
	auto& axis = state.axis[m_axisIndex];

	m_delta = axis.value - m_value;
	m_value = axis.value;
	axis.matched = true;
}
