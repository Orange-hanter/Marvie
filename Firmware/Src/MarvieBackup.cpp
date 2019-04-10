#include "MarvieBackup.h"
#include <string.h>

chibios_rt::_Mutex MarvieBackup::mutex;

bool MarvieBackup::Settings::isValid()
{
	Crc32HW::acquire();
	uint32_t currCrc = Crc32HW::crc32Uint( &crc + 1, sizeof( Settings ) / 4, 0xFFFFFFFF );
	Crc32HW::release();

	return crc == currCrc;
}

void MarvieBackup::Settings::setValid()
{
	Crc32HW::acquire();
	crc = Crc32HW::crc32Uint( &crc + 1, sizeof( Settings ) / 4, 0xFFFFFFFF );
	Crc32HW::release();
}

void MarvieBackup::Settings::reset()
{
	memset( this, 0, sizeof( Settings ) );

	flags.ethernetDhcp = false;
	flags.gsmEnabled = false;

	strcpy( passwords.adminPassword, "admin" );
	strcpy( passwords.observerPassword, "observer" );

	eth.ip = IpAddress( 192, 168, 10, 10 );
	eth.netmask = 0xFFFFFF00;
	eth.gateway = IpAddress( 192, 168, 10, 1 );

	gsm.pinCode = 1111;
	strcpy( gsm.apn, "vmi" );

	setValid();
}

void MarvieBackup::FailureDesc::reset()
{
	memset( this, 0, sizeof( FailureDesc ) );
}

void MarvieBackup::init()
{
	if( !isMarkerCorrect() )
		reset();
	else if( !settings.isValid() )
		settings.reset();
}

void MarvieBackup::reset()
{
	marker = correctMarker;
	version = 1;

	settings.reset();
	failureDesc.reset();
}

void MarvieBackup::acquire()
{
	mutex.lock();
}

void MarvieBackup::release()
{
	mutex.unlock();
}