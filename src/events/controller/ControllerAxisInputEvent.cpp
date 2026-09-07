#include "ControllerAxisInputEvent.h"

ControllerAxisInputEvent::ControllerAxisInputEvent( IRoot* lockobj )
{
}

bool ControllerAxisInputEvent::Match( const Events::State& state )
{
	if( m_input )
	{
		auto element = m_input->GetElement();
		auto index = m_input->GetIndex();
		auto it = std::find_if( state.axis.begin(), state.axis.end(), [element, index]( const Events::Axis& axis ) {
			return axis.descriptor == element && axis.index == index;
		} );
		if( it == state.axis.end() )
		{
			return false; // axis not found
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
	auto element = m_input->GetElement();
	auto index = m_input->GetIndex();
	auto it = std::find_if( state.axis.begin(), state.axis.end(), [element, index]( const Events::Axis& axis ) {
		return axis.descriptor == element && axis.index == index;
	} );

	auto& axis = *it;

	m_delta = axis.value - m_value;
	m_value = axis.value;
	axis.matched = true;
}

void ControllerAxisInputEvent::SetInput( InputElement* input )
{
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
	}
	m_input = input;
}

InputElement* ControllerAxisInputEvent::GetInput() const
{
	return m_input;
}