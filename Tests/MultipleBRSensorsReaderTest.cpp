#include "MultipleBRSensorsReader.h"
#include "Core/Assert.h"
#include "hal.h"
#include <math.h>

static MultipleBRSensorsReader thread;

class Sensor : public AbstractBRSensor
{
public:
	int id;
	Sensor( int id ) : id( id ) {}
	const char* name() const final override { return "Sensor"; }
	void setIODevice( IODevice* ) final override {}
	IODevice* ioDevice() final override { return nullptr; }
};

class GoodSensor : public Sensor
{
public:	
	class Data : public SensorData { friend class GoodSensor; } data;

	GoodSensor( int id ) : Sensor( id ) {}
	Data* readData() final override
	{
		chThdSleepMicroseconds( 1000 );
		data.valid = true;
		data.t.setMsec( ( uint32_t )TIME_I2MS( chVTGetSystemTimeX() ) );
		data.t.setSec( TIME_I2S( chVTGetSystemTimeX() ) );
		return &data;
	}
	Data* sensorData() final override { return &data; }
};

class BadSensor : public Sensor
{
public:
	class Data : public SensorData { friend class BadSensor; } data;

	BadSensor( int id ) : Sensor( id ) {}
	Data* readData() final override
	{
		chThdSleepMicroseconds( 1000 );
		data.valid = false;
		return &data;
	}
	Data* sensorData() final override { return &data; }
};

int main()
{
	halInit();
	chSysInit();

	thread.setMinInterval( 1 );
	thread.addSensorElement( thread.createSensorElement( new GoodSensor( 0 ), TIME_MS2I( 10 ), TIME_MS2I( 10 ) ) );
	thread.addSensorElement( thread.createSensorElement( new GoodSensor( 1 ), TIME_MS2I( 4 ), TIME_MS2I( 4 ) ) );
	thread.addSensorElement( thread.createSensorElement( new GoodSensor( 2 ), TIME_MS2I( 10 ), TIME_MS2I( 10 ) ) );
	thread.addSensorElement( thread.createSensorElement( new BadSensor( 3 ), TIME_MS2I( 12 ), TIME_MS2I( 4 ) ) );

	EvtListener listener;
	thread.eventSource()->registerMask( &listener, EVENT_MASK( 0 ) );
	int n = 0;
	struct Info
	{
		Time time;
		int id;
	} info[6];

	thread.startReading( NORMALPRIO );
	while( n < 6 )
	{
		chEvtWaitAny( ALL_EVENTS );
		if( listener.getAndClearFlags() & MultipleBRSensorsReader::SensorDataUpdated )
		{
			Sensor* s;
			while( ( s = static_cast< Sensor* >( thread.nextUpdatedSensor() ) ) )
			{
				info[n].id = s->id;
				info[n].time = s->sensorData()->time();
				++n;
			}
		}
	}
	thread.stopReading();
	thread.waitForStateChange();

	assert( info[0].id == 1 );
	assert( info[1].id == 1 );
	assert( info[2].id == 0 );
	assert( info[3].id == 2 );
	assert( info[4].id == 3 );
	assert( info[5].id == 1 );

	listener.getAndClearFlags();
	chEvtGetAndClearEvents( ALL_EVENTS );
	while( thread.nextUpdatedSensor() );

	thread.setMinInterval( 0 );
	thread.startReading( NORMALPRIO );
	systime_t t0 = chVTGetSystemTimeX();
	systime_t t1 = t0 + TIME_S2I( 4 );
	int counter[4] = {};
	while( chVTIsSystemTimeWithinX( t0, t1 ) )
	{
		chEvtWaitAny( ALL_EVENTS );
		if( listener.getAndClearFlags() & MultipleBRSensorsReader::SensorDataUpdated )
		{
			Sensor* s;
			while( ( s = static_cast< Sensor* >( thread.nextUpdatedSensor() ) ) )
				++counter[s->id];
		}
	}
	thread.stopReading();
	thread.waitForStateChange();

	assert( abs( counter[0] - 400 ) <= 2 );
	assert( abs( counter[1] - 1000 ) <= 2 );
	assert( abs( counter[2] - 400 ) <= 2 );
	assert( abs( counter[3] - 1000 ) <= 2 );

	listener.getAndClearFlags();
	chEvtGetAndClearEvents( ALL_EVENTS );
	while( thread.nextUpdatedSensor() );

	thread.setMinInterval( TIME_MS2I( 100 ) );
	thread.startReading( NORMALPRIO );
	t0 = chVTGetSystemTimeX();
	t1 = t0 + TIME_S2I( 4 );
	int counter2[4] = {};
	while( chVTIsSystemTimeWithinX( t0, t1 ) )
	{
		chEvtWaitAny( ALL_EVENTS );
		if( listener.getAndClearFlags() & MultipleBRSensorsReader::SensorDataUpdated )
		{
			Sensor* s;
			while( ( s = static_cast< Sensor* >( thread.nextUpdatedSensor() ) ) )
				++counter2[s->id];
		}
	}
	thread.stopReading();
	thread.waitForStateChange();

	assert( abs( counter2[0] - 6 ) <= 2 );
	assert( abs( counter2[1] - 14 ) <= 2 );
	assert( abs( counter2[2] - 6 ) <= 2 );
	assert( abs( counter2[3] - 14 ) <= 2 );

	listener.getAndClearFlags();
	chEvtGetAndClearEvents( ALL_EVENTS );
	thread.removeAllSensorElements();

	thread.addSensorElement( thread.createSensorElement( new GoodSensor( 0 ), TIME_MS2I( 10 ), TIME_MS2I( 10 ) ) );

	thread.setMinInterval( 0 );
	thread.startReading( NORMALPRIO );
	t0 = chVTGetSystemTimeX();
	t1 = t0 + TIME_S2I( 1 );
	int counter3[4] = {};
	while( chVTIsSystemTimeWithinX( t0, t1 ) )
	{
		chEvtWaitAny( ALL_EVENTS );
		if( listener.getAndClearFlags() & MultipleBRSensorsReader::SensorDataUpdated )
		{
			Sensor* s;
			while( ( s = static_cast< Sensor* >( thread.nextUpdatedSensor() ) ) )
				++counter3[s->id];
		}
	}
	thread.stopReading();
	thread.waitForStateChange();

	assert( abs( counter3[0] - 100 ) <= 2 );
	assert( counter3[1] == 0 );
	assert( counter3[2] == 0 );
	assert( counter3[3] == 0 );

	while( true )
		;

	return 0;
}