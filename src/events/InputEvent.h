#pragma once
#include "../StdAfx.h"
#include "Events.h"
#include "../InputElement.h"

/**
 * @brief Base class for input events that match a single device element against a condition.
 *
 * Owns the element identity and the attach/match bookkeeping shared by every event type.
 * Subclasses supply only the behaviour that varies by element kind: which descriptors they
 * accept, how a state snapshot is evaluated, and how a matched state is claimed.
 */
BLUE_CLASS( InputEvent ) : public IRoot
{
public:
	EXPOSE_TO_BLUE();

	InputEvent( IRoot* lockobj = nullptr );

	/**
	 * @brief Evaluates this event against the given state and records the result.
	 * @param state The current device state to evaluate.
	 * @return true if the state satisfies the event condition, false otherwise.
	 */
	bool Match( const Events::State& state );

	/**
	 * @brief Marks this event's element in @p state as claimed so other triggers skip it.
	 */
	virtual void Own( Events::State& state );

	/**
	 * @brief Checks whether the event has just matched since the last evaluation.
	 * @return true if the event has just matched, false otherwise.
	 */
	virtual bool JustMatched();

	/**
	 * @brief Binds this event to a device element.
	 *
	 * Elements this event type does not accept leave the event unattached.
	 *
	 * @param input The element to monitor, or nullptr to detach.
	 */
	void AttachTo( const InputElement* input );

protected:
	/// @brief Captures per-type tracking state before the next evaluation.
	virtual void BeforeEvaluate();

	/**
	 * @brief Evaluates the match condition and updates the internal tracking state.
	 * @param state The current device state to evaluate.
	 * @return true if the state satisfies the event condition, false otherwise.
	 */
	virtual bool Evaluate( const Events::State& state );

	/// @brief Whether this event type can monitor @p element; logs the reason when it cannot.
	virtual bool AcceptsElement( DeviceEnums::InputElementDescriptor element ) const;

	Events::ElementKey m_key{}; ///< Identifies the device element this event monitors.
	bool m_attached{ false };   ///< Whether an input element has been attached to a physical input.
	bool m_matched{ false };    ///< Result of the most recent evaluation.
};

TYPEDEF_BLUECLASS( InputEvent );
BLUE_DECLARE_VECTOR( InputEvent );
