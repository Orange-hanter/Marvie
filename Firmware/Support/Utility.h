#pragma once

#include <stdint.h>
#include "Core/RingIterator.h"

namespace Utility
{
	float invSqrtf( float x );
	char* printInt( char* str, int v );
	char* printIntRightAlign( char* str, int value, int width = 1, char fillChar = '0' );

	template< typename R, typename T >
	R parseDouble( T p )
	{
		int s = 1;
		if( *p == '-' )
		{
			s = -1;
			++p;
		}

		R acc = 0.0;
		while( *p >= '0' && *p <= '9' )
			acc = acc * 10 + *p++ - '0';

		if( *p++ == '.' )
		{
			R k = 0.1;
			while( *p >= '0' && *p <= '9' )
			{
				acc += ( *p++ - '0' ) * k;
				k *= 0.1;
			}
		}

		return s * acc;
	}
	uint32_t parseInt( const uint8_t* p );
	uint32_t parseHex( const uint8_t* p );
	uint32_t parseTime( const uint8_t* p );
	float convertPressureToMeters( float p );

	template< typename T>
	T constrain( T value, T min, T max )
	{
		if( value < min )
			return min;
		else if( value > max )
			return max;
		return value;
	}

	void copy( uint8_t* dst, ByteRingIterator begin, uint32_t size );
}