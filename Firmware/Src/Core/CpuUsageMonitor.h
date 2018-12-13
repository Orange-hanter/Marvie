#pragma once

#include "cpp_wrappers/ch.hpp"

using namespace chibios_rt;

class CpuUsageMonitor
{
	friend void cpuUsageMonitorIdleEnterHookCpp();
	friend void cpuUsageMonitorIdleLeaveHookCpp();

	CpuUsageMonitor();
	~CpuUsageMonitor();

public:
	enum Event : eventflags_t { DataUpdated };

	static CpuUsageMonitor* instance();
	static inline CpuUsageMonitor* tryInstance() { return inst; }

	uint32_t usage();

	inline EvtSource* eventSource() { return &eSource; };

private:
	inline void idleEnter();
	inline void idleLeave();
	static void timerCallback( void* );

private:
	static CpuUsageMonitor* inst;
	virtual_timer_t timer;
	volatile bool idleInside;
	uint32_t irqCount;
	rtcnt_t idleLast;
	rtcnt_t idleSum;
	uint32_t cpuUsage;
	EvtSource eSource;
};