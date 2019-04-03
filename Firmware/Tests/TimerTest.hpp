#include "Core/Timer.h"
#include "Core/ThreadsQueue.h"
#include "Core/Assert.h"
#include "Core/MemoryStatus.h"

namespace TimerTest
{
	static ThreadsQueue tq;
	static int counter = 0;

	void f0()
	{
		++counter;
	}

	void f1( int a )
	{
		assert( a == 42 );
		tq.dequeueNext( MSG_OK );
	}

	int f3( int a, int b, int c )
	{
		assert( a == 42 && b == 314 && c == 667 );
		tq.dequeueNext( MSG_OK );
		return 0;
	}

	class NonmovableType
	{
	public:
		NonmovableType() : v( 0 ), c( 0 ) {}
		NonmovableType( int v ) : v( v ), c( 0 ) {}
		NonmovableType( const NonmovableType& other ) : v( other.v ), c( other.c + 1 ) {}
		//NonmovableType( NonmovableType&& other ) = delete;
		NonmovableType& operator=( const NonmovableType& other ) { v = other.v; c = other.c + 1; return *this; }
		//NonmovableType& operator=( NonmovableType&& other ) = delete;
		int v, c;	
	};

	class NoncopyableType
	{
	public:
		NoncopyableType() : v( 0 ), m( 0 ) {}
		NoncopyableType( int v ) : v( v ), m( 0 ) {}
		NoncopyableType( const NoncopyableType& other ) = delete;
		NoncopyableType( NoncopyableType&& other ) : NoncopyableType() { std::swap( v, other.v ); m = other.m + 1; }
		NoncopyableType& operator=( const NoncopyableType& other ) = delete;
		NoncopyableType& operator=( NoncopyableType&& other ) { std::swap( v, other.v ); m = other.m + 1; return *this; }
		int v, m;
	};

	class CopymovableType
	{
	public:
		CopymovableType() : v( 0 ), c( 0 ), m( 0 ) {}
		CopymovableType( int v ) : v( v ), c( 0 ), m( 0 ) {}
		CopymovableType( const CopymovableType& other ) : v( other.v ), c( other.c + 1 ), m( other.m ) {}
		CopymovableType( CopymovableType&& other ) : c( other.c ), m( other.m + 1 ) { std::swap( v, other.v ); }
		CopymovableType& operator=( const CopymovableType& other ) { v = other.v; c = other.c + 1, m = other.m; return *this; }
		CopymovableType& operator=( CopymovableType&& other ) { std::swap( v, other.v ); m = other.m + 1, c = other.c; return *this; }
		int v, c, m;
	};

	class A
	{
	public:
		A() { ++counter; }
		A( const A& other ) { ++counter; }
		~A() { --counter; }
	};

	int test()
	{
		auto mem0 = MemoryStatus::freeSpace( MemoryStatus::Region::All );
		{
			StaticTimer< 20 > timer;
			timer.setSingleShot( true );
			timer.callOnTimeout( f0 );
			timer.setInterval( TIME_MS2I( 1 ) );
			timer.start();
			Thread::sleep( TIME_MS2I( 10 ) );
			timer.stop();
			assert( counter == 1 );
			counter = 0;
			timer.setSingleShot( false );
			timer.start();
			Thread::sleep( TIME_MS2I( 10 ) );
			timer.stop();
			assert( std::abs( counter - 10 ) <= 1 );

			timer.setInterval( TIME_MS2I( 5 ) );
			timer.callOnTimeout( f1, 42 );
			timer.start();
			auto t0 = chVTGetSystemTimeX();
			tq.enqueueSelf( TIME_INFINITE );
			timer.stop();
			assert( std::abs( ( int )TIME_I2MS( chVTTimeElapsedSinceX( t0 ) ) - 5 ) <= 1 );
			
			timer.callOnTimeout( f3, 42, 314, 667 );
			timer.start();
			t0 = chVTGetSystemTimeX();
			tq.enqueueSelf( TIME_INFINITE );
			timer.stop();
			assert( std::abs( ( int )TIME_I2MS( chVTTimeElapsedSinceX( t0 ) ) - 5 ) <= 1 );

			int c = 160;
			timer.callOnTimeout( [&c]( int a )
			{
				assert( c == 160 );
				assert( a == 42 );
				tq.dequeueNext( MSG_OK );
			}, 42 );
			timer.start();
			t0 = chVTGetSystemTimeX();
			tq.enqueueSelf( TIME_INFINITE );
			timer.stop();
			assert( std::abs( ( int )TIME_I2MS( chVTTimeElapsedSinceX( t0 ) ) - 5 ) <= 1 );

			counter = 0;
			timer.setSingleShot( false );
			timer.setInterval( TIME_MS2I( 1 ) );
			timer.callOnTimeout( []( NoncopyableType a ) {
				assert( a.m == 2 );
				if( counter++ )
				{
					assert( a.v == 0 );
					tq.dequeueNext( MSG_OK );
				}
				else
					assert( a.v == 314 );
			}, NoncopyableType( 314 ) );
			timer.start();
			tq.enqueueSelf( TIME_INFINITE );
			timer.stop();

			counter = 0;
			timer.callOnTimeout( []( NonmovableType a ) {
				assert( a.c == 2 );
				assert( a.v == 314 );
				if( counter++ )
					tq.dequeueNext( MSG_OK );
			}, NonmovableType( 314 ) );
			timer.start();
			tq.enqueueSelf( TIME_INFINITE );
			timer.stop();

			counter = 0;
			timer.callOnTimeout( []( const NoncopyableType& a ) {
				assert( a.m == 1 );
				assert( a.v == 314 );
				if( counter++ )
					tq.dequeueNext( MSG_OK );
			}, NoncopyableType( 314 ) );
			timer.start();
			tq.enqueueSelf( TIME_INFINITE );
			timer.stop();
		}
		assert( mem0 == MemoryStatus::freeSpace( MemoryStatus::Region::All ) );
		
		counter = 0;
		{
			StaticTimer< 20 > timer;
			timer.setSingleShot( true );
			timer.setInterval( TIME_MS2I( 1 ) );
			timer.callOnTimeout( []( A a ) {
				tq.dequeueNext( MSG_OK );
			}, A() );
			assert( counter != 0 );
			timer.start();
			tq.enqueueSelf( TIME_INFINITE );
			timer.stop();

			timer.callOnTimeout( []( A a ) {
				tq.dequeueNext( MSG_OK );
			}, A() );
			timer.start();
			tq.enqueueSelf( TIME_INFINITE );
			timer.stop();
		}
		assert( counter == 0 );
		assert( mem0 == MemoryStatus::freeSpace( MemoryStatus::Region::All ) );

		{
			Timer* timer = new Timer( []( int a ) {
				assert( a == 42 );
				tq.dequeueNext( MSG_OK );
			}, 42 );
			timer->setInterval( TIME_MS2I( 5 ) );
			auto t0 = chVTGetSystemTimeX();
			timer->start();
			tq.enqueueSelf( TIME_INFINITE );
			assert( std::abs( ( int )TIME_I2MS( chVTTimeElapsedSinceX( t0 ) ) - 5 ) <= 1 );
			delete timer;

			timer = new Timer( []( A a, int b ) {
				assert( b == 42 );
				tq.dequeueNext( MSG_OK );
			}, A(), 42 );
			assert( counter != 0 );
			timer->setInterval( TIME_MS2I( 5 ) );
			timer->start();
			tq.enqueueSelf( TIME_INFINITE );

			timer->callOnTimeout( []() {
				tq.dequeueNext( MSG_OK );
			} );
			timer->start();
			tq.enqueueSelf( TIME_INFINITE );

			Timer timer2( std::move( *timer ) );
			delete timer;
			tq.enqueueSelf( TIME_INFINITE );
		}
		assert( counter == 0 );
		assert( mem0 == MemoryStatus::freeSpace( MemoryStatus::Region::All ) );

		{
			int gg = 271;
			auto t0 = chVTGetSystemTimeX();
			Timer::singleShot( TIME_MS2I( 5 ), [&gg]( A a, int b, int c )
			{
				assert( gg == 271 );
				assert( b = 42 );
				assert( c == 314 );
				tq.dequeueNext( MSG_OK );
			}, A(), 42, 314 );
			tq.enqueueSelf( TIME_INFINITE );
			assert( std::abs( ( int )TIME_I2MS( chVTTimeElapsedSinceX( t0 ) ) - 5 ) <= 1 );
		}
		assert( counter != 0 );
		assert( mem0 != MemoryStatus::freeSpace( MemoryStatus::Region::All ) );
		Thread::sleep( TIME_MS2I( 1 ) );
		assert( counter == 0 );
		assert( mem0 == MemoryStatus::freeSpace( MemoryStatus::Region::All ) );

		{
			BasicTimer timer;
			auto t0 = chVTGetSystemTimeX();
			timer.start( TIME_MS2I( 5 ), []( void* p )
			{
				assert( ( uint32_t )p == 42 );
				tq.dequeueNext( MSG_OK );
			}, ( void* )42 );
			tq.enqueueSelf( TIME_INFINITE );
			assert( std::abs( ( int )TIME_I2MS( chVTTimeElapsedSinceX( t0 ) ) - 5 ) <= 1 );
		}

		return 0;
	}
}