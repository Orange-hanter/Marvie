#pragma once

#include <stdint.h>

class Time
{
public:
	Time() : ms( 0 ), s( 0 ), m( 0 ), h( 0 ) {}
	Time( uint32_t msec, uint32_t sec, uint32_t min, uint32_t hour ) : ms( msec ), s( sec ), m( min ), h( hour ) {}

	inline void setTime( uint32_t msec, uint32_t sec, uint32_t min, uint32_t hour ) { ms = msec; s = sec; m = min; h = hour; }
	inline void setMsec( uint32_t msec ) { ms = msec; }
	inline void setSec( uint32_t sec ) { s = sec; }
	inline void setMin( uint32_t min ) { m = min; }
	inline void setHour( uint32_t hour ) { h = hour; }

	inline uint32_t msec() const { return ms; }
	inline uint32_t sec() const { return s; }
	inline uint32_t min() const { return m; }
	inline uint32_t hour() const { return h; }

private:
	uint32_t ms : 10;
	uint32_t s : 6;
	uint32_t m : 6;
	uint32_t h : 5;
};

class Date
{
public:
	Date() : d( 0 ), m( 0 ), y( 2018 ) {}
	Date( uint32_t day, uint32_t month, uint32_t year ) : d( day ), dw(0 ), m( month ), y( year ) {}
	Date( uint32_t day, uint32_t dayOfWeek, uint32_t month, uint32_t year ) : d( day ), dw( dayOfWeek ), m( month ), y( year ) {}

	inline void setData( uint32_t day, uint32_t month, uint32_t year ) { d = day; m = month; y = year; }
	inline void setData( uint32_t day, uint32_t dayOfWeek, uint32_t month, uint32_t year ) { d = day; dw = dayOfWeek; m = month; y = year; }
	inline void setDay( uint32_t day ) { d = day; }
	inline void setDayOfWeek( uint32_t dayOfWeek ) { dw = dayOfWeek; }
	inline void setMonth( uint32_t month ) { m = month; }
	inline void setYear( uint32_t year ) { y = year; }

	inline uint32_t day() const { return d; }
	inline uint32_t dayOfWeek() const { return dw; }
	inline uint32_t month() const { return m; }
	inline uint32_t year() const { return y; }

private:
	uint32_t d  : 5;
	uint32_t dw : 3;
	uint32_t m  : 4;
	uint32_t y  : 12;
};

class DateTime : public Time, public Date
{
public:
	DateTime() : Time(), Date() {}
	DateTime( Date data, Time time ) : Time( time ), Date( data ) {}
};