#include "MarvieDevice.h"

//uint8_t data[513] __attribute__( ( section( ".ram4" ) ) );

int main()
{
	halInit();
	chSysInit();

	ObjectMemoryUtilizer::instance()->runUtilizer( LOWPRIO );	
	MarvieDevice::instance()->exec();

	return 0;
}