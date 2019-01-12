#include "LowPassFilter.h"

LowPassFilter::LowPassFilter()
{
	y = 0.0f;
	alpha = 1.0f;
}

void LowPassFilter::setFreq( float dt, float frequency )
{
	float RC = 1.0f / ( 2.0f * 3.1415926f * frequency );
	alpha = dt / ( RC + dt );
}

void LowPassFilter::setAlpha( float alpha )
{
	this->alpha = alpha;
}

void LowPassFilter::reset( float value )
{
	y = value;
}

float LowPassFilter::update( float x )
{
	y += alpha * ( x - y );
	return y;
}