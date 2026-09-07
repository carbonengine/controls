#include "InputElement.h"

InputElement::InputElement( IRoot* lockobj )
{
}

void InputElement::Initialize( DeviceEnums::InputElementDescriptor element, uint32_t index )
{
	m_element = element;
	m_index = index;
}

DeviceEnums::InputElementDescriptor InputElement::GetElement() const
{
	return m_element;
}

uint32_t InputElement::GetIndex() const
{
	return m_index;
}
