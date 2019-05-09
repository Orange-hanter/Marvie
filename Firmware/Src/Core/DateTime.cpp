#include "DateTime.h"
#include "Support/Utility.h"

char* Time::printTime( char* _str, bool printMs )
{
	char* str = Utility::printIntRightAlign( _str, h, 2 );
	*str++ = ':';
	str = Utility::printIntRightAlign( str, m, 2 );
	*str++ = ':';
	str = Utility::printIntRightAlign( str, s, 2 );
	if( printMs )
	{
		*str++ = ':';
		str = Utility::printIntRightAlign( str, ms, 3 );
	}

	return str;
}

char* Date::printDate( char* _str )
{
	char* str = Utility::printIntRightAlign( _str, d, 2 );
	*str++ = '.';
	str = Utility::printIntRightAlign( str, m, 2 );
	*str++ = '.';
	str = Utility::printIntRightAlign( str, y, 4 );

	return str;
}

Date Date::fromDaysSinceEpoch( int32_t days )
{
	uint32_t y = uint32_t( ( 10000 * ( uint64_t )days + 14780 ) / 3652425 );
	int32_t ddd = days - ( 365 * y + y / 4 - y / 100 + y / 400 );
	if( ddd < 0 )
	{
		--y;
		ddd = days - ( 365 * y + y / 4 - y / 100 + y / 400 );
	}
	const uint32_t mi = ( 100 * ddd + 52 ) / 3060;
	const uint32_t mm = ( mi + 2 ) % 12 + 1;
	y = y + ( mi + 2 ) / 12;
	const uint32_t dd = ddd - ( mi * 306 + 5 ) / 10 + 1;

	return Date( y, mm, dd, ( DayOfWeek )( ( days + 2 ) % 7 + 1 ) );
}

uint32_t Date::_daysSinceEpoch( uint32_t day, uint32_t month, uint32_t year )
{
	//https://stackoverflow.com/a/12863278

	month = ( month + 9 ) % 12;
	year -= month / 10;

	return 365 * year + year / 4 - year / 100 + year / 400 + ( month * 306 + 5 ) / 10 + ( day - 1 );
}

char* DateTime::printDateTime( char* str, char separator /*= ' '*/, bool printMs /*= false */ )
{
	char* s = d.printDate( str );
	*s++ = separator;

	return t.printTime( s, printMs );
}
