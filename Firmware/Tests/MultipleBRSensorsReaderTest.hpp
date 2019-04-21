#include "Core/Assert.h"
#include "Core/Event.h"
#include "Core/MemoryStatus.h"
#include "MultipleBRSensorsReader.h"
#include "hal.h"
#include <math.h>

namespace MultipleBRSensorsReaderTest
{
	static systime_t gt0 = 0;

	class Sensor : public AbstractBRSensor
	{
	public:
		int id;
		Sensor( int id ) : id( id ) {}
		const char* name() const final override { return "Sensor"; }
	};

	class GoodSensor : public Sensor
	{
	public:
		class Data : public SensorData
		{
			friend class GoodSensor;
		} data;

		GoodSensor( int id ) : Sensor( id ) {}
		Data* readData() final override
		{
			chThdSleepMicroseconds( 1000 );
			data.errType = SensorData::Error::NoError;
			data.t.time().setMsec( ( uint32_t )TIME_I2US( chVTGetSystemTimeX() - gt0 ) / 1000 );
			data.t.time().setSec( TIME_I2MS( chVTGetSystemTimeX() - gt0 ) / 1000 );
			return &data;
		}
		Data* sensorData() final override { return &data; }
		uint32_t sensorDataSize() final override { return sizeof( Data ) - sizeof( SensorData ); }
	};

	class BadSensor : public Sensor
	{
	public:
		class Data : public SensorData
		{
			friend class BadSensor;
		} data;

		BadSensor( int id ) : Sensor( id ) {}
		Data* readData() final override
		{
			chThdSleepMicroseconds( 1000 );
			data.errType = SensorData::Error::CrcError;
			return &data;
		}
		Data* sensorData() final override { return &data; }
		uint32_t sensorDataSize() final override { return sizeof( Data ) - sizeof( SensorData ); }
	};

	void removeAllSensors( MultipleBRSensorsReader* reader )
	{
		NanoList< MultipleBRSensorsReader::SensorDesc > list;
		reader->moveAllSensorElementsTo( list );
		NanoList< MultipleBRSensorsReader::SensorDesc >::Node* node;
		while( ( node = list.popFront() ) )
		{
			delete( *node ).value.sensor;
			reader->deleteSensorElement( static_cast< MultipleBRSensorsReader::SensorElement* >( node ) );
		}
	}

	int test()
	{
		Thread::getAndClearEvents( ALL_EVENTS );
		//auto mem0 = MemoryStatus::freeSpace( MemoryStatus::Region::All );

		//====Test_1===========================================================================================================

		MultipleBRSensorsReader* thread = new MultipleBRSensorsReader;
		thread->setMinInterval( 1 );
		thread->addSensorElement( thread->createSensorElement( new GoodSensor( 0 ), TIME_MS2I( 10 ), TIME_MS2I( 10 ) ) );
		thread->addSensorElement( thread->createSensorElement( new GoodSensor( 1 ), TIME_MS2I( 4 ), TIME_MS2I( 4 ) ) );
		thread->addSensorElement( thread->createSensorElement( new GoodSensor( 2 ), TIME_MS2I( 10 ), TIME_MS2I( 10 ) ) );
		thread->addSensorElement( thread->createSensorElement( new BadSensor( 3 ), TIME_MS2I( 12 ), TIME_MS2I( 4 ) ) );

		EventListener listener;
		thread->eventSource().registerMask( &listener, EVENT_MASK( 0 ) );
		int n = 0;
		struct Info
		{
			Time time;
			int id;
		} info[6];

		thread->startReading( NORMALPRIO );
		while( n < 6 )
		{
			Thread::waitAnyEvent( ALL_EVENTS );
			if( listener.getAndClearFlags() & MultipleBRSensorsReader::SensorDataUpdated )
			{
				Sensor* s;
				while( ( s = static_cast< Sensor* >( thread->nextUpdatedSensor() ) ) )
				{
					info[n].id = s->id;
					info[n].time = s->sensorData()->time().time();
					++n;
				}
			}
		}
		thread->stopReading();
		thread->waitForStateChange();

		assert( info[0].id == 1 );
		assert( info[1].id == 1 );
		assert( info[2].id == 0 );
		assert( info[3].id == 2 );
		assert( info[4].id == 3 );
		assert( info[5].id == 1 );

		listener.getAndClearFlags();
		Thread::getAndClearEvents( ALL_EVENTS );
		while( thread->nextUpdatedSensor() )
			;

		//====Test_2===========================================================================================================

		thread->setMinInterval( 0 );
		thread->startReading( NORMALPRIO );
		systime_t t0 = chVTGetSystemTimeX();
		systime_t t1 = t0 + TIME_S2I( 4 );
		int counter[4] = {};
		while( chVTIsSystemTimeWithinX( t0, t1 ) )
		{
			Thread::waitAnyEvent( ALL_EVENTS );
			if( listener.getAndClearFlags() & MultipleBRSensorsReader::SensorDataUpdated )
			{
				Sensor* s;
				while( ( s = static_cast< Sensor* >( thread->nextUpdatedSensor() ) ) )
					++counter[s->id];
			}
		}
		thread->stopReading();
		thread->waitForStateChange();

		assert( abs( counter[0] - 400 ) <= 2 );
		assert( abs( counter[1] - 1000 ) <= 2 );
		assert( abs( counter[2] - 400 ) <= 2 );
		assert( abs( counter[3] - 1000 ) <= 3 );

		listener.getAndClearFlags();
		Thread::getAndClearEvents( ALL_EVENTS );
		while( thread->nextUpdatedSensor() )
			;

		//====Test_3===========================================================================================================

		thread->setMinInterval( TIME_MS2I( 100 ) );
		thread->startReading( NORMALPRIO );
		t0 = chVTGetSystemTimeX();
		t1 = t0 + TIME_S2I( 4 );
		int counter2[4] = {};
		while( chVTIsSystemTimeWithinX( t0, t1 ) )
		{
			Thread::waitAnyEvent( ALL_EVENTS );
			if( listener.getAndClearFlags() & MultipleBRSensorsReader::SensorDataUpdated )
			{
				Sensor* s;
				while( ( s = static_cast< Sensor* >( thread->nextUpdatedSensor() ) ) )
					++counter2[s->id];
			}
		}
		thread->stopReading();
		thread->waitForStateChange();

		assert( abs( counter2[0] - 6 ) <= 2 );
		assert( abs( counter2[1] - 14 ) <= 2 );
		assert( abs( counter2[2] - 6 ) <= 2 );
		assert( abs( counter2[3] - 14 ) <= 2 );

		listener.getAndClearFlags();
		Thread::getAndClearEvents( ALL_EVENTS );
		thread->removeAllSensorElements( true );

		//====Test_4===========================================================================================================

		thread->addSensorElement( thread->createSensorElement( new GoodSensor( 0 ), TIME_MS2I( 10 ), TIME_MS2I( 10 ) ) );

		thread->setMinInterval( 0 );
		thread->startReading( NORMALPRIO );
		t0 = chVTGetSystemTimeX();
		t1 = t0 + TIME_S2I( 1 );
		int counter3[4] = {};
		while( chVTIsSystemTimeWithinX( t0, t1 ) )
		{
			Thread::waitAnyEvent( ALL_EVENTS );
			if( listener.getAndClearFlags() & MultipleBRSensorsReader::SensorDataUpdated )
			{
				Sensor* s;
				while( ( s = static_cast< Sensor* >( thread->nextUpdatedSensor() ) ) )
					++counter3[s->id];
			}
		}
		thread->stopReading();
		thread->waitForStateChange();

		assert( abs( counter3[0] - 100 ) <= 2 );
		assert( counter3[1] == 0 );
		assert( counter3[2] == 0 );
		assert( counter3[3] == 0 );

		listener.getAndClearFlags();
		Thread::getAndClearEvents( ALL_EVENTS );
		thread->removeAllSensorElements( true );

		//====Test_6===========================================================================================================

		thread->addSensorElement( thread->createSensorElement( new GoodSensor( 0 ), TIME_MS2I( 10 ), TIME_MS2I( 10 ) ) );
		thread->addSensorElement( thread->createSensorElement( new GoodSensor( 1 ), TIME_MS2I( 14 ), TIME_MS2I( 14 ) ) );
		thread->addSensorElement( thread->createSensorElement( new GoodSensor( 2 ), TIME_MS2I( 18 ), TIME_MS2I( 18 ) ) );

		int n2 = 0;
		Info info2[6];

		gt0 = chVTGetSystemTimeX();
		thread->setMinInterval( TIME_MS2I( 1 ) );
		thread->startReading( NORMALPRIO );
		thread->forceAll();
		while( n2 < 6 )
		{
			Thread::waitAnyEvent( ALL_EVENTS );
			if( listener.getAndClearFlags() & MultipleBRSensorsReader::SensorDataUpdated )
			{
				Sensor* s;
				while( ( s = static_cast< Sensor* >( thread->nextUpdatedSensor() ) ) )
				{
					info2[n2].id = s->id;
					info2[n2].time = s->sensorData()->time().time();
					++n2;
				}
			}
		}
		thread->stopReading();
		thread->waitForStateChange();

		assert( info2[0].id == 0 && info2[0].time.msec() == 2 );
		assert( info2[1].id == 1 && info2[1].time.msec() == 4 );
		assert( info2[2].id == 2 && info2[2].time.msec() == 6 );
		assert( info2[3].id == 0 && info2[3].time.msec() == 12 );
		assert( info2[4].id == 1 && info2[4].time.msec() == 17 );
		assert( info2[5].id == 2 && info2[5].time.msec() == 22 );

		listener.getAndClearFlags();
		Thread::getAndClearEvents( ALL_EVENTS );
		thread->removeAllSensorElements( true );

		//====Test_5===========================================================================================================

		AbstractBRSensor* sensors[4];
		thread->addSensorElement( thread->createSensorElement( ( sensors[0] = new GoodSensor( 0 ) ), TIME_MS2I( 20 ), TIME_MS2I( 20 ) ) );
		thread->addSensorElement( thread->createSensorElement( ( sensors[1] = new GoodSensor( 1 ) ), TIME_MS2I( 28 ), TIME_MS2I( 28 ) ) );
		thread->addSensorElement( thread->createSensorElement( ( sensors[2] = new GoodSensor( 2 ) ), TIME_MS2I( 36 ), TIME_MS2I( 36 ) ) );
		thread->addSensorElement( thread->createSensorElement( ( sensors[3] = new GoodSensor( 3 ) ), TIME_MS2I( 105 ), TIME_MS2I( 105 ) ) );

		listener.getAndClearFlags();
		int n3 = 0;
		Info info3[6];
		Info info4;

		gt0 = chVTGetSystemTimeX();
		thread->setMinInterval( TIME_MS2I( 1 ) );
		thread->startReading( NORMALPRIO );
		thread->forceOne( sensors[2] );
		while( true )
		{
			Thread::waitAnyEvent( ALL_EVENTS );
			if( listener.getAndClearFlags() & MultipleBRSensorsReader::SensorDataUpdated )
			{
				Sensor* s;
				while( ( s = static_cast< Sensor* >( thread->nextUpdatedSensor() ) ) )
				{
					if( s == sensors[3] )
					{
						info4.id = s->id;
						info4.time = s->sensorData()->time().time();
						goto EndTest5;
					}
					if( n3 == 0 )
						thread->forceOne( sensors[1] );
					else if( n3 == 1 )
						thread->forceOne( sensors[0] );
					else if( n3 >= 6 )
						break;
					info3[n3].id = s->id;
					info3[n3].time = s->sensorData()->time().time();
					++n3;
				}
			}
		}
	EndTest5:
		thread->stopReading();
		thread->waitForStateChange();

		assert( info3[0].id == 2 && info3[0].time.msec() == 2 );
		assert( info3[1].id == 1 && info3[1].time.msec() == 4 );
		assert( info3[2].id == 0 && info3[2].time.msec() == 6 );
		assert( info3[3].id == 0 && info3[3].time.msec() == 26 );
		assert( info3[4].id == 1 && info3[4].time.msec() == 32 );
		assert( info3[5].id == 2 && info3[5].time.msec() == 38 );
		assert( info4.id == 3 && info4.time.msec() == 106 );

		listener.getAndClearFlags();
		Thread::getAndClearEvents( ALL_EVENTS );
		thread->removeAllSensorElements( true );

		delete thread;
		//assert( mem0 == MemoryStatus::freeSpace( MemoryStatus::Region::All ) );

		return 0;
	}
} // namespace MultipleBRSensorsReaderTest