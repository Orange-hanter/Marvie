#include "ObjectMemoryUtilizer.h"
#include "CriticalSectionLocker.h"

ObjectMemoryUtilizer* ObjectMemoryUtilizer::inst = nullptr;

ObjectMemoryUtilizer::ObjectMemoryUtilizer() : Thread( 256 )
{
	state = State::Stopped;
	chMBObjectInit( &mailbox, mboxBuffer, sizeof( mboxBuffer ) / sizeof( msg_t ) );
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

bool ObjectMemoryUtilizer::runUtilizer( tprio_t prio )
{
	if( state != State::Stopped )
		return false;

	setPriority( prio );
	state = State::Running;
	if( !start() )
		state = State::Stopped;

	return state == State::Running;
}

void ObjectMemoryUtilizer::stopUtilizer()
{
	if( state == State::Stopped )
		return;
	requestInterruption();
	chMBPostTimeout( &mailbox, 0, TIME_INFINITE );
	wait();
}

void ObjectMemoryUtilizer::utilize( Object* p )
{
	CriticalSectionLocker locker;
	if( port_is_isr_context() )
	{
		if( chMBPostI( &mailbox, reinterpret_cast< msg_t >( p ) ) != MSG_OK )
			chSysHalt( "ObjectMemoryUtilizer" );
	}
	else
		chMBPostTimeoutS( &mailbox, reinterpret_cast< msg_t >( p ), TIME_INFINITE );
}

void ObjectMemoryUtilizer::main()
{
	while( !isInterruptionRequested() )
	{
		msg_t msg;
		chMBFetchTimeout( &mailbox, &msg, TIME_INFINITE );
		delete reinterpret_cast< Object* >( msg );
	}
	state = State::Stopped;
}