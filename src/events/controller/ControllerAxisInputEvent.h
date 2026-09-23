#pragma once

#include "../../StdAfx.h"
#include "../InputEvent.h"

/**
 * @brief Input event that matches when a controller analog axis changes value.
 *
 * Tracks the current and previous axis value and computes a delta.
 * Small changes below Events::AXIS_THRESHOLD are ignored to filter noise.
 */
BLUE_CLASS( ControllerAxisInputEvent ) :
	public InputEvent
{
public:
	EXPOSE_TO_BLUE();

	ControllerAxisInputEvent( IRoot* lockobj = nullptr );

	/** @copydoc InputEvent::Own() */
	void Own( Events::State & state ) override;

	/** @copydoc InputEvent::JustMatched() */
	bool JustMatched() override;

private:
	void BeforeEvaluate() override;
	/** @copydoc InputEvent::Evaluate() */
	bool Evaluate( const Events::State& state ) override;

	/** @copydoc InputEvent::AcceptsElement() */
	bool AcceptsElement( DeviceEnums::InputElementDescriptor element ) const override;

	bool m_initialized = false; ///< Whether the initial axis value has been captured.

	float m_value = 0.0f; ///< Last known axis value.
	float m_delta = 0.0f; ///< Change since last update.
};

TYPEDEF_BLUECLASS( ControllerAxisInputEvent );