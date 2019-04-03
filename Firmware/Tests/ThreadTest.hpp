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
		Thread::sleep( TIME_MS2I( 5 ) );
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
		assert( mem0 == MemoryStatus::freeSpace( MemoryStatus::Region::All ) );

		thread = Thread::create( []( int a )
        {
			assert( a == 42 );
		}, 42 );
		assert( thread->start() == true );
		thread->wait();
		assert( thread->start() == true );
		thread->wait();
		thread->setStackSize( 2048 );
		assert( thread->start() == true );
		thread->wait();
		delete thread;
		assert( mem0 == MemoryStatus::freeSpace( MemoryStatus::Region::All ) );

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
			assert( future.isValid() == true );
			assert( future.get() == 314 );
			assert( future.isValid() == false );
			assert( std::abs( ( int )TIME_I2MS( chVTTimeElapsedSinceX( t0 ) ) - 10 ) <= 1 );
		}

		{
			systime_t t0 = chVTGetSystemTimeX();
			auto future = Concurrent::run( []( int a ) {
				assert( a == 314 );
				Thread::sleep( TIME_MS2I( 10 ) );
				return;
			}, 314 );
			assert( future.isValid() == true );
			future.get();
			assert( future.isValid() == false );
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
			assert( std::abs( ( int )TIME_I2MS( chVTTimeElapsedSinceX( t0 ) ) - 10 ) <= 1 );
		}

		{
			systime_t t0 = chVTGetSystemTimeX();
			auto future = Concurrent::run( []( NoncopyableType v ) {
				Thread::sleep( TIME_MS2I( 10 ) );
				assert( v.v == 42 );
				return;
			}, NoncopyableType( 42 ) );
			future.get();
			assert( std::abs( ( int )TIME_I2MS( chVTTimeElapsedSinceX( t0 ) ) - 10 ) <= 1 );
		}

		{
			systime_t t0 = chVTGetSystemTimeX();
			auto future = Concurrent::run( []( NoncopyableType v ) {
				Thread::sleep( TIME_MS2I( 10 ) );
				assert( v.v == 42 );
				return NoncopyableType( 314 );
			}, NoncopyableType( 42 ) );
			assert( future.get().v == 314 );
			assert( std::abs( ( int )TIME_I2MS( chVTTimeElapsedSinceX( t0 ) ) - 10 ) <= 1 );
		}

		{
			systime_t t0 = chVTGetSystemTimeX();
			auto future = Concurrent::run( []( NonmovableType v ) {
				Thread::sleep( TIME_MS2I( 10 ) );
				assert( v.v == 42 );
				return NonmovableType( 314 );
			}, NonmovableType( 42 ) );
			auto res = future.get();
			assert( res.v == 314 );
			assert( res.c == 2 )
			assert( std::abs( ( int )TIME_I2MS( chVTTimeElapsedSinceX( t0 ) ) - 10 ) <= 1 );
		}

		{
			systime_t t0 = chVTGetSystemTimeX();
			auto future = Concurrent::run( []() {
				Thread::sleep( TIME_MS2I( 10 ) );
				return CopymovableType( 314 );
			} );
			auto res = future.get();
			assert( res.v == 314 );
			assert( res.c == 0 );
			assert( res.m == 2 );
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
			assert( future.isValid() == true );
			assert( &future.get() == &gg );
			assert( future.isValid() == false );
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
