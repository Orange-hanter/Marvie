#include "Core/Assert.h"
#include "Core/Event.h"
#include "Core/MemoryStatus.h"
#include "SingleBRSensorReader.h"
#include "hal.h"
#include <math.h>

namespace SingleBRSensorReaderTest
{
	class Sensor : public AbstractBRSensor
	{
	public:
		class Data : public SensorData
		{
			friend class Sensor;
		} data;

		Sensor() {}
		const char* name() const final override { return "Sensor"; }
		void setIODevice( IODevice* ) final override {}
		uint32_t sensorDataSize() final override { return 0; }

		Data* readData() final override
		{
			chThdSleepMicroseconds( 1000 );
			data.errType = SensorData::Error::NoError;
			data.t.time().setMsec( ( uint32_t )TIME_I2MS( chVTGetSystemTimeX() ) );
			data.t.time().setSec( TIME_I2S( chVTGetSystemTimeX() ) );
			return &data;
		}
		Data* sensorData() final override { return &data; }
	};

	int test()
	{
		Thread::getAndClearEvents( ALL_EVENTS );
		auto mem0 = MemoryStatus::freeSpace( MemoryStatus::Region::All );

		SingleBRSensorReader* thread = new SingleBRSensorReader;

		EventListener listener;
		thread->eventSource().registerMask( &listener, EVENT_MASK( 0 ) );

		Sensor* sensor = new Sensor;
		thread->setSensor( sensor, TIME_MS2I( 10 ), TIME_MS2I( 10 ) );

		thread->startReading( NORMALPRIO );
		systime_t t0 = chVTGetSystemTimeX();
		systime_t t1 = t0 + TIME_S2I( 1 );
		int counter = 0;
		while( chVTIsSystemTimeWithinX( t0, t1 ) )
		{
			Thread::waitAnyEvent( ALL_EVENTS );
			if( listener.getAndClearFlags() & SingleBRSensorReader::SensorDataUpdated )
				++counter;
		}
		thread->stopReading();
		thread->waitForStateChange();

		assert( abs( counter - 100 ) <= 2 );

		listener.getAndClearFlags();
		Thread::getAndClearEvents( ALL_EVENTS );
		thread->setSensor( sensor, 0, 0 );

		thread->startReading( NORMALPRIO );
		t0 = chVTGetSystemTimeX();
		t1 = t0 + TIME_S2I( 1 );
		int counter2 = 0;
		while( chVTIsSystemTimeWithinX( t0, t1 ) )
		{
			Thread::waitAnyEvent( ALL_EVENTS );
			if( listener.getAndClearFlags() & SingleBRSensorReader::SensorDataUpdated )
				++counter2;
		}
		thread->stopReading();
		thread->waitForStateChange();

		assert( abs( counter2 - 1000 ) <= 2 );
		
		delete thread;
		delete sensor;
		assert( mem0 == MemoryStatus::freeSpace( MemoryStatus::Region::All ) );

		return 0;
	}
} // namespace SingleBRSensorReaderTest