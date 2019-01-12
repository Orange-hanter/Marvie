#include "MovingAvgFilter.h"

MovingAvgFilter::MovingAvgFilter( uint32_t windowSize )
{
	this->windowSize = windowSize;
	m = new float[windowSize];
	for( uint32_t i = 0; i < windowSize; ++i )
		m[i] = 0;
	sum = 0;
	end = 0;
}

MovingAvgFilter::~MovingAvgFilter()
{
	delete[] m;
}

float MovingAvgFilter::update( float x )
{
	sum += x;
	if( end == windowSize - 1 )
		end = 0;
	else
		++end;
	sum -= m[end];
	m[end] = x;

	return sum / windowSize;
}

void MovingAvgFilter::reset( float x )
{
	for( uint32_t i = 0; i < windowSize; ++i )
		m[i] = x;
	sum = x * windowSize;
}