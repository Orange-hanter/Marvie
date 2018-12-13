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

uint32_t Date::_daysSinceEpoch( uint32_t day, uint32_t month, uint32_t year )
{
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
