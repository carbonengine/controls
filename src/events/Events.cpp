#include "../StdAfx.h"
#include "Events.h"

namespace Events
{
bool Rumble::empty() const
{
	return lowFrequency == 0.0f && highFrequency == 0.0f && leftTrigger == 0.0f && rightTrigger == 0.0f;
}

uint64_t GetTimestamp()
{
	return std::chrono::duration_cast<std::chrono::microseconds>( std::chrono::steady_clock::now().time_since_epoch() ).count();
}
}