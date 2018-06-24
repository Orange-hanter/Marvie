#include "Crc32HW.h"

uint32_t Crc32HW::crc32Uint( const uint32_t* pData, size_t count, uint32_t init )
{
	if( 0xFFFFFFFF == init )
		CRC->CR |= CRC_CR_RESET;

	while( count-- )
		CRC->DR = __RBIT( *pData++ );

	return ~__RBIT( CRC->DR );
}

uint32_t Crc32HW::crc32Byte( const uint8_t* pData, size_t count, uint32_t init )
{
	uint32_t crc;
	uint32_t *p32 = ( uint32_t* )pData;
	size_t count32 = count >> 2;
	if( 0xFFFFFFFF == init )
		CRC->CR |= CRC_CR_RESET;

	while( count32-- )
		CRC->DR = __RBIT( *p32++ );

	crc = __RBIT( CRC->DR );
	count = count % 4;
	if( count )
	{
		CRC->DR = __RBIT( crc );
		switch( count ) {
		case 1:
			CRC->DR = __RBIT( ( *p32 & 0x000000FF ) ^ crc ) >> 24;
			crc = ( crc >> 8 ) ^ __RBIT( CRC->DR );
			break;
		case 2:
			CRC->DR = ( __RBIT( ( *p32 & 0x0000FFFF ) ^ crc ) >> 16 );
			crc = ( crc >> 16 ) ^ __RBIT( CRC->DR );
			break;
		case 3:
			CRC->DR = __RBIT( ( *p32 & 0x00FFFFFF ) ^ crc ) >> 8;
			crc = ( crc >> 24 ) ^ __RBIT( CRC->DR );
			break;
		}
	}
	return ~crc;
}

uint32_t Crc32HW::crc32Byte( const uint8_t* pDataA, uint32_t sizeA, const uint8_t* pDataB, uint32_t sizeB )
{
	uint32_t crc;
	if( sizeA & 0x03 )
	{
		Crc32HW::crc32Uint( reinterpret_cast< const uint32_t* >( pDataA ), sizeA >> 2, -1 );
		uint8_t t[4];
		uint32_t a = sizeA & 0x03;
		uint32_t b = sizeA & ~0x03;
		uint32_t c = 4 - a;
		switch( a )
		{
		case 1:
			t[0] = pDataA[b];
			t[1] = pDataB[0];
			t[2] = pDataB[1];
			t[3] = pDataB[2];
			break;
		case 2:
			t[0] = pDataA[b];
			t[1] = pDataA[b + 1];
			t[2] = pDataB[0];
			t[3] = pDataB[1];
			break;
		case 3:
			t[0] = pDataA[b];
			t[1] = pDataA[b + 1];
			t[2] = pDataA[b + 2];
			t[3] = pDataB[0];
		}
		if( c <= sizeB )
		{
			Crc32HW::crc32Uint( reinterpret_cast< const uint32_t* >( t ), 1, 0 );
			crc = Crc32HW::crc32Byte( pDataB + c, sizeB - c, 0 );
		}
		else
			crc = Crc32HW::crc32Byte( t, a + sizeB, 0 );
	}
	else
	{
		Crc32HW::crc32Uint( reinterpret_cast< const uint32_t* >( pDataA ), sizeA >> 2, -1 );
		crc = Crc32HW::crc32Byte( pDataB, sizeB, 0 );
	}

	return crc;
}