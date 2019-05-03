#include "Core/Event.h"
#include "Core/Thread.h"

namespace EventTest
{
	int test()
	{
		Thread::getAndClearEvents( ALL_EVENTS );
		EventSource source;
		{
			EventListener listener;
			assert( listener.isListening() == false );
			source.registerMaskWithFlags( &listener, EVENT_MASK( 0 ), ( eventflags_t )3 );
			assert( listener.isListening() == true );
			assert( source.registeredListenerCount() == 1 );

			source.broadcastFlags( ( eventflags_t )1 );
			assert( Thread::getAndClearEvents( ALL_EVENTS ) == EVENT_MASK( 0 ) );
			assert( listener.getAndClearFlags() == ( eventflags_t )1 );

			source.broadcastFlags( ( eventflags_t )255 );
			assert( Thread::getAndClearEvents( ALL_EVENTS ) == EVENT_MASK( 0 ) );
			assert( listener.getAndClearFlags() == ( eventflags_t )3 );

			source.broadcastFlags( ( eventflags_t )4 );
			assert( Thread::getAndClearEvents( ALL_EVENTS ) == 0 );
			assert( listener.getAndClearFlags() == 0 );
		}
		assert( source.registeredListenerCount() == 0 );
		{
			EventListener listener[5];
			for( int i = 0; i < 5; ++i )
				EventSourceRef( &source ).registerMaskWithFlags( &listener[i], EVENT_MASK( i + 5 ), ( eventflags_t )1 << ( eventflags_t )( i + 1 ) );
			assert( source.registeredListenerCount() == 5 );
			for( int i = 0; i < 5; ++i )
			{
				source.broadcastFlags( ( eventflags_t )1 << ( eventflags_t )( i + 1 ) );
				assert( Thread::getAndClearEvents( ALL_EVENTS ) == EVENT_MASK( i + 5 ) );
				for( int i2 = 0; i2 < 5; ++i2 )
					assert( ( i == i2 && listener[i2].getAndClearFlags() == ( ( eventflags_t )1 << ( eventflags_t )( i + 1 ) ) ) ||
						( i != i2 && listener[i2].getAndClearFlags() == ( eventflags_t )0 ) );
			}
			source.unregister( &listener[3] );
			for( int i = 0; i < 5; ++i )
				assert( ( i == 3 && listener[3].isListening() == false ) || listener[i].isListening() == true );
			assert( source.registeredListenerCount() == 4 );
		}
		assert( source.registeredListenerCount() == 0 );
		{
			EventListener listener[5];
			for( int i = 0; i < 5; ++i )
				source.registerMaskWithFlags( &listener[i], EVENT_MASK( i + 5 ), 1 );
			source.unregisterAll();
			assert( source.registeredListenerCount() == 0 );
			for( int i = 0; i < 5; ++i )
				assert( listener[i].isListening() == false );
		}

		event_source_t nativeSource;
		chEvtObjectInit( &nativeSource );
		{
			EventSourceRef sourceRef( &nativeSource );
			EventListener listener;
			assert( listener.isListening() == false );
			sourceRef.registerMaskWithFlags( &listener, EVENT_MASK( 0 ), ( eventflags_t )3 );
			assert( listener.isListening() == true );
			assert( chEvtIsListeningI( &nativeSource ) == true );

			chEvtBroadcastFlags( &nativeSource, ( eventflags_t )1 );
			assert( Thread::getAndClearEvents( ALL_EVENTS ) == EVENT_MASK( 0 ) );
			assert( listener.getAndClearFlags() == ( eventflags_t )1 );

			chEvtBroadcastFlags( &nativeSource, ( eventflags_t )255 );
			assert( Thread::getAndClearEvents( ALL_EVENTS ) == EVENT_MASK( 0 ) );
			assert( listener.getAndClearFlags() == ( eventflags_t )3 );

			chEvtBroadcastFlags( &nativeSource, ( eventflags_t )4 );
			assert( Thread::getAndClearEvents( ALL_EVENTS ) == 0 );
			assert( listener.getAndClearFlags() == 0 );

			listener.unregister();
			assert( listener.isListening() == false );
			assert( chEvtIsListeningI( &nativeSource ) == false );

			source.registerMaskWithFlags( &listener, EVENT_MASK( 0 ), ( eventflags_t )3 );
			sourceRef.registerMaskWithFlags( &listener, EVENT_MASK( 0 ), ( eventflags_t )3 );
			assert( chEvtIsListeningI( &nativeSource ) == true );
			assert( source.registeredListenerCount() == 0 );

			sourceRef.registerMaskWithFlags( &listener, EVENT_MASK( 0 ), ( eventflags_t )4 );
			chEvtBroadcastFlags( &nativeSource, ( eventflags_t )3 );
			assert( Thread::getAndClearEvents( ALL_EVENTS ) == 0 );
			chEvtBroadcastFlags( &nativeSource, ( eventflags_t )4 );
			assert( Thread::getAndClearEvents( ALL_EVENTS ) == EVENT_MASK( 0 ) );
		}
		assert( chEvtIsListeningI( &nativeSource ) == false );

		{
			auto thread = Thread::createAndStart( []() {
				assert( Thread::waitAnyEvent( ALL_EVENTS ) == EVENT_MASK( 5 ) );
			} );
			EventListener listener( thread->threadReference() );
			EventSourceRef( &nativeSource ).registerMask( &listener, EVENT_MASK( 5 ) );
			chEvtBroadcastFlags( &nativeSource, ( eventflags_t )1 );
			thread->wait();
			delete thread;
		}

		return 0;
	}
} // namespace EventTest
