#include "ControllerSwitchInputEvent.h"

namespace {

Events::Switch* GetSwitchState( Events::State& state, DeviceEnums::ElementKey key)
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
}

bool ControllerSwitchInputEvent::JustMatched()
{
	return m_matched && m_previousState != m_state;
}

bool ControllerSwitchInputEvent::Evaluate( const Events::State& state )
{
	if( m_attached  )
	{
		if( const auto* switchState = GetSwitchState( state, m_key ) )
		{
			if( !switchState->matched && (switchState->position == m_event || m_event == Events::SwitchPosition::Any) )
			{
				m_state = switchState->position;
				return true;
			}
		}
	}

	return false;
}

void ControllerSwitchInputEvent::Own( Events::State& state )
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
