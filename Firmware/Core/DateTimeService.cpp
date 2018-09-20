#include "DateTimeService.h"
#include "hal.h"

DateTime DateTimeService::currentDateTime()
{
	RTCDateTime dateTime;
	rtcGetTime( &RTCD1, &dateTime );
	return DateTime( Date( dateTime.year + RTC_BASE_YEAR, dateTime.month, dateTime.day, ( Date::DayOfWeek )dateTime.dayofweek ),
					 Time( ( dateTime.millisecond / 1000 / 3600 ),
						   ( dateTime.millisecond / 1000 / 60 ) % 60,
						   ( dateTime.millisecond / 1000 ) % 60,
						   dateTime.millisecond % 1000 ) );
}

void DateTimeService::setDateTime( const DateTime& dateTime )
{
	RTCDateTime rtcDateTime;
	rtcDateTime.dstflag = false;
	if( dateTime.date().year() >= RTC_BASE_YEAR )
		rtcDateTime.year = dateTime.date().year() - RTC_BASE_YEAR;
	else
		rtcDateTime.year = 0;
	rtcDateTime.month = dateTime.date().month();
	rtcDateTime.day = dateTime.date().day();
	rtcDateTime.dayofweek = dateTime.date().dayOfWeek();
	rtcDateTime.millisecond = dateTime.time().msec() + dateTime.time().sec() * 1000 +
							  dateTime.time().min() * 60 * 1000 + dateTime.time().hour() * 3600 * 1000;
	rtcSetTime( &RTCD1, &rtcDateTime );
}