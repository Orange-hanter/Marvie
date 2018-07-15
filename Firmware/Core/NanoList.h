#pragma once

template< typename T >
class NanoList
{
public:
	class Node
	{
		friend class NanoList< T >;
		Node* next, *prev;

	public:
		Node();
		Node( T value );

		T value;
	};

	class Iterator
	{
		friend class NanoList< T >;
		Node* node;

		Iterator( Node* node );

	public:
		Iterator();

		inline Iterator& operator++();
		inline Iterator operator++( int );
		inline Iterator& operator--();
		inline Iterator operator--( int );
		inline Iterator operator+( int );
		inline Iterator operator-( int );
		inline bool operator ==( Iterator ) const;
		inline bool operator !=( Iterator ) const;
		inline T& operator*();
		inline operator Node*();
	};

	NanoList();
	~NanoList();

	void pushBack( Node* node );
	void pushFront( Node* node );
	Node* popBack();
	Node* popFront();
	Iterator insert( Iterator before, Node* node );
	Iterator insert( Iterator before, NanoList< T >& );
	Node* remove( Iterator pos );
	void clear();
	unsigned int size();
	bool isEmpty();

	Iterator begin();
	Iterator end();

private:
	Node enode;
	unsigned int n;
};

template< typename T >
NanoList< T >::NanoList()
{
	enode.next = &enode;
	enode.prev = &enode;
	n = 0;
}

template< typename T >
NanoList< T >::~NanoList()
{

}

template< typename T >
void NanoList< T >::pushBack( Node* node )
{
	insert( end(), node );
}

template< typename T >
void NanoList< T >::pushFront( Node* node )
{
	insert( begin(), node );
}

template< typename T >
typename NanoList< T >::Node* NanoList< T >::popBack()
{
	if( n )
		return remove( Iterator( enode.prev ) );
	return nullptr;
}

template< typename T >
typename NanoList< T >::Node* NanoList< T >::popFront()
{
	if( n )
		return remove( Iterator( enode.next ) );
	return nullptr;
}

template< typename T >
typename NanoList< T >::Iterator NanoList< T >::insert( Iterator before, Node* node )
{
	before.node->prev->next = node;
	node->prev = before.node->prev;
	node->next = before.node;
	before.node->prev = node;
	++n;
	return Iterator( node );
}

template< typename T >
typename NanoList< T >::Iterator NanoList< T >::insert( Iterator before, NanoList< T >& list )
{
	if( list.n == 0 )
		return before;

	Node* tmp = before.node->prev->next = list.enode.next;
	list.enode.next->prev = before.node->prev;
	list.enode.prev->next = before.node;
	before.node->prev = list.enode.prev;
	n += list.n;
	list.clear();
	return Iterator( tmp );
}

template< typename T >
typename NanoList< T >::Node* NanoList< T >::remove( Iterator pos )
{
	pos.node->prev->next = pos.node->next;
	pos.node->next->prev = pos.node->prev;
	--n;
	return pos.node;
}

template< typename T >
void NanoList< T >::clear()
{
	enode.next = &enode;
	enode.prev = &enode;
	n = 0;
}

template< typename T >
unsigned int NanoList< T >::size()
{
	return n;
}

template< typename T >
bool NanoList<T>::isEmpty()
{
	return n == 0;
}

template< typename T >
typename NanoList< T >::Iterator NanoList< T >::begin()
{
	return Iterator( enode.next );
}

template< typename T >
typename NanoList< T >::Iterator NanoList< T >::end()
{
	return Iterator( &enode );
}

template< typename T >
NanoList< T >::Node::Node() : next( nullptr ), prev( nullptr ) {}

template< typename T >
NanoList< T >::Node::Node( T value ) : next( nullptr ), prev( nullptr ), value( value ) {}

template< typename T >
NanoList< T >::Iterator::Iterator( Node* node ) : node( node ) {}

template< typename T >
NanoList< T >::Iterator::Iterator() : node( nullptr ) {}

template< typename T >
typename NanoList< T >::Iterator& NanoList< T >::Iterator::operator++()
{
	node = node->next;
	return *this;
}

template< typename T >
typename NanoList< T >::Iterator NanoList< T >::Iterator::operator++( int )
{
	Iterator tmp( *this );
	operator ++();
	return tmp;
}

template< typename T >
typename NanoList< T >::Iterator& NanoList< T >::Iterator::operator--()
{
	node = node->prev;
	return *this;
}

template< typename T >
typename NanoList< T >::Iterator NanoList< T >::Iterator::operator--( int )
{
	Iterator tmp( *this );
	operator --();
	return tmp;
}

template< typename T >
typename NanoList< T >::Iterator NanoList< T >::Iterator::operator-( int v )
{
	Iterator tmp( *this );
	if( v > 0 )
	{
		while( v-- )
			--tmp;
	}
	else
	{
		while( v++ != 0 )
			++tmp;
	}

	return tmp;
}

template< typename T >
typename NanoList< T >::Iterator NanoList< T >::Iterator::operator+( int v )
{
	Iterator tmp( *this );
	if( v > 0 )
	{
		while( v-- )
			++tmp;
	}
	else
	{
		while( v++ != 0 )
			--tmp;
	}

	return tmp;
}

template< typename T >
bool NanoList< T >::Iterator::operator==( Iterator i ) const
{
	return node == i.node;
}

template< typename T >
bool NanoList< T >::Iterator::operator!=( Iterator i ) const
{
	return node != i.node;
}

template< typename T >
T& NanoList< T >::Iterator::operator*()
{
	return node->value;
}

template< typename T >
NanoList< T >::Iterator::operator Node*()
{
	return node;
}