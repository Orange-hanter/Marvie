#include "UdpStressTestServer.h"
#include <string.h>

UdpStressTestServer::UdpStressTestServer( uint16_t localPort )
{
	udpSocket.bind( localPort );
}

void UdpStressTestServer::exec()
{
	struct Header { uint32_t counter; uint32_t lost; uint32_t err; uint32_t crc; };
	uint32_t lastNum, lostCounter, errCounter, counter;
	lastNum = lostCounter = errCounter = counter = 0;

	while( !chThdShouldTerminateX() )
	{
		if( !udpSocket.waitDatagram( TIME_MS2I( 100 ) ) )
			continue;

		IpAddress addr;
		uint16_t port;
		uint32_t datagramSize = udpSocket.readDatagram( tmp, 2048, &addr, &port );
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
		for( uint32_t i = 0; i < additionalSize / 4; ++i )
			tmp[i + sizeof( header )] = 0x55;
		header.crc = crc32Stm( crc32Stm( 0xFFFFFFFF, ( uint32_t* )&header, 3 ), ( uint32_t* )( tmp + sizeof( header ) ), additionalSize / 4 );
		memcpy( tmp, &header, sizeof( header ) );

		udpSocket.writeDatagram( tmp, sizeof( header ) + additionalSize, addr, port );
	}
}

uint32_t UdpStressTestServer::crc32Stm( uint32_t crc, uint32_t* data, uint32_t size )
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