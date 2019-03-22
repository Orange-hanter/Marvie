#include "Core/Assert.h"
#include "Core/Concurrent.h"
#include "Network/PingService.h"
#include <stdlib.h>

namespace PingTest
{
	int test()
	{
		PingService* pingService = PingService::instance();
		pingService->setPongTimeout( 300 );
		pingService->startService();

		chThdSleepMilliseconds( 4000 );
		volatile bool flag = false;

		for( int i = 0; i < 8; ++i )
		{
			Concurrent::_run( [&flag, pingService, i]() {
				while( pingService->state() == PingService::State::Working )
				{
					PingService::TimeMeasurement tm;
					if( i % 2 )
					{
						systime_t t0 = chVTGetSystemTimeX();
						int nOk = pingService->ping( IpAddress( 192, 168, 10, 1 ), 4, &tm, 100 );
						int dt = ( int )TIME_I2MS( chVTGetSystemTimeX() - t0 );
						assert( ( nOk == 4 && !flag ) || flag );
						assert( abs( dt - ( nOk * ( tm.maxMs + 100 ) + ( 4 - nOk ) * 300 ) ) < 10 );
					}
					else
					{
						systime_t t0 = chVTGetSystemTimeX();
						int nOk = pingService->ping( IpAddress( 192, 168, 10, 42 ), 4, &tm, 100 );
						int dt = ( int )TIME_I2MS( chVTGetSystemTimeX() - t0 );
						assert( nOk == 0 );
						//assert( abs( dt - 4 * 300 ) < 110 * 4 );
					}
					//int nOk = pingService->ping( IpAddress( 192, 168, 10, 1 ), 4, &tm, 100 );
					//assert( nOk == 4 );
				}
			} );
		}

		chThdSleepMilliseconds( 15000 );
		flag = true;
		pingService->stopService();
		pingService->waitForStopped();

		thread_reference_t ref = nullptr;
		chSysLock();
		chThdSuspendS( &ref );

		return 0;
	}
} // namespace PingTest