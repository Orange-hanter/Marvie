#pragma once

#include "Event.h"

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

	inline EventSourceRef eventSource() { return &eSource; };

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
	EventSource eSource;
};