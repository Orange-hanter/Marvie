#pragma once

#include <stdint.h>

template< typename Type >
class ReverseRingIterator
{
public:
	ReverseRingIterator();
	ReverseRingIterator( Type* begin, Type* current, uint32_t size );
	ReverseRingIterator( const ReverseRingIterator& );

	ReverseRingIterator& operator=( const ReverseRingIterator& );
	ReverseRingIterator& operator--();
	ReverseRingIterator operator--( int );
	ReverseRingIterator operator-( uint32_t ) const;
	void operator-=( uint32_t );
	ReverseRingIterator& operator++();
	ReverseRingIterator operator++( int );
	ReverseRingIterator operator+( uint32_t ) const;
	void operator+=( uint32_t );
	int operator-( const ReverseRingIterator& ) const;

	bool operator==( const ReverseRingIterator& ) const;
	bool operator!=( const ReverseRingIterator& ) const;

	Type& operator*() const;

	bool isValid() const;

private:
	Type* begin;
	Type* pos;
	uint32_t size;
};

typedef ReverseRingIterator< uint8_t > ReverseByteRingIterator;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

template< typename Type >
ReverseRingIterator<Type>::ReverseRingIterator()
{
	begin = nullptr;
	pos = nullptr;
	size = 0;
}

template< typename Type >
ReverseRingIterator<Type>::ReverseRingIterator( Type* begin, Type* current, uint32_t size )
{
	this->begin = begin;
	this->pos = current;
	this->size = size;
}

template< typename Type >
ReverseRingIterator<Type>::ReverseRingIterator( const ReverseRingIterator& i )
{
	begin = i.begin;
	pos = i.pos;
	size = i.size;
}

template< typename Type >
ReverseRingIterator<Type>& ReverseRingIterator<Type>::operator=( const ReverseRingIterator& i )
{
	begin = i.begin;
	pos = i.pos;
	size = i.size;
	return *this;
}

template< typename Type >
ReverseRingIterator<Type>& ReverseRingIterator<Type>::operator--()
{
	if( ++pos == begin + size + 1 )
		pos = begin;
	return *this;
}

template< typename Type >
ReverseRingIterator<Type> ReverseRingIterator<Type>::operator--( int )
{
	ReverseRingIterator tmp( *this );
	operator--();
	return tmp;
}

template< typename Type >
ReverseRingIterator<Type> ReverseRingIterator<Type>::operator-( uint32_t v ) const
{
	ReverseRingIterator i( *this );
	if( v > size )
		v %= size + 1;
	i.pos += v;
	if( i.pos >= begin + size + 1 )
		i.pos -= size + 1;
	return i;
}

template< typename Type >
void ReverseRingIterator<Type>::operator-=( uint32_t v )
{
	if( v > size )
		v %= size + 1;
	pos += v;
	if( pos >= begin + size + 1 )
		pos -= size + 1;
}

template< typename Type >
ReverseRingIterator<Type>& ReverseRingIterator<Type>::operator++()
{
	if( pos == begin )
		pos = begin + size;
	else
		--pos;
	return *this;
}

template< typename Type >
ReverseRingIterator<Type> ReverseRingIterator<Type>::operator++( int )
{
	ReverseRingIterator tmp( *this );
	operator++();
	return tmp;
}

template< typename Type >
ReverseRingIterator<Type> ReverseRingIterator<Type>::operator+( uint32_t v ) const
{
	ReverseRingIterator i( *this );
	if( v > size )
		v %= size + 1;
	i.pos -= v;
	if( i.pos < begin )
		i.pos += size + 1;
	return i;
}

template< typename Type >
void ReverseRingIterator<Type>::operator+=( uint32_t v )
{
	if( v > size )
		v %= size + 1;
	pos -= v;
	if( pos < begin )
		pos += size + 1;
}

template< typename Type >
int ReverseRingIterator<Type>::operator-( const ReverseRingIterator& i ) const
{
	int v = i.pos - pos;
	if( v >= 0 )
		return v;
	else
		return size + v + 1;
}

template< typename Type >
bool ReverseRingIterator<Type>::operator==( const ReverseRingIterator& i ) const
{
	return pos == i.pos;
}

template< typename Type >
bool ReverseRingIterator<Type>::operator!=( const ReverseRingIterator& i ) const
{
	return pos != i.pos;
}

template< typename Type >
Type& ReverseRingIterator<Type>::operator*() const
{
	return *pos;
}

template< typename Type >
bool ReverseRingIterator<Type>::isValid() const
{
	return begin != nullptr;
}