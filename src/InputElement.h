#pragma once
#include "StdAfx.h"
#include "DeviceEnums.h"

/**
 * @brief Represents a single input element used for event registry.
 */
BLUE_CLASS( InputElement ) : public IRoot
{
public:
	EXPOSE_TO_BLUE();
	InputElement( IRoot* lockobj = nullptr );
	void Initialize( DeviceEnums::InputElementDescriptor element, uint32_t index );

	DeviceEnums::InputElementDescriptor GetElement() const;
	uint32_t GetIndex() const;

private:
	DeviceEnums::InputElementDescriptor m_element{ DeviceEnums::InputElementDescriptor::Unknown }; ///< Canonical element identifier for this element.
	uint32_t m_index{ 0 }; ///< Index of this element in the device's element array.
};
TYPEDEF_BLUECLASS( InputElement );
BLUE_DECLARE_VECTOR( InputElement );
