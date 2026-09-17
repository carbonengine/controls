#include "InputEvent.h"

InputEvent::InputEvent( IRoot* lockobj )
{
}

bool InputEvent::Match( const Events::State& state )
{
	BeforeEvaluate();
	m_matched = Evaluate( state );
	return m_matched;
}

void InputEvent::AttachTo( const InputElement* input )
{
	m_attached = false;
	m_key = {};

	if( !input )
	{
		return;
	}

	const auto element = input->GetElement();
	if( !AcceptsElement( element ) )
	{
		return;
	}

	m_key = { element, input->GetIndex() };
	m_attached = true;
}

void InputEvent::BeforeEvaluate()
{
	CCP_ASSERT_M( true, "InputEvent::BeforeEvaluate: Subclasses must implement BeforeEvaluate to capture per-type tracking state." );
}

void InputEvent::Own( Events::State& state )
{
	CCP_ASSERT_M( true, "InputEvent::Own: Subclasses must implement Own to claim matched state." );
}

bool InputEvent::JustMatched()
{
	CCP_ASSERT_M( true, "InputEvent::JustMatched: Subclasses must implement JustMatched to check if the event has just matched." );
	return false;
}

bool InputEvent::Evaluate( const Events::State& state )
{
	CCP_ASSERT_M( true, "InputEvent::Evaluate: Subclasses must implement Evaluate to check if the event matches the given state." );
	return false;
}

bool InputEvent::AcceptsElement( DeviceEnums::InputElementDescriptor element ) const
{
	CCP_ASSERT_M( true, "InputEvent::AcceptsElement: Subclasses must implement AcceptsElement to check if the event can monitor the given element." );
	return false;
}