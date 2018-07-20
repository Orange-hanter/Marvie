#include "DateTimeService.h"
#include "hal.h"

DateTime DateTimeService::currentDateTime()
{
	RTCDateTime dateTime;
	rtcGetTime( &RTCD1, &dateTime );
	return DateTime( Date( dateTime.day, dateTime.dayofweek, dateTime.month, dateTime.year + RTC_BASE_YEAR ),
					 Time( dateTime.millisecond % 1000,
						   ( dateTime.millisecond / 1000 ) % 60,
						   ( dateTime.millisecond / 1000 / 60 ) % 60,
						   ( dateTime.millisecond / 1000 / 3600 ) ) );
}

void DateTimeService::setDateTime( const DateTime& dateTime )
{
	RTCDateTime rtcDateTime;
	rtcDateTime.dstflag = false;
	if( dateTime.year() >= RTC_BASE_YEAR )
		rtcDateTime.year = dateTime.year() - RTC_BASE_YEAR;
	else
		rtcDateTime.year = 0;
	rtcDateTime.month = dateTime.month();
	rtcDateTime.day = dateTime.day();
	rtcDateTime.dayofweek = dateTime.dayOfWeek();
	rtcDateTime.millisecond = dateTime.msec() + dateTime.sec() * 1000 +
							  dateTime.min() * 60 * 1000 + dateTime.hour() * 3600 * 1000;
	rtcSetTime( &RTCD1, &rtcDateTime );
}