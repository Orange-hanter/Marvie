#pragma once

#include "FunctorThread.h"

namespace Concurrent
{
	template< typename Functor >
	FunctorThread< Functor >* run( Functor functor, uint32_t stackSize, tprio_t priority, bool tryCCM = true, bool autoDelete = true )
	{
		FunctorThread< Functor >* thread = new FunctorThread< Functor >( functor, stackSize, tryCCM, autoDelete );
		thread->start( priority );
		return thread;
	}
}