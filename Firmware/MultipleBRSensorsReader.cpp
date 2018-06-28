#include "MultipleBRSensorsReader.h"
#include "Core/Assert.h"
#include "chmemcore.h"

#define SENSOR_ELEMENT( i ) ( static_cast< SensorElement* >( static_cast< SensorNode* >( i ) ) )

memory_pool_t MultipleBRSensorsReader::objectPool = _MEMORYPOOL_DATA( MultipleBRSensorsReader::objectPool, sizeof( MultipleBRSensorsReader::SensorElement ), PORT_NATURAL_ALIGN, nullptr );

MultipleBRSensorsReader::MultipleBRSensorsReader() : BaseDynamicThread( MULTIPLE_BR_SENSORS_READER_STACK_SIZE ), BRSensorReader()
{
	minInterval = TIME_MS2I( 1000 );
	sumTimeError = 0;
	_nextSensor = nullptr;
	updatedSensorsRoot = nullptr;
}

MultipleBRSensorsReader::~MultipleBRSensorsReader()
{
	stopReading();
	waitForStateChange();
	removeAllSensorElements();
}

MultipleBRSensorsReader::SensorElement* MultipleBRSensorsReader::createSensorElement()
{
	return alloc();
}

MultipleBRSensorsReader::SensorElement* MultipleBRSensorsReader::createSensorElement( AbstractBRSensor* sensor, sysinterval_t normalPriod, sysinterval_t emergencyPeriod )
{
	SensorElement* e = alloc();
	e->value.sensor = sensor;
	e->value.normalPriod = normalPriod;
	e->value.emergencyPeriod = emergencyPeriod;

	return e;
}

void MultipleBRSensorsReader::deleteSensorElement( SensorElement* e )
{
	free( e );
}

void MultipleBRSensorsReader::addSensorElement( SensorElement* sensorElement )
{
	if( tState != State::Stopped )
	{
		assert( false );
		return;
	}

	sensorElement->next = nullptr;
	elist.pushBack( sensorElement );
}

void MultipleBRSensorsReader::removeAllSensorElements()
{
	if( tState != State::Stopped )
	{
		assert( false );
		return;
	}

	for( auto i = elist.popFront(); i; i = elist.popFront() )
		deleteSensorElement( static_cast< SensorElement* >( i ) );
	updatedSensorsRoot = nullptr;
}

void MultipleBRSensorsReader::moveAllSensorElementsTo( NanoList< SensorDesc >& list )
{
	if( tState != State::Stopped )
	{
		assert( false );
		return;
	}

	list.insert( list.end(), elist );
	updatedSensorsRoot = nullptr;
}

void MultipleBRSensorsReader::setMinInterval( sysinterval_t minInterval )
{
	this->minInterval = minInterval;
}

void MultipleBRSensorsReader::startReading( tprio_t prio )
{
	if( tState != State::Stopped || !elist.size() )
	{
		assert( false );
		return;
	}

	tState = State::Working;
	extEventSource.broadcastFlags( ( eventflags_t )EventFlag::StateChanged );
	start( prio );
}

void MultipleBRSensorsReader::stopReading()
{
	chSysLock();
	if( tState == State::Stopped || tState == State::Stopping )
	{
		chSysUnlock();
		return;
	}
	tState = State::Stopping;
	extEventSource.broadcastFlagsI( ( eventflags_t )EventFlag::StateChanged );
	signalEventsI( InnerEventFlag::StopRequestFlag );
	chSchRescheduleS();
	chSysUnlock();
}

AbstractBRSensor* MultipleBRSensorsReader::nextUpdatedSensor()
{
	AbstractBRSensor* next;
	chSysLock();
	if( updatedSensorsRoot )
	{
		next = updatedSensorsRoot->value.sensor;
		SensorElement* tmp = updatedSensorsRoot;
		if( updatedSensorsRoot == updatedSensorsRoot->next )
			updatedSensorsRoot = nullptr;
		else
			updatedSensorsRoot = updatedSensorsRoot->next;
		tmp->next = nullptr;
	}
	else
		next = nullptr;
	chSysUnlock();

	return next;
}

AbstractBRSensor* MultipleBRSensorsReader::nextSensor()
{
	return _nextSensor;
}

void MultipleBRSensorsReader::main()
{
	sumTimeError = 0;
	prepareList();
	getAndClearEvents( ALL_EVENTS );

	virtual_timer_t timer;
	chVTObjectInit( &timer );
	if( minInterval == 0 )
		signalEvents( InnerEventFlag::TimeoutFlag );
	else
		chVTSet( &timer, minInterval, timerCallback, this );

	while( tState == State::Working )
	{
		eventmask_t em = chEvtWaitAny( ALL_EVENTS );
		if( em & InnerEventFlag::StopRequestFlag )
			break;
		if( em & InnerEventFlag::TimeoutFlag )
		{
			auto currentNode = elist.popFront();
			systime_t t0 = chVTGetSystemTimeX();
			auto data = currentNode->value.sensor->readData();
			addTimeError( chVTTimeElapsedSinceX( t0 ) );
			insert( currentNode, data->isValid() ? currentNode->value.normalPriod : currentNode->value.emergencyPeriod );

			SensorElement* next = SENSOR_ELEMENT( elist.begin() );
			_nextSensor = next->value.sensor;
			if( next->delta < sumTimeError )
				sumTimeError -= next->delta, next->delta = 0;
			else
				next->delta -= sumTimeError, sumTimeError = 0;
			sysinterval_t nextInterval;
			if( next->delta < minInterval )
				addTimeError( minInterval - next->delta ), nextInterval = minInterval;
			else
				nextInterval = next->delta;

			chSysLock();
			if( !static_cast< SensorElement* >( currentNode )->next )
			{
				if( updatedSensorsRoot )
				{
					static_cast< SensorElement* >( currentNode )->next = updatedSensorsRoot;
					updatedSensorsRoot = static_cast< SensorElement* >( currentNode );
				}
				else
				{
					updatedSensorsRoot = static_cast< SensorElement* >( currentNode )->next = static_cast< SensorElement* >( currentNode );
					extEventSource.broadcastFlagsI( EventFlag::SensorDataUpdated );
				}
			}
			chSchRescheduleS();
			chSysUnlock();

			if( nextInterval == 0 )
				signalEvents( InnerEventFlag::TimeoutFlag );
			else
				chVTSet( &timer, nextInterval, timerCallback, this );
		}
	}

	chVTReset( &timer );

	chSysLock();
	tState = State::Stopped;
	_nextSensor = nullptr;
	extEventSource.broadcastFlagsI( ( eventflags_t )EventFlag::StateChanged );
	chThdDequeueNextI( &waitingQueue, MSG_OK );
	exitS( MSG_OK );
}

void MultipleBRSensorsReader::prepareList()
{
	NanoList< SensorDesc > tmpList;
	tmpList.insert( tmpList.end(), elist );
	SensorNode* node;
	while( ( node = tmpList.popFront() ) )
		insert( node, node->value.normalPriod );
}

void MultipleBRSensorsReader::insert( SensorNode* node, sysinterval_t period )
{
	if( !elist.size() )
	{
		static_cast< SensorElement* >( node )->delta = period;
		elist.pushBack( node );
		return;
	}

	auto i = elist.begin();
	auto end = elist.end();
	while( SENSOR_ELEMENT( i )->delta <= period )
	{
		period -= SENSOR_ELEMENT( i )->delta;
		if( ++i == end )
			goto Leave;
	}
	SENSOR_ELEMENT( i )->delta -= period;
Leave:
	elist.insert( i, node );
	static_cast< SensorElement* >( node )->delta = period;
}

void MultipleBRSensorsReader::addTimeError( sysinterval_t dt )
{
	sysinterval_t sum = sumTimeError + dt;
	if( sum < sumTimeError ) // overflow check
		sumTimeError = TIME_MAX_INTERVAL;
	else
		sumTimeError = sum;
}

MultipleBRSensorsReader::SensorElement* MultipleBRSensorsReader::alloc()
{
	chSysLock();
	void* sd = chPoolAllocI( &objectPool );
	if( !sd )
	{
		void* p = chCoreAllocI( sizeof( SensorElement ) * 10 );
		for( int i = 0; i < 10; ++i )
		{
			chPoolFreeI( &objectPool, p );
			p = ( uint8_t* )p + sizeof( SensorElement );
		}
		sd = chPoolAllocI( &objectPool );
	}
	chSysUnlock();

	return new( sd ) SensorElement;
}

void MultipleBRSensorsReader::free( void* sd )
{
	chPoolFree( &objectPool, sd );
}

void MultipleBRSensorsReader::timerCallback( void* p )
{
	chSysLockFromISR();
	reinterpret_cast< MultipleBRSensorsReader* >( p )->signalEventsI( InnerEventFlag::TimeoutFlag );
	chSysUnlockFromISR();
}

MultipleBRSensorsReader::SensorElement::SensorElement() : delta( 0 ), next( nullptr ) {}

void* MultipleBRSensorsReader::SensorElement::operator new( size_t size, void* where )
{
	return where;
}

MultipleBRSensorsReader::SensorDesc::SensorDesc() : sensor( nullptr ), normalPriod( 0 ), emergencyPeriod( 0 ) {}