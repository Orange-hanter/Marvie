#include "Src/usbcfg.h"

namespace UsbSduTest
{
	int test()
	{
        //while( true );
		//Initializes a serial-over-USB CDC driver
		sduObjectInit( &PORTAB_SDU1 );
		sduStart( &PORTAB_SDU1, &serusbcfg );

		// Activates the USB driver and then the USB bus pull-up on D+.
		// Note, a delay is inserted in order to not have to disconnect the cable
		// after a reset.
		usbDisconnectBus( serusbcfg.usbp );
		chThdSleepMilliseconds( 1500 );
		usbStart( serusbcfg.usbp, &usbcfg );
		usbConnectBus( serusbcfg.usbp );

		while( true )
		{
			reinterpret_cast< BaseSequentialStream* >( &PORTAB_SDU1 )->vmt->write( &PORTAB_SDU1, ( uint8_t* )"Hello\n", 6 );
			chThdSleepMilliseconds( 1000 );
		}

		return 0;
	}
} // namespace UsbSduTest