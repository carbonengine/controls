#include "ControllerAxisInputEvent.h"

ControllerAxisInputEvent::ControllerAxisInputEvent( IRoot* lockobj )
{
}

bool ControllerAxisInputEvent::Match( const Events::State& state )
{
	if( m_attached && state.axis.size() > 0 )
	{
		auto it = std::find_if( state.axis.begin(), state.axis.end(), [&]( const Events::Axis& axis ) {
			return axis.descriptor == m_element && axis.index == m_index;
		} );

		if( it == state.axis.end() )
		{
			CCP_LOGERR( "ControllerAxisInputEvent::Match: Could not find axis state for element %s index %d", DeviceEnums::ToKeyString( m_element ), m_index );
			return false;
		}

		const auto& axis = *it;

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
	if( !m_attached )
	{
		CCP_LOGERR( "ControllerAxisInputEvent::Own: Cannot own axis state because no input element is attached." );
		return;
	}
	auto it = std::find_if( state.axis.begin(), state.axis.end(), [&]( const Events::Axis& axis ) {
		return axis.descriptor == m_element && axis.index == m_index;
	} );

	if( it == state.axis.end() )
	{
		CCP_LOGERR( "ControllerAxisInputEvent::Own: Could not find axis state for element %s index %d", DeviceEnums::ToKeyString( m_element ), m_index );
		return;
	}

	auto& axis = *it;

	m_delta = axis.value - m_value;
	m_value = axis.value;
	axis.matched = true;
}

void ControllerAxisInputEvent::AttachTo( const InputElement* input )
{
	m_attached = false;
	m_element = DeviceEnums::InputElementDescriptor::Unknown;
	m_index = 0;
	// check if the input is valid and is an axis
	if( input )
	{
		auto element = input->GetElement();

		switch( element )
		{
		case DeviceEnums::InputElementDescriptor::LeftStickX:
		case DeviceEnums::InputElementDescriptor::LeftStickY:
		case DeviceEnums::InputElementDescriptor::RightStickX:
		case DeviceEnums::InputElementDescriptor::RightStickY:
		case DeviceEnums::InputElementDescriptor::LeftTriggerAxis:
		case DeviceEnums::InputElementDescriptor::RightTriggerAxis:
		case DeviceEnums::InputElementDescriptor::Unknown:
			// valid input element for axis
			break;
		default:
			// invalid input element for axis
			CCP_LOGERR( "ControllerAxisInputEvent::SetInput: Invalid input element for axis: %s. Ignoring the assignment", DeviceEnums::ToKeyString( element ) );
			return;
		}
		m_attached = true;
		m_element = element;
		m_index = input->GetIndex();
	}
}