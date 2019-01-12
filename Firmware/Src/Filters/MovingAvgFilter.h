#pragma once

#include "stdint.h"
#include "Abstract1DFilter.h"

class MovingAvgFilter : public Abstract1DFilter
{
public:
	MovingAvgFilter( uint32_t windowSize );
	~MovingAvgFilter();

	float update( float x ) override;
	void reset( float x ) override;

private:
	uint32_t windowSize;
	uint32_t end;
	float* m;
	float sum;
};