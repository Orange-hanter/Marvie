#pragma once

#include <stdint.h>

template< typename Type > class RingIterator;
template< typename Type > void ringToLinearArrays( const RingIterator< Type >&, uint32_t, Type**, uint32_t*, Type**, uint32_t* );

template< typename Type >
class RingIterator
{
	friend void ringToLinearArrays<>( const RingIterator&, uint32_t, Type**, uint32_t*, Type**, uint32_t* );

public:
	RingIterator();
	RingIterator( Type* begin, Type* current, uint32_t size );
	RingIterator( const RingIterator& );

	RingIterator& operator=( const RingIterator& );
	RingIterator& operator++();
	RingIterator operator++( int );
	RingIterator operator+( uint32_t ) const;
	void operator+=( uint32_t );
	RingIterator& operator--();
	RingIterator operator--( int );
	RingIterator operator-( uint32_t ) const;
	void operator-=( uint32_t );
	int operator-( const RingIterator& ) const;

	bool operator==( const RingIterator& ) const;
	bool operator!=( const RingIterator& ) const;

	Type& operator*() const;

	bool isValid() const;

private:
	Type* begin;
	Type* pos;
	uint32_t size;
};

typedef RingIterator< uint8_t > ByteRingIterator;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

template< typename Type >
RingIterator<Type>::RingIterator()
{
	begin = nullptr;
	pos = nullptr;
	size = 0;
}

template< typename Type >
RingIterator<Type>::RingIterator( Type* begin, Type* current, uint32_t size )
{
	this->begin = begin;
	this->pos = current;
	this->size = size;
}

template< typename Type >
RingIterator<Type>::RingIterator( const RingIterator& i )
{
	begin = i.begin;
	pos = i.pos;
	size = i.size;
}

template< typename Type >
RingIterator<Type>& RingIterator<Type>::operator=( const RingIterator& i )
{
	begin = i.begin;
	pos = i.pos;
	size = i.size;
	return *this;
}

template< typename Type >
RingIterator<Type>& RingIterator<Type>::operator++()
{
	if( ++pos == begin + size + 1 )
		pos = begin;
	return *this;
}

template< typename Type >
RingIterator<Type> RingIterator<Type>::operator++( int )
{
	RingIterator tmp( *this );
	operator ++();
	return tmp;
}

template< typename Type >
RingIterator<Type> RingIterator<Type>::operator+( uint32_t v ) const
{
	RingIterator i( *this );
	if( v > size )
		v %= size + 1;
	i.pos += v;
	if( i.pos >= begin + size + 1 )
		i.pos -= size + 1;
	return i;
}

template< typename Type >
void RingIterator<Type>::operator+=( uint32_t v )
{
	if( v > size )
		v %= size + 1;
	pos += v;
	if( pos >= begin + size + 1 )
		pos -= size + 1;
}

template< typename Type >
RingIterator<Type>& RingIterator<Type>::operator--()
{
	if( pos == begin )
		pos = begin + size;
	else
		--pos;
	return *this;
}

template< typename Type >
RingIterator<Type> RingIterator<Type>::operator--( int )
{
	RingIterator tmp( *this );
	operator --();
	return tmp;
}

template< typename Type >
RingIterator<Type> RingIterator<Type>::operator-( uint32_t v ) const
{
	RingIterator i( *this );
	if( v > size )
		v %= size + 1;
	i.pos -= v;
	if( i.pos < begin )
		i.pos += size + 1;
	return i;
}

template< typename Type >
void RingIterator<Type>::operator-=( uint32_t v )
{
	if( v > size )
		v %= size + 1;
	pos -= v;
	if( pos < begin )
		pos += size + 1;
}

template< typename Type >
int RingIterator<Type>::operator-( const RingIterator& i ) const
{
	int v = pos - i.pos;
	if( v >= 0 )
		return v;
	else
		return size + v + 1;
}

template< typename Type >
bool RingIterator<Type>::operator==( const RingIterator& i ) const
{
	return pos == i.pos;
}

template< typename Type >
bool RingIterator<Type>::operator!=( const RingIterator& i ) const
{
	return pos != i.pos;
}

template< typename Type >
Type& RingIterator<Type>::operator*() const
{
	return *pos;
}

template< typename Type >
bool RingIterator<Type>::isValid() const
{
	return begin != nullptr;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

template< typename Type >
void ringToLinearArrays( const RingIterator< Type >& begin, uint32_t size, Type** ppDataA, uint32_t* pSizeA, Type** ppDataB, uint32_t* pSizeB )
{
	uint32_t h = begin.begin + begin.size + 1 - begin.pos;
	if( size > h )
	{
		*ppDataA = begin.pos;
		*pSizeA = h;
		*ppDataB = begin.begin;
		*pSizeB = size - h;
	}
	else // size < h || size == h
	{
		*ppDataA = begin.pos;
		*pSizeA = size;
		*ppDataB = nullptr;
		*pSizeB = 0;
	}
}