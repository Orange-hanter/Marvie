#pragma once

#include "BaseDynamicThread.h"
#include <type_traits>

template< typename Functor >
class FunctorThread : public BaseDynamicThread
{
public:
	inline FunctorThread( Functor functor, uint32_t stackSize, bool tryCCM = true, bool _autoDelete = false ) :
		BaseDynamicThread( stackSize, tryCCM ), functor( functor ), _autoDelete( _autoDelete ) {}

	bool autoDelete() { return _autoDelete; }
	void setAutoDelete( bool _autoDelete ) { this->_autoDelete = _autoDelete; }

private:
	void main() final override
	{
		_main( functor );
	}

	template< typename F > typename std::enable_if< std::is_void< typename std::__invoke_result< F >::type >::value, F >::type
		_main( F t )
	{
		functor();
		chSysLock();
		if( _autoDelete )
			deleteLater();
		exitS( 0 );
		return t;
	}

	template< typename F > typename std::enable_if< !std::is_void< typename std::__invoke_result< F >::type >::value, F >::type
		_main( F t )
	{
		msg_t msg = ( msg_t )functor();
		chSysLock();
		if( _autoDelete )
			deleteLater();
		exitS( msg );
		return t;
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