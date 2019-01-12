#pragma once

class Abstract1DFilter
{
public:
	virtual ~Abstract1DFilter() {}

	virtual float update( float ) = 0;
	virtual void reset( float ) = 0;
};