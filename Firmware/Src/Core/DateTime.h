#pragma once

#include <stdint.h>

class Time
{
public:
	Time() : ms( 0 ), s( 0 ), m( 0 ), h( 0 ) {}
	Time( uint32_t hour, uint32_t min, uint32_t sec, uint32_t msec ) : ms( msec ), s( sec ), m( min ), h( hour ) {}

	inline void setTime( const Time& time ) { ms = time.ms; s = time.s; m = time.m; h = time.h; }
	inline void setTime( uint32_t hour, uint32_t min, uint32_t sec, uint32_t msec ) { ms = msec; s = sec; m = min; h = hour; }
	inline void setMsec( uint32_t msec ) { ms = msec; }
	inline void setSec( uint32_t sec ) { s = sec; }
	inline void setMin( uint32_t min ) { m = min; }
	inline void setHour( uint32_t hour ) { h = hour; }

	inline uint32_t msec() const { return ms; }
	inline uint32_t sec() const { return s; }
	inline uint32_t min() const { return m; }
	inline uint32_t hour() const { return h; }

	inline int32_t msecsSinceStartOfDay() const { return ms + s * 1000 + m * 60000 + h * 3600000; }
	inline int32_t msecsTo( const Time& time ) const { return time.msecsSinceStartOfDay() - msecsSinceStartOfDay(); }

	char* printTime( char* str, bool printMs = false );

	inline bool operator==( const Time& other ) const { return ms == other.ms && s == other.s && m == other.m && h == other.h; }
	inline bool operator!=( const Time& other ) const { return ms != other.ms || s != other.s || m != other.m || h != other.h; }

private:
	uint32_t ms : 10;
	uint32_t s : 6;
	uint32_t m : 6;
	uint32_t h : 5;
};

class Date
{
public:
	enum DayOfWeek
	{
		Monday = 1,
		Tuesday,
		Wednesday,
		Thursday,
		Friday,
		Saturday,
		Sunday
	};

	Date() : d( 0 ), dw( 0 ), m( 0 ), y( 0 ) {}
	Date( uint32_t year, uint32_t month, uint32_t day ) : d( day ), dw( _calcDayOfWeek( day, month, year ) ), m( month ), y( year ) {}
	Date( uint32_t year, uint32_t month, uint32_t day, uint32_t dayOfWeek ) : d( day ), dw( dayOfWeek ), m( month ), y( year ) {}
	Date( uint32_t year, uint32_t month, uint32_t day, DayOfWeek dayOfWeek ) : d( day ), dw( dayOfWeek ), m( month ), y( year ) {}

	inline void setDate( const Date& date ) { d = date.d; dw = date.dw; m = date.m; y = date.y; }
	inline void setDate( uint32_t year, uint32_t month, uint32_t day ) { d = day; m = month; y = year; dw = _calcDayOfWeek( day, month, year ); }
	inline void setDate( uint32_t year, uint32_t month, uint32_t day, DayOfWeek dayOfWeek ) { d = day; dw = dayOfWeek; m = month; y = year; }
	inline void setDate( uint32_t year, uint32_t month, uint32_t day, uint32_t dayOfWeek ) { d = day; dw = dayOfWeek; m = month; y = year; }
	inline void setDay( uint32_t day ) { d = day; }
	inline void setDayOfWeek( DayOfWeek dayOfWeek ) { dw = dayOfWeek; }
	inline void setMonth( uint32_t month ) { m = month; }
	inline void setYear( uint32_t year ) { y = year; }

	inline uint32_t day() const { return d; }
	inline DayOfWeek dayOfWeek() const { return ( DayOfWeek )dw; }
	inline uint32_t month() const { return m; }
	inline uint32_t year() const { return y; }

	inline int32_t daysSinceEpoch() const { return _daysSinceEpoch( day(), month(), year() ); }
	inline int32_t daysTo( const Date& date ) const { return _daysSinceEpoch( date.day(), date.month(), date.year() ) - _daysSinceEpoch( day(), month(), year() ); }

	char* printDate( char* str );

	inline bool operator==( const Date& other ) const { return d == other.d && m == other.m && y == other.y; }
	inline bool operator!=( const Date& other ) const { return d != other.d || m != other.m || y != other.y; }

private:
	static uint32_t _daysSinceEpoch( uint32_t day, uint32_t month, uint32_t year );
	static inline DayOfWeek _calcDayOfWeek( uint32_t day, uint32_t month, uint32_t year )
	{
		return ( DayOfWeek )( ( _daysSinceEpoch( day, month, year ) + 2 ) % 7 + 1 );
	}

private:
	uint32_t d : 5;
	uint32_t dw : 3;
	uint32_t m : 4;
	uint32_t y : 12;
};

class DateTime
{
public:
	DateTime() {}
	DateTime( const Date& data, const Time& time ) : d( data ), t( time ) {}

	inline void setDate( const Date& date ) { d.setDate( date ); }
	inline void setTime( const Time& time ) { t.setTime( time ); }

	inline Date& date() { return d; };
	inline Time& time() { return t; };
	inline Date date() const { return d; };
	inline Time time() const { return t; };

	inline int64_t msecsSinceEpoch() const { return ( int64_t )d.daysSinceEpoch() * 86400000 + t.msecsSinceStartOfDay(); }
	inline int64_t msecsTo( const DateTime& dateTime ) const { return dateTime.msecsSinceEpoch() - msecsSinceEpoch(); }

	inline DateTime& operator=( const DateTime& other ) { d = other.d; t = other.t; return *this; }

	char* printDateTime( char* str, char separator = ' ', bool printMs = false );

	inline bool operator==( const DateTime& other ) const { return d == other.d && t == other.t; }
	inline bool operator!=( const DateTime& other ) const { return d != other.d || t != other.t; }

private:
	Date d;
	Time t;
};