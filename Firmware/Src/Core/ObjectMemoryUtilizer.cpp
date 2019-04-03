#include "ObjectMemoryUtilizer.h"

ObjectMemoryUtilizer* ObjectMemoryUtilizer::inst = nullptr;

ObjectMemoryUtilizer::ObjectMemoryUtilizer() : BaseDynamicThread( 256 )
{
	state = State::Stopped;
	chMBObjectInit( &mailbox, mboxBuffer, 8 );
}

ObjectMemoryUtilizer::~ObjectMemoryUtilizer()
{

}

ObjectMemoryUtilizer* ObjectMemoryUtilizer::instance()
{
	if( inst )
		return inst;
	inst = new ObjectMemoryUtilizer;
	return inst;
}

void ObjectMemoryUtilizer::runUtilizer( tprio_t prio )
{
	if( state == State::Stopped )
	{
		state = State::Running;
		start( prio );
	}
	else
		chThdSetPriority( prio );
}

void ObjectMemoryUtilizer::stopUtilizer()
{
	if( state == State::Stopped )
		return;
	chThdTerminate( this->thread_ref );
	chSysLock();
	chMBPostI( &mailbox, 0 );
	chSysUnlock();
	chThdWait( this->thread_ref );
}

void ObjectMemoryUtilizer::utilize( Object* p )
{
	syssts_t sysStatus = chSysGetStatusAndLockX();
	if( port_is_isr_context() )
	{
		if( chMBPostI( &mailbox, reinterpret_cast< msg_t >( p ) ) != MSG_OK )
			chSysHalt( "ObjectMemoryUtilizer" );
	}
	else
		chMBPostTimeoutS( &mailbox, reinterpret_cast< msg_t >( p ), TIME_INFINITE );
	chSysRestoreStatusX( sysStatus );
}

void ObjectMemoryUtilizer::main()
{
	while( !chThdShouldTerminateX() )
	{
		msg_t msg;
		chMBFetchTimeout( &mailbox, &msg, TIME_INFINITE );
		delete reinterpret_cast< Object* >( msg );
	}
	state = State::Stopped;
}