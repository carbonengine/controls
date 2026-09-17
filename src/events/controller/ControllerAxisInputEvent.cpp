#include "ControllerAxisInputEvent.h"

namespace{

Events::Axis* GetAxisState( Events::State& state, DeviceEnums::ElementKey key )
{
    auto it = state.axis.find( key );
    if( it != state.axis.end() )
    {
        return &(it->second);
    }
    CCP_LOGERR( "Axis state not found for axis %s", DeviceEnums::ToKeyString( key.descriptor ) );
    return nullptr;
}

const Events::Axis* GetAxisState( const Events::State& state, DeviceEnums::ElementKey key )
{
    auto it = state.axis.find( key );
    if( it != state.axis.end() )
    {
        return &(it->second);
    }
    CCP_LOGERR( "Axis state not found for axis %s", DeviceEnums::ToKeyString( key.descriptor ) );
    return nullptr;
}

}

ControllerAxisInputEvent::ControllerAxisInputEvent( IRoot* lockobj ) :
	InputEvent( lockobj )
{
}

bool ControllerAxisInputEvent::JustMatched()
{
	// an axis only matches when its value moved past the threshold, so every match is a new event
	return m_matched;
}

bool ControllerAxisInputEvent::Evaluate( const Events::State& state )
{
	if( m_attached )
	{
		if( const auto* axis = GetAxisState( state, m_key ) )
		{
			if( axis->matched )
			{
				return false; // already matched by another event
			}
			if( !m_initialized )
			{
				// There is nothing to compare the first reading against, so adopt it as the baseline.
				m_initialized = true;
				m_value = axis->value;
				m_delta = 0.0f;
				return false;
			}
			if( std::abs( axis->value - m_value ) < Events::AXIS_THRESHOLD )
			{
				return false; // ignore small changes in axis value
			}

			return true;
		}
	}
	return false;
}

void ControllerAxisInputEvent::Own( Events::State& state )
{
	if( !m_attached )
	{
		CCP_LOGERR( "ControllerAxisInputEvent::Own: Cannot own axis state because no input element is attached." );
		return;
	}
	
	if( auto* axis = GetAxisState( state, m_key ) )
	{
		m_delta = axis->value - m_value;
		m_value = axis->value;
		axis->matched = true;
	}
}

bool ControllerAxisInputEvent::AcceptsElement( DeviceEnums::InputElementDescriptor element ) const
{
	switch( element )
	{
	case DeviceEnums::InputElementDescriptor::LeftStickX:
	case DeviceEnums::InputElementDescriptor::LeftStickY:
	case DeviceEnums::InputElementDescriptor::RightStickX:
	case DeviceEnums::InputElementDescriptor::RightStickY:
	case DeviceEnums::InputElementDescriptor::LeftTriggerAxis:
	case DeviceEnums::InputElementDescriptor::RightTriggerAxis:
	case DeviceEnums::InputElementDescriptor::Unknown:
		return true;
	default:
		CCP_LOGERR( "ControllerAxisInputEvent::AttachTo: Invalid input element for axis: %s. Ignoring the assignment", DeviceEnums::ToKeyString( element ) );
		return false;
	}
}