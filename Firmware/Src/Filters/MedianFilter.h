#pragma once

#include "Abstract1DFilter.h"
#include <stdint.h>

class MedianFilter : public Abstract1DFilter
{
public:
	MedianFilter( uint32_t n );
	~MedianFilter();

	float update( float x ) override;
	void reset( float x ) override;

private:
	struct Node
	{
		float v;
		Node* next;
		Node* prev;
	}* nodes;
	uint32_t n;
	uint32_t pos;
	Node* begin;
};
