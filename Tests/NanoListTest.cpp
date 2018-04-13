#include "Core/NanoList.h"
#include "Core/Assert.h"

int main()
{
	NanoList< int > list;
	NanoList< int >::Node a( 0 ), b( 1 ), c( 2 ), d( 3 );
	int h = 0;

	list.clear();
	assert( list.size() == 0 );
	assert( list.begin() == list.end() );

	list.clear();
	list.pushBack( &a );
	list.pushBack( &b );
	list.pushFront( &c );
	list.pushFront( &d );
	assert( list.size() == 4 );

	int ansA[4] = { 3, 2, 0, 1 };
	h = 0;
	for( auto i = list.begin(); i != list.end(); ++i )
		assert( ansA[h++] == *i );
	assert( h == 4 );

	assert( list.remove( ++list.begin() )->value == 2 );
	list.remove( --list.end() );
	int ansB[2] = { 3, 0 };
	h = 0;
	for( auto i = list.begin(); i != list.end(); ++i )
		assert( ansB[h++] == *i );
	assert( h == 2 );

	assert( list.popBack()->value == 0 );
	assert( list.size() == 1 );
	assert( *--list.end() == 3 );
	list.pushBack( &a );
	assert( list.popFront()->value == 3 );
	assert( list.popFront()->value == 0 );
	assert( list.size() == 0 );
	assert( list.begin() == list.end() );
	assert( list.popBack() == nullptr );
	assert( list.popFront() == nullptr );

	NanoList< int > list2;
	NanoList< int >::Node e( 4 ), f( 5 ), g( 6 );
	list2.pushBack( &e );
	list2.pushBack( &f );
	list2.pushBack( &g );
	list.pushBack( &a );
	list.pushBack( &b );
	list.pushBack( &c );
	list.pushBack( &d );
	assert( *list.insert( list.begin(), list2 ) == 4 );
	assert( list2.size() == 0 );
	assert( list2.begin() == list2.end() );
	int ansC[7] = { 4, 5, 6, 0, 1, 2, 3 };
	h = 0;
	for( auto i = list.begin(); i != list.end(); ++i )
		assert( ansC[h++] == *i );
	assert( h == 7 );

	while( true )
		;
}