#include "ch.h"
#include "hal.h"
#include "os/various/cpp_wrappers/ch.hpp"
#include "Assert.h"
#include "Core/RingBuffer.h"

using namespace chibios_rt;

class RingBufferTest : private BaseStaticThread< 256 >
{
public:
	void exec()
	{
		buffer = new DynamicRingBuffer< uint8_t >( 5 );

		uint8_t data[10] = { 0, 1, 2, 3 };
		assert( buffer->write( data, 2 ) == 2 );
		assert( buffer->write( data, 2 ) == 2 );
		assert( buffer->write( data, 2 ) == 1 );
		assert( buffer->writeAvailable() == 0 );
		assert( buffer->readAvailable() == 5 );
		assert( buffer->isOverflowed() == true );
		buffer->clear();
		assert( buffer->writeAvailable() == 5 );
		assert( buffer->readAvailable() == 0 );
		for( int i = 0; i < 3; ++i )
		{
			assert( buffer->write( data, 4 ) == 4 );
			uint8_t tmp[4];
			assert( buffer->read( tmp, 4 ) == 4 );
			for( int i2 = 0; i2 < 4; ++i2 )
				assert( tmp[i2] == data[i2] );
			for( int i2 = 0; i2 < 4; ++i2 )
				data[i2] += 4;
		}

		systime_t startTime = chVTGetSystemTimeX();
		assert( buffer->read( data, 1, TIME_MS2I( 10 ) ) == 0 );
		assert( TIME_I2MS( chVTTimeElapsedSinceX( startTime ) ) == 10 );
		
		seq = Sequence::S1;
		start( NORMALPRIO + 1 );
		startTime = chVTGetSystemTimeX();		
		assert( buffer->read( data, 10, TIME_MS2I( 10 ) ) == 3 );
		assert( TIME_I2MS( chVTTimeElapsedSinceX( startTime ) ) == 10 );
		wait();

		seq = Sequence::S2;
		start( NORMALPRIO + 1 );
		startTime = chVTGetSystemTimeX();
		assert( buffer->read( data, 10, TIME_INFINITE ) == 10 );
		assert( TIME_I2MS( chVTTimeElapsedSinceX( startTime ) ) == 10 * 3 );
		wait();

		seq = Sequence::S3;
		start( NORMALPRIO + 1 );
		assert( buffer->read( data, 10, TIME_INFINITE ) == 10 );		
		assert( buffer->isOverflowed() == false );
		wait();

		seq = Sequence::S4;
		start( NORMALPRIO + 1 );
		buffer->write( data, 2, TIME_INFINITE );		
		buffer->clear();
		wait();

		seq = Sequence::S5;
		start( NORMALPRIO + 1 );
		assert( buffer->waitForReadAvailable( 3, TIME_MS2I( 1 ) ) == false );
		assert( buffer->readAvailable() < 3 );
		assert( buffer->waitForReadAvailable( 3, TIME_MS2I( 10 ) ) == true );	
		assert( buffer->readAvailable() >= 3 );
		wait();
	}

private:
	void main() final override
	{
		switch( seq )
		{
		case RingBufferTest::Sequence::S1:
		{
			for( uint8_t i = 0; i < 3; ++i )
			{
				chThdSleepMilliseconds( 3 );
				buffer->write( &i, 1 );
			}
			exit( MSG_OK );
			break;
		}
		case RingBufferTest::Sequence::S2:
		{
			for( uint8_t i = 0; i < 10; ++i )
			{
				chThdSleepMilliseconds( 3 );
				buffer->write( &i, 1 );
			}
			exit( MSG_OK );
			break;
		}
		case RingBufferTest::Sequence::S3:
		{
			for( uint8_t i = 0; i < 10; ++i )
				buffer->write( &i, 1, TIME_INFINITE );
			exit( MSG_OK );
			break;
		}
		case RingBufferTest::Sequence::S4:
		{
			uint8_t data[10];
			assert( buffer->read( data, 10, TIME_INFINITE ) == 2 );
			exit( MSG_OK );
			break;
		}
		case RingBufferTest::Sequence::S5:
		{
			uint8_t data[5] = { 1,2,3,4,5 };
			buffer->write( data, 1, TIME_INFINITE );
			chThdSleepMilliseconds( 2 );
			buffer->write( data + 1, 4, TIME_INFINITE );
			exit( MSG_OK );
			break;
		}
		default:
			break;
		}
	}

private:
	DynamicRingBuffer< uint8_t >* buffer;
	enum class Sequence { S1, S2, S3, S4, S5 } seq;
} ringBufferTest;


int main()
{
	halInit();
	chSysInit();

	ringBufferTest.exec();
	while( true )
		;

	return 0;
}