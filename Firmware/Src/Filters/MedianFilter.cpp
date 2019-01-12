#include "MedianFilter.h"

MedianFilter::MedianFilter( uint32_t n )
{
	nodes = new Node[n];
	this->n = n;
	pos = 0;
	begin = nodes;
	for( uint32_t i = 0; i < n; i++ )
	{
		nodes[i].v = 0;
		nodes[i].prev = nodes + i - 1;
		nodes[i].next = nodes + i + 1;
	}
	nodes[0].prev = nodes + n - 1;
	nodes[n - 1].next = nodes;
}

MedianFilter::~MedianFilter()
{
	delete[] nodes;
}

float MedianFilter::update( float v )
{
	nodes[pos].v = v;
	nodes[pos].prev->next = nodes[pos].next;
	nodes[pos].next->prev = nodes[pos].prev;
	if( begin == nodes + pos )
		begin = begin->next;

	uint32_t i;
	Node* node = begin;
	for( i = 0; i < n - 1; i++ )
	{
		if( node->v < v )
			node = node->next;
		else
			break;
	}

	nodes[pos].prev = node->prev;
	nodes[pos].next = node;
	node->prev->next = nodes + pos;
	node->prev = nodes + pos;
	if( i == 0 )
		begin = node->prev;
	if( ++pos == n )
		pos = 0;
	
	Node* mid = begin;
	for( i = 0; i < ( n - 1 ) / 2; i++, mid = mid->next );
	return mid->v;
}

void MedianFilter::reset( float x )
{
	for( uint32_t i = 0; i < n; i++ )
		nodes[i].v = x;
}
