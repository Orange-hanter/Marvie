#include "Crc32SW.h"

#define CRC32_POLY   0x04C11DB7
#define CRC32_POLY_R 0xEDB88320

uint32_t Crc32SW::crc32r_table[256];
uint32_t Crc32SW::crc32_table[256];
bool Crc32SW::init = []()
{
	int i, j;
	uint32_t c, cr;
	for( i = 0; i < 256; ++i )
	{
		cr = i;
		c = i << 24;
		for( j = 8; j > 0; --j )
		{
			c = c & 0x80000000 ? ( c << 1 ) ^ CRC32_POLY : ( c << 1 );
			cr = cr & 0x00000001 ? ( cr >> 1 ) ^ CRC32_POLY_R : ( cr >> 1 );
		}
		crc32_table[i] = c;
		crc32r_table[i] = cr;
	}

	return true;
}();

uint32_t Crc32SW::crc( const uint8_t* data, uint32_t size, uint32_t crcInit )
{
	uint32_t v;
	uint32_t crc;
	crc = ~crcInit;
	while( size-- > 0 ) {
		v = *data++;
		crc = ( crc >> 8 ) ^ crc32r_table[( crc ^ ( v ) ) & 0xff];
	}
	return ~crc;
}