#include "Thread.h"
#include "Assert.h"
#include "NewDelete.h"

Thread::Thread()
{
	wa = nullptr;
	size = 1024;
	stackPolicy = MemoryAllocPolicy::TryCCM;
}

Thread::Thread( uint32_t stackSize, MemoryAllocPolicy stackPolicy /*= MemoryAllocPolicy::TryCCM */ )
{
	wa = nullptr;
	this->size = stackSize;
	this->stackPolicy = stackPolicy;
}

Thread::Thread( Thread&& other ) : Thread()
{
	swap( std::move( other ) );
}

Thread::~Thread()
{
	assert( threadRef == nullptr || threadRef->state == CH_STATE_FINAL );
	delete[] wa;
}

Thread& Thread::operator=( Thread&& other )
{
	swap( std::move( other ) );
	return *this;
}

void Thread::setStackSize( uint32_t size, MemoryAllocPolicy memPolicy /*= MemoryAllocPolicy::TryCCM */ )
{
	if( threadRef && threadRef->state != CH_STATE_FINAL )
		return;

	threadRef = nullptr;
	delete[] wa;
	wa = nullptr;
	this->size = size;
	this->stackPolicy = memPolicy;
}

uint32_t Thread::stackSize()
{
	return size;
}

bool Thread::start()
{
	if( threadRef && threadRef->state != CH_STATE_FINAL )
		return false;

	uint32_t waSize = THD_WORKING_AREA_SIZE( size );
	wa = new( stackPolicy ) uint8_t[waSize];
	if( wa == nullptr )
		return false;
	threadRef = chThdCreateStatic( wa, waSize, threadPrio, thdStart, this );
	return true;
}

void Thread::main()
{

}

void Thread::thdStart( void* p )
{
	reinterpret_cast< Thread* >( p )->main();
	chSysGetStatusAndLockX();
	exitS( 0 );
}

void Thread::swap( Thread&& other ) noexcept
{
	std::swap( wa, other.wa );
	std::swap( size, other.size );
	std::swap( stackPolicy, other.stackPolicy );
	AbstractThread::swap( std::move( other ) );
}