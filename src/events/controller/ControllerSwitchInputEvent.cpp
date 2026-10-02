// Copyright © 2026 CCP ehf.

#include "ControllerSwitchInputEvent.h"

namespace
{

Events::Switch* GetSwitchState( Events::State& state, DeviceEnums::ElementKey key )
{
	auto it = state.switches.find( key );
	if( it != state.switches.end() )
	{
		return &it->second;
	}
	CCP_LOGERR( "Switch state not found for switch %s", DeviceEnums::ToKeyString( key.descriptor ) );
	return nullptr;
}


const Events::Switch* GetSwitchState( const Events::State& state, DeviceEnums::ElementKey key )
{
	auto it = state.switches.find( key );
	if( it != state.switches.end() )
	{
		return &it->second;
	}
	CCP_LOGERR( "Switch state not found for switch %s", DeviceEnums::ToKeyString( key.descriptor ) );
	return nullptr;
}
}

ControllerSwitchInputEvent::ControllerSwitchInputEvent( IRoot* lockobj ) :
	InputEvent( lockobj )
{
}

void ControllerSwitchInputEvent::BeforeEvaluate()
{
	m_previousState = m_state;
	m_previouslyConditionsMet = m_conditionsMet;
}

bool ControllerSwitchInputEvent::JustMatched()
{
	if( m_event == Events::SwitchPosition::Any )
	{
		return m_matched && m_previousState != m_state;
	}
	return m_matched && !m_previouslyConditionsMet;
}

bool ControllerSwitchInputEvent::Evaluate( const Events::State& state )
{
	m_conditionsMet = false;
	if( m_attached )
	{
		if( const auto* switchState = GetSwitchState( state, m_key ) )
		{
			// the position is always tracked, even when it does not match or is owned elsewhere, so that
			// edges are reported against what the switch actually did
			m_state = switchState->position;
			switch( m_event )
			{
			case Events::SwitchPosition::Any:
				m_conditionsMet = true;
				break;
			case Events::SwitchPosition::NonCenter:
				m_conditionsMet = m_state != Events::SwitchPosition::Center;
				break;
			default:
				m_conditionsMet = m_state == m_event;
				break;
			}
			return m_conditionsMet && !switchState->matched;
		}
	}

	return false;
}

void ControllerSwitchInputEvent::Own( Events::State& state, bool /*combo*/ )
{
	if( !m_attached )
	{
		CCP_LOGERR( "ControllerSwitchInputEvent::Own: Cannot own switch state because no input element is attached." );
		return;
	}

	if( auto* switchState = GetSwitchState( state, m_key ) )
	{
		switchState->matched = true;
	}
}

bool ControllerSwitchInputEvent::AcceptsElement( DeviceEnums::InputElementDescriptor element ) const
{
	if( element == DeviceEnums::InputElementDescriptor::DPad )
	{
		return true;
	}

	CCP_LOGERR( "ControllerSwitchInputEvent::AttachTo: Invalid input element for switch: %s. Ignoring the assignment", DeviceEnums::ToKeyString( element ) );
	return false;
}
