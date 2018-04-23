#include "Drivers/Network/SimGsm/SimGsm.h"
#include "Core/Assert.h"
#include <string.h>

static uint8_t ib[2048], ob[1024], tmp[2048];

static SimGsm* gsm;
static AbstractUdpSocket* udpSocket;

uint32_t crc32Stm( uint32_t crc, uint32_t* data, uint32_t size )
{
	static const uint32_t table[16] = {
		0x00000000, 0x04C11DB7, 0x09823B6E, 0x0D4326D9,
		0x130476DC, 0x17C56B6B, 0x1A864DB2, 0x1E475005,
		0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61,
		0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD };

	for( uint32_t w = 0; w < size; w++ )
	{
		crc ^= data[w];
		crc = ( crc << 4 ) ^ table[crc >> 28];
		crc = ( crc << 4 ) ^ table[crc >> 28];
		crc = ( crc << 4 ) ^ table[crc >> 28];
		crc = ( crc << 4 ) ^ table[crc >> 28];
		crc = ( crc << 4 ) ^ table[crc >> 28];
		crc = ( crc << 4 ) ^ table[crc >> 28];
		crc = ( crc << 4 ) ^ table[crc >> 28];
		crc = ( crc << 4 ) ^ table[crc >> 28];
	}

	return crc;
}

int main()
{
	halInit();
	chSysInit();

	Usart* usart = Usart::instance( &SD6 );
	usart->setOutputBuffer( ob, 1024 );
	usart->setInputBuffer( ib, 2048 );
	usart->open();
	palSetPadMode( GPIOC, 6, PAL_MODE_ALTERNATE( GPIO_AF_USART6 ) );
	palSetPadMode( GPIOC, 7, PAL_MODE_ALTERNATE( GPIO_AF_USART6 ) );

	gsm = new SimGsm( IOPA8 );
	gsm->setUsart( usart );
	gsm->setApn( "m2m30.velcom.by" );
	gsm->startModem( NORMALPRIO );
	gsm->waitForStatusChange();

	struct Header { uint32_t counter; uint32_t lost; uint32_t err; uint32_t crc; };
	uint32_t lastNum, lostCounter, errCounter, counter;
	static uint8_t additionalData[2032];

	for( int i = 0; i < 2032; ++i )
		additionalData[i] = 0x55;
	lastNum = lostCounter = errCounter = counter = 0;

	udpSocket = gsm->createUdpSocket( 2048, 0 );
	udpSocket->bind( 1112 );

	while( udpSocket->isOpen() )
	{
		if( !udpSocket->waitDatagram( TIME_S2I( 1 ) ) )
			continue;

		IpAddress addr;
		uint32_t datagramSize = udpSocket->readDatagram( tmp, 2048, &addr );
		if( datagramSize % 4 != 0 || datagramSize < 16 )
		{
			errCounter++;
			continue;
		}
		uint32_t additionalSize = datagramSize - 16;
		uint32_t* data = reinterpret_cast< uint32_t* >( tmp );

		uint32_t crc = crc32Stm( 0xFFFFFFFF, data, 3 );
		uint32_t expectedCrc = data[3];
		crc = crc32Stm( crc, data + 4, additionalSize / 4 );
		if( crc != expectedCrc )
		{
			errCounter++;
			continue;
		}

		uint32_t num = data[0];
		if( lastNum > num )
		{
			lostCounter = 0;
			errCounter = 0;
		}
		else
			lostCounter += num - lastNum - 1;
		lastNum = num;
		counter++;
		Header header = { counter, lostCounter, errCounter, 0 };
		header.crc = crc32Stm( crc32Stm( 0xFFFFFFFF, ( uint32_t* )&header, 3 ), ( uint32_t* )additionalData, additionalSize / 4 );

		memcpy( tmp, &header, sizeof( header ) );
		memcpy( tmp + sizeof( header ), additionalData, additionalSize );

		udpSocket->writeDatagram( tmp, sizeof( header ) + additionalSize, addr, 1113 );
	}

	chThdSleep( TIME_INFINITE );
}