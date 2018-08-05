#include "ch.h"
#include "hal.h"
#include "ChibiOS/various/cpp_wrappers/ch.hpp"
#include "core/Assert.h"
#include "Core/RingBuffer.h"

using namespace chibios_rt;

class RingBufferTestClass : private BaseStaticThread< 256 >
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

		buffer->clear();
		data[0] = 1; data[1] = 2; data[2] = 3; data[3] = 4; data[4] = 5;
		buffer->write( data, 5 );
		buffer->read( nullptr, 2 );
		buffer->write( data, 2 );
		assert( buffer->peek( 0 ) == 3 );
		assert( buffer->peek( 4 ) == 2 );
		uint8_t tmp[5];
		for( int i = 0; i < 5; ++i )
			buffer->peek( i, tmp + i, 1 );
		uint8_t tmp2[5];
		buffer->peek( 0, tmp2, 5 );
		assert( tmp[0] == tmp2[0] && tmp[0] == 3 );
		assert( tmp[1] == tmp2[1] && tmp[1] == 4 );
		assert( tmp[2] == tmp2[2] && tmp[2] == 5 );
		assert( tmp[3] == tmp2[3] && tmp[3] == 1 );
		assert( tmp[4] == tmp2[4] && tmp[4] == 2 );
	}

private:
	void main() final override
	{
		switch( seq )
		{
		case RingBufferTestClass::Sequence::S1:
		{
			for( uint8_t i = 0; i < 3; ++i )
			{
				chThdSleepMilliseconds( 3 );
				buffer->write( &i, 1 );
			}
			exit( MSG_OK );
			break;
		}
		case RingBufferTestClass::Sequence::S2:
		{
			for( uint8_t i = 0; i < 10; ++i )
			{
				chThdSleepMilliseconds( 3 );
				buffer->write( &i, 1 );
			}
			exit( MSG_OK );
			break;
		}
		case RingBufferTestClass::Sequence::S3:
		{
			for( uint8_t i = 0; i < 10; ++i )
				buffer->write( &i, 1, TIME_INFINITE );
			exit( MSG_OK );
			break;
		}
		case RingBufferTestClass::Sequence::S4:
		{
			uint8_t data[10];
			assert( buffer->read( data, 10, TIME_INFINITE ) == 2 );
			exit( MSG_OK );
			break;
		}
		case RingBufferTestClass::Sequence::S5:
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
} ringBufferTestClass;


int ringBufferTest()
{
	ringBufferTestClass.exec();

	return 0;
}