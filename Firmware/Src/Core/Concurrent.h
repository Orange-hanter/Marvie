#pragma once

#include "Future.h"

namespace Concurrent
{
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