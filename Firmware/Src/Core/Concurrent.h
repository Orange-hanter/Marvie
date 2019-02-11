#pragma once

#include "FunctorThread.h"

namespace Concurrent
{
	template< typename Functor >
	FunctorThread< Functor >* run( Functor&& functor, uint32_t stackSize = 1024, tprio_t priority = NORMALPRIO, bool tryCCM = true, bool autoDelete = true )
	{
		FunctorThread< Functor >* thread = new FunctorThread< Functor >( std::forward< Functor >( functor ), stackSize, tryCCM, autoDelete );
		thread->start( priority );
		return thread;
	}
}