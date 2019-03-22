#include "Core/Assert.h"
#include "Core/Thread.h"
#include "Core/Future.h"
#include "Core/Concurrent.h"
#include "Core/MemoryStatus.h"

namespace ThreadTest
{
	class NonmovableType
	{
	public:
		NonmovableType() : v( 0 ) {}
		NonmovableType( int v ) : v( v ) {}
		NonmovableType( const NonmovableType& other ) : v( other.v ) {}
		NonmovableType( NonmovableType&& other ) = delete;
		NonmovableType& operator=( const NonmovableType& other ) { v = other.v; return *this; }
		NonmovableType& operator=( NonmovableType&& other ) = delete;
		int v;	
	};

	class NoncopyableType
	{
	public:
		NoncopyableType() : v( 0 ) {}
		NoncopyableType( int v ) : v( v ) {}
		NoncopyableType( const NoncopyableType& other ) = delete;
		NoncopyableType( NoncopyableType&& other ) : NoncopyableType() { std::swap( v, other.v ); }
		NoncopyableType& operator=( const NoncopyableType& other ) = delete;
		NoncopyableType& operator=( NoncopyableType&& other ) { std::swap( v, other.v ); return *this; }
		int v;
	};

	class CopymovableType
	{
	public:
		CopymovableType() : c( 0 ), m( 0 ) {}
		CopymovableType( const CopymovableType& other ) : c( other.c + 1 ), m( other.m ) {}
		CopymovableType( CopymovableType&& other ) : c( other.c ), m( other.m + 1 ) {}
		CopymovableType& operator=( const CopymovableType& other ) { c = other.c + 1, m = other.m; return *this; }
		CopymovableType& operator=( CopymovableType&& other ) { m = other.m + 1, c = other.c; return *this; }
		int c, m;
	};
	
	int test()
	{
		size_t mem0 = MemoryStatus::freeSpace( MemoryStatus::Region::All );

		Thread* thread = Thread::createAndStart( []( int a, int b )
        {
			assert( a == 42 );
			assert( b == 73 );
			Thread::sleep( TIME_MS2I( 10 ) );
		}, 42, 73 );
		assert( thread->start() == false );
		Thread::sleep( TIME_MS2I( 2000 ) );
		thread->wait();
		delete thread;

		thread = Thread::create( []( int a, float b, int c )
		{
			assert( a == 42 );
			assert( b == 73 );
			assert( c == 101 );
			Thread::sleep( TIME_MS2I( 10 ) );
		}, 42, 73, 101 );
        thread->setStackSize( 2048 );
		assert( thread->start() == true );
		thread->wait();
		delete thread;

		constexpr ThreadProperties threadProps( 2048, NORMALPRIO );
		thread = Thread::create( threadProps, []( int a, float b, int c )
		{
			assert( a == 42 );
			assert( b == 73 );
			assert( c == 101 );
			Thread::sleep( TIME_MS2I( 10 ) );
		}, 42, 73, 101 );
		assert( thread->start() == true );
		thread->wait();
		delete thread;

		{
			Thread threadObj;
			threadObj.start();
			Thread tmp = std::move( threadObj );
			tmp.setStackSize( 2048 );
			assert( tmp.stackSize() == 1024 );
			tmp.wait();
			tmp.setStackSize( 2048 );
			assert( tmp.stackSize() == 2048 );
			assert( tmp.start() == true );
			tmp.wait();
		}

		{
			systime_t t0 = chVTGetSystemTimeX();
			auto future = Concurrent::run( []( int a ) {
				Thread::sleep( TIME_MS2I( 10 ) );
				return a;
			}, 314 );
			assert( future.get() == 314 );
			assert( std::abs( ( int )TIME_I2MS( chVTTimeElapsedSinceX( t0 ) ) - 10 ) <= 1 );
		}

		{
			systime_t t0 = chVTGetSystemTimeX();
			auto future = Concurrent::run( []( int a ) {
				assert( a == 314 );
				Thread::sleep( TIME_MS2I( 10 ) );
				return;
			}, 314 );
			future.get();
			assert( std::abs( ( int )TIME_I2MS( chVTTimeElapsedSinceX( t0 ) ) - 10 ) <= 1 );
		}

		{
			systime_t t0 = chVTGetSystemTimeX();
			Future< void > future = Concurrent::run( []() {
				Thread::sleep( TIME_MS2I( 10 ) );
				return 314;
			} );
			future.get();
			assert( std::abs( ( int )TIME_I2MS( chVTTimeElapsedSinceX( t0 ) ) - 10 ) <= 1 );
		}

		{
			systime_t t0 = chVTGetSystemTimeX();
			Future< void > future = Concurrent::run( []() {
				Thread::sleep( TIME_MS2I( 10 ) );
				return 314;
			} );
			assert( future.wait( TIME_MS2I( 5 ) ) == false );
			assert( future.wait( TIME_MS2I( 6 ) ) == true );
			future.get();
			assert( std::abs( ( int )TIME_I2MS( chVTTimeElapsedSinceX( t0 ) ) - 10 ) <= 1 );
		}

		{
			systime_t t0 = chVTGetSystemTimeX();
			auto future = Concurrent::run( []() {
				Thread::sleep( TIME_MS2I( 10 ) );
				return NoncopyableType( 314 );
			} );
			assert( future.get().v == 314 );
			assert( future.get().v == 0 );
			assert( std::abs( ( int )TIME_I2MS( chVTTimeElapsedSinceX( t0 ) ) - 10 ) <= 1 );
		}

		{
			systime_t t0 = chVTGetSystemTimeX();
			auto future = Concurrent::run( []() {
				Thread::sleep( TIME_MS2I( 10 ) );
				return CopymovableType();
			} );
			assert( future.get().c == 1 );
			assert( future.get().m == 1 );
			assert( future.get().c == 1 );
			assert( std::abs( ( int )TIME_I2MS( chVTTimeElapsedSinceX( t0 ) ) - 10 ) <= 1 );
		}

		{
			systime_t t0 = chVTGetSystemTimeX();
			auto future = Concurrent::run( []() {
				Thread::sleep( TIME_MS2I( 10 ) );
				return;
			} );
			future.get();
			assert( std::abs( ( int )TIME_I2MS( chVTTimeElapsedSinceX( t0 ) ) - 10 ) <= 1 );
		}

		{
			int gg = 42;
			systime_t t0 = chVTGetSystemTimeX();
			auto future = Concurrent::run( [&gg]() -> int& {
				Thread::sleep( TIME_MS2I( 10 ) );
				return gg;
			} );
			assert( &future.get() == &gg );
			assert( &future.get() == &gg );
			assert( std::abs( ( int )TIME_I2MS( chVTTimeElapsedSinceX( t0 ) ) - 10 ) <= 1 );
		}

		{
			int gg = 42;
			systime_t t0 = chVTGetSystemTimeX();
			auto future = Concurrent::run( [&gg]( int a ) -> int& {
				assert( a == 314 );
				Thread::sleep( TIME_MS2I( 10 ) );
				return gg;
			}, 314 );
			assert( &future.get() == &gg );
			assert( &future.get() == &gg );
			assert( std::abs( ( int )TIME_I2MS( chVTTimeElapsedSinceX( t0 ) ) - 10 ) <= 1 );
		}
		Thread::sleep( TIME_MS2I( 1 ) );
		assert( mem0 == MemoryStatus::freeSpace( MemoryStatus::Region::All ) );

		{
			EventListener listener;
			Concurrent::run( []() {
				Thread::sleep( TIME_MS2I( 10 ) );
				return;
			} ).eventSource().registerOne( &listener, 0 );
			systime_t t0 = chVTGetSystemTimeX();
			Thread::waitAnyEvent( EVENT_MASK( 0 ) );
			assert( std::abs( ( int )TIME_I2MS( chVTTimeElapsedSinceX( t0 ) ) - 10 ) <= 1 );
			assert( listener.isListening() == true );
			Thread::sleep( TIME_MS2I( 1 ) );
			assert( listener.isListening() == false );
		}

		for( int i = 0; i < 10; ++i )
		{
			Concurrent::run( []() {
				Thread::sleep( TIME_MS2I( 10 ) );
				return;
			} );
		}
		Thread::sleep( TIME_MS2I( 5 ) );
		assert( mem0 != MemoryStatus::freeSpace( MemoryStatus::Region::All ) );
		Thread::sleep( TIME_MS2I( 6 ) );
		assert( mem0 == MemoryStatus::freeSpace( MemoryStatus::Region::All ) );

		return 0;
	}
} // namespace ThreadTest
