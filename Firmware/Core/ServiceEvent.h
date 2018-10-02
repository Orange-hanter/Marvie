#pragma once

#include "ch.h"

enum ServiceEvent : eventmask_t
{
	TimeoutServiceEvent = 1UL << 31,
	DataServiceEvent    = 1UL << 30,
	LastNonServiceEvent = 1UL << 29
};