// Copyright © 2026 CCP ehf.

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
	void Initialize( const DeviceEnums::ElementKey& key );

	const DeviceEnums::ElementKey& GetKey() const;

private:
	DeviceEnums::ElementKey m_key{}; ///< Identity this element was published under by the handler.
};
TYPEDEF_BLUECLASS( InputElement );
BLUE_DECLARE_VECTOR( InputElement );
