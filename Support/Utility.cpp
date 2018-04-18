#include "Utility.h"
#include <math.h>
#include "string.h"

namespace Utility
{
	// Fast inverse square-root
	// See: http://en.wikipedia.org/wiki/Fast_inverse_square_root
	//		http://rrrola.wz.cz/inv_sqrt.html
	float invSqrtf(float x)
	{
		union { float f; unsigned int u; } y = { x };
		y.u = 0x5F1FFFF9ul - ( y.u >> 1 );
		return 0.703952253f * y.f * ( 2.38924456f - x * y.f * y.f );
	}

	char* printInt( char* str, int v )
	{
		char t[10];
		int n = 0;
		if( v < 0 )
			*str++ = '-', v = -v;
		while( v >= 10 )
		{
			int nv = v / 10;
			t[n++] = '0' + v - nv * 10;
			v = nv;
		}
		*str++ = '0' + v;
		while( n )
			*str++ = t[--n];
		return str;
	}

	char* printIntRightAlign( char* str, int value, int width /*= 1*/, char fillChar /*= '0' */ )
	{
		if( width < 1 )
			return str;
		char* end = str + width;
		bool ngv = false;
		if( value < 0 )
			ngv = true, value = -value;
		while( value >= 10 && width )
		{
			int nv = value / 10;
			str[--width] = '0' + value - nv * 10;
			value = nv;
		}
		if( width )
			str[--width] = '0' + value;
		if( ngv && width )
			str[--width] = '-';
		while( width )
			str[--width] = fillChar;
		return end;
	}

	uint32_t parseInt( const uint8_t* p )
	{
		uint32_t acc = 0;
		while( *p >= '0' && *p <= '9' )
			acc = acc * 10 + *p++ - '0';

		return acc;
	}

	uint32_t parseHex(const uint8_t* p)
	{
		uint32_t acc = 0;
		while( *p )
		{
			char byte = *p++;
			if( byte >= '0' && byte <= '9' ) byte -= '0';
			else if( byte >= 'a' && byte <= 'f' ) byte += 10 - 'a';
			else if( byte >= 'A' && byte <= 'F' ) byte += 10 - 'A';
			else break;
			acc = ( acc << 4 ) | ( byte & 0xF );
		}

		return acc;
	}

	// hhmmss to seconds
	uint32_t parseTime(const uint8_t* p)
	{
		uint32_t time = 0;
		if( *p >= '0' && *p <= '9' )
			time += ( *p++ - '0' ) * 3600; else return 0;
		if( *p >= '0' && *p <= '9' )
			time += ( *p++ - '0' ) * 3600; else return 0;
		if( *p >= '0' && *p <= '9' )
			time += ( *p++ - '0' ) * 60;   else return 0;
		if( *p >= '0' && *p <= '9' )
			time += ( *p++ - '0' ) * 60;   else return 0;
		if( *p >= '0' && *p <= '9' )
			time += ( *p++ - '0' );        else return 0;
		if( *p >= '0' && *p <= '9' )
			time += ( *p - '0' );          else return 0;
		return time;
	}

	float convertPressureToMeters( float p )
	{
		return ( 1.0f - powf( p / 101325.0f, 1.0f / 5.255f ) ) * 44330.0f;
	}

	void copy( uint8_t* dst, ByteRingIterator begin, uint32_t size )
	{
		uint8_t* data[2];
		uint32_t dataSize[2];
		ringToLinearArrays( begin, size, data, dataSize, data + 1, dataSize + 1 );
		memcpy( dst, data[0], dataSize[0] );
		if( data[1] )
			memcpy( dst + dataSize[0], data[1], dataSize[1] );
	}
}