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

	inline uint32_t msec() { return ms; }
	inline uint32_t sec() { return s; }
	inline uint32_t min() { return m; }
	inline uint32_t hour() { return h; }

private:
	uint32_t ms : 10;
	uint32_t s : 6;
	uint32_t m : 6;
	uint32_t h : 5;
};

class Data
{
public:
	Data() : d( 0 ), m( 0 ), y( 2018 ) {}
	Data( uint32_t day, uint32_t month, uint32_t year ) : d( day ), m( month ), y( year ) {}

	inline void setData( uint32_t day, uint32_t month, uint32_t year ) { d = day; m = month; y = year; }
	inline void setDay( uint32_t day ) { d = day; }
	inline void setMonth( uint32_t month ) { m = month; }
	inline void setYear( uint32_t year ) { y = year; }

	inline uint32_t day() { return d; }
	inline uint32_t month() { return m; }
	inline uint32_t year() { return y; }

private:
	uint32_t d : 5;
	uint32_t m : 4;
	uint32_t y : 12;
};

class DataTime : public Time, public Data
{
public:
	DataTime() : Time(), Data() {}
	DataTime( Data data, Time time ) : Time( time ), Data( data ) {}
};