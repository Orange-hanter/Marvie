#pragma once

#include "FunctorThread.h"
#include "Future.h"

namespace Concurrent
{
	template< typename Functor >
	FunctorThread< Functor >* _run( Functor&& functor, uint32_t stackSize = 1024, tprio_t priority = NORMALPRIO, bool tryCCM = true, bool autoDelete = true )
	{
		FunctorThread< Functor >* thread = new FunctorThread< Functor >( std::forward< Functor >( functor ), stackSize, tryCCM, autoDelete );
		thread->start( priority );
		return thread;
	}

	template< typename Function, typename... Args, typename = std::enable_if_t< !std::is_same< ThreadProperties, std::decay_t< Function > >::value > >
	inline auto run( Function&& function, Args&&... args )
	{
		return _createFutureThread( defaultThreadProperties, std::forward< Function >( function ), std::forward< Args >( args )... );
	}

	template< typename Function, typename... Args >
	inline auto run( const ThreadProperties& props, Function&& function, Args&&... args )
	{
		return _createFutureThread( props, std::forward< Function >( function ), std::forward< Args >( args )... );
	}
}