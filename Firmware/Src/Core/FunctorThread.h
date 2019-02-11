#pragma once

#include "BaseDynamicThread.h"
#include <utility>

template< typename Functor >
class FunctorThread : public BaseDynamicThread
{
public:
	inline FunctorThread( Functor&& functor, uint32_t stackSize, bool tryCCM = true, bool _autoDelete = false ) :
		BaseDynamicThread( stackSize, tryCCM ), functor( std::forward< Functor >( functor ) ), _autoDelete( _autoDelete ) {}

	bool autoDelete() { return _autoDelete; }
	void setAutoDelete( bool _autoDelete ) { this->_autoDelete = _autoDelete; }

private:
	void main() final override
	{
		_main< Functor >();
	}

	template< typename F > 
	std::enable_if_t< std::is_void< typename std::__invoke_result< F >::type >::value, void > _main()
	{
		functor();
		chSysLock();
		if( _autoDelete )
			deleteLater();
		exitS( 0 );
	}

	template< typename F > 
	std::enable_if_t< !std::is_void< typename std::__invoke_result< F >::type >::value, void > _main()
	{
		msg_t msg = ( msg_t )functor();
		chSysLock();
		if( _autoDelete )
			deleteLater();
		exitS( msg );
	}

private:
	Functor functor;
	volatile bool _autoDelete;
};

template< typename Functor >
FunctorThread< Functor >* createFunctorThread( Functor functor, uint32_t stackSize, bool tryCCM = true, bool _autoDelete = false )
{
	return new FunctorThread< Functor >( functor, stackSize, tryCCM, _autoDelete );
}