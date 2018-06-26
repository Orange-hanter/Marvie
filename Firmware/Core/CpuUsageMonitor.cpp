#include "CpuUsageMonitor.h"
#include "hal_lld.h"

CpuUsageMonitor* CpuUsageMonitor::inst = nullptr;

CpuUsageMonitor::CpuUsageMonitor()
{
	idleInside = false;
	irqCount = 0;
	idleLast = 0;
	idleSum = 0;
	cpuUsage = 0;
	chVTObjectInit( &timer );
	chVTSet( &timer, TIME_MS2I( 200 ), timerCallback, this );
}

CpuUsageMonitor::~CpuUsageMonitor()
{
	chVTReset( &timer );
}

CpuUsageMonitor* CpuUsageMonitor::instance()
{
	if( inst )
		return inst;
	inst = new CpuUsageMonitor;
	return inst;
}

uint32_t CpuUsageMonitor::usage()
{
	return cpuUsage;
}

void CpuUsageMonitor::idleEnter()
{
	idleInside = true;
	idleLast = chSysGetRealtimeCounterX();
}

void CpuUsageMonitor::idleLeave()
{
	idleInside = false;
	idleSum += chSysGetRealtimeCounterX() - idleLast;
}

void CpuUsageMonitor::timerCallback( void* p )
{
	CpuUsageMonitor* m = reinterpret_cast< CpuUsageMonitor* >( p );
	chSysLockFromISR();
	rtcnt_t t = chSysGetRealtimeCounterX();
	if( m->idleInside )
	{
		m->idleSum += t - m->idleLast;
		m->idleLast = t;
	}

	m->cpuUsage = 100 - ( RTC2MS( STM32_SYSCLK, ( m->idleSum ) ) >> 1 );

	m->idleSum = 0;
	chVTSetI( &m->timer, TIME_MS2I( 200 ), timerCallback, p );
	m->eSource.broadcastFlagsI( DataUpdated );
	chSysUnlockFromISR();
}

void cpuUsageMonitorIdleEnterHookCpp()
{
	if( CpuUsageMonitor::tryInstance() )
		CpuUsageMonitor::tryInstance()->idleEnter();
}

void cpuUsageMonitorIdleLeaveHookCpp()
{
	if( CpuUsageMonitor::tryInstance() )
		CpuUsageMonitor::tryInstance()->idleLeave();
}

extern "C"
{
	void cpuUsageMonitorIdleEnterHook()
	{
		cpuUsageMonitorIdleEnterHookCpp();
	}

	void cpuUsageMonitorIdleLeaveHook()
	{
		cpuUsageMonitorIdleLeaveHookCpp();
	}
}