#pragma once

#include "DateTime.h"

class DateTimeService
{
public:
	static DateTime currentDateTime();
	static void setDateTime( const DateTime& dateTime );
};