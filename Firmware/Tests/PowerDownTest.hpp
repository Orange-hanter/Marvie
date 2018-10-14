#pragma once

#include "hal.h"
#include "Core/DateTimeService.h"

namespace PowerDownTest
{
	int test()
	{
		volatile uint32_t* reg = &RTC->BKP18R;

		palSetPadMode( GPIOC, 13, PAL_MODE_INPUT );

		while( palReadPad( GPIOC, 13 ) )
			;

		rtcnt_t t0 = chSysGetRealtimeCounterX();
		int i = 0;
		while( true )
			reg[i++ % 2] = chSysGetRealtimeCounterX() - t0;

		/*Time t0 = DateTimeService::currentDateTime().time();
		int i = 0;
		while( true )
			reg[i++ % 2] = t0.msecsTo( DateTimeService::currentDateTime().time() );*/

		return 0;
	}
}