// Copyright © 2026 CCP ehf.

#include "InputElement.h"

InputElement::InputElement( IRoot* lockobj )
{
}

void InputElement::Initialize( const DeviceEnums::ElementKey& key )
{
	m_key = key;
}

const DeviceEnums::ElementKey& InputElement::GetKey() const
{
	return m_key;
}
