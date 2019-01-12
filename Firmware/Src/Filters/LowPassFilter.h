#pragma once

#include "Abstract1DFilter.h"

class LowPassFilter : public Abstract1DFilter
{
public:
	LowPassFilter();

	void setFreq( float dt, float frequency );
	void setAlpha( float alpha );

	void reset( float value = 0.0f ) override;
	float update( float x ) override;

private:
	float alpha;
	float y;
};