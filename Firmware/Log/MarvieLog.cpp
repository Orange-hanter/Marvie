#include "MarvieLog.h"
#include "Core/DateTimeService.h"
#include "Core/Assert.h"
#include "Crc32HW.h"
#include <string.h>

MarvieLog::MarvieLog() : BaseDynamicThread( 2048 )
{
	logState = State::Stopped;
	chThdQueueObjectInit( &waitingQueue );
	maxSize = 100 * 1024 * 1024;
	logSize = 0;
	overwritingEnabled = true;
	onlyNewDigitSignal = true;
	dSignalPeriod = TIME_S2I( 1 );
	aSignalPeriod = TIME_S2I( 1 );
	digitBlocksCount = analogBlocksCount = 0;
	analogChannelsCount = 0;
	signalProvider = nullptr;
	pendingFlags[0] = pendingFlags[1] = pendingFlags[2] = pendingFlags[3] = 0;
}

MarvieLog::~MarvieLog()
{
	stopLogging();
	waitForStop();
}

void MarvieLog::setRootPath( const TCHAR* path )
{
	if( logState != State::Stopped )
		return;

	rootDir.setPath( path );
	rootDir.mkpath();
}

void MarvieLog::setMaxSize( uint32_t mb )
{
	if( logState != State::Stopped )
		return;

	maxSize = mb * 1024 * 1024;
}

void MarvieLog::setOverwritingEnabled( bool enabled )
{
	if( logState != State::Stopped )
		return;

	overwritingEnabled = enabled;
}

void MarvieLog::setOnlyNewDigitSignal( bool enabled )
{
	if( logState != State::Stopped )
		return;

	onlyNewDigitSignal = enabled;
}

void MarvieLog::setDigitSignalPeriod( uint32_t msec )
{
	if( logState != State::Stopped )
		return;

	dSignalPeriod = TIME_MS2I( msec );
}

void MarvieLog::setAnalogSignalPeriod( uint32_t msec )
{
	if( logState != State::Stopped )
		return;

	aSignalPeriod = TIME_MS2I( msec );
}

void MarvieLog::setSignalBlockDescList( const std::list< SignalBlockDesc >& list )
{
	if( logState != State::Stopped )
		return;

	digitBlocksCount = analogBlocksCount = 0;
	analogChannelsCount = 0;
	blockDescVect.resize( list.size() );
	std::size_t i = 0;
	for( auto& iList : list )
	{
		blockDescVect[i] = iList;
		if( blockDescVect[i].digitCount )
			++digitBlocksCount;
		if( blockDescVect[i].analogCount )
		{
			++analogBlocksCount;
			analogChannelsCount += blockDescVect[i].analogCount;
		}
		++i;
	}
	assert( digitBlocksCount <= sizeof( buffer ) / 4 - 17 );
	assert( analogChannelsCount <= sizeof( buffer ) / 4 - 18 );
}

void MarvieLog::setSignalProvider( AbstractSRSensor::SignalProvider* signalProvider )
{
	if( logState != State::Stopped )
		return;

	this->signalProvider = signalProvider;
}

bool MarvieLog::clean()
{
	chSysLock();
	if( logState == State::Stopped || logState == State::Stopping )
	{
		chSysUnlock();
		return false;
	}
	chEvtSignalI( thread_ref, CleanRequestEvent );
	chSchRescheduleS();
	chSysUnlock();
	
	return true;
}

bool MarvieLog::startLogging( tprio_t prio /*= NORMALPRIO */ )
{
	if( logState != State::Stopped /*|| ( ( dSignalPeriod != 0 || aSignalPeriod != 0 ) && !signalProvider )*/ )
		return false;

	logState = State::Working;
	start( prio );
	return true;
}

void MarvieLog::stopLogging()
{
	chSysLock();
	if( logState == State::Stopped || logState == State::Stopping )
	{
		chSysUnlock();
		return;
	}
	logState = State::Stopping;
	chEvtSignalI( thread_ref, StopRequestEvent );
	chSchRescheduleS();
	chSysUnlock();
}

bool MarvieLog::waitForStop( sysinterval_t timeout /*= TIME_INFINITE */ )
{
	msg_t msg = MSG_OK;
	chSysLock();
	if( logState == State::Stopping )
		msg = chThdEnqueueTimeoutS( &waitingQueue, timeout );
	chSysUnlock();

	return msg == MSG_OK;
}

MarvieLog::State MarvieLog::state()
{
	return logState;
}

uint64_t MarvieLog::size()
{
	return logSize;
}

void MarvieLog::updateSensor( AbstractSensor* sensor, const std::string* name )
{
	chSysLock();
	if( logState == State::Working || logState == State::Archiving )
	{
		mutex.lockS();
		chEvtSignalI( thread_ref, PendingSensorEvent );
		chSchRescheduleS();
		chSysUnlock();

		uint32_t block = sensor->userData() / 64;
		uint32_t bit = sensor->userData() % 64;
		if( !( pendingFlags[block] & ( 1ULL << bit ) ) )
		{
			pendingFlags[block] |= ( 1ULL << bit );
			pendingSensors.push_back( SensorDesc( sensor, name ) );
		}
		mutex.unlock();
	}
	else
		chSysUnlock();
}

void MarvieLog::main()
{
	for( auto& i : blockDescVect )
		i.digitBlockData = 0xFFFFFFFF;
	logSize = rootDir.contentSize();
	if( logSize > maxSize && overwritingEnabled )
		free( logSize - maxSize );

	virtual_timer_t digitSignalTimer, analogSignalTimer;
	chVTObjectInit( &digitSignalTimer );
	chVTObjectInit( &analogSignalTimer );
	if( dSignalPeriod )
		chVTSet( &digitSignalTimer, dSignalPeriod, dTimerCallback, this );
	if( aSignalPeriod )
		chVTSet( &analogSignalTimer, aSignalPeriod, aTimerCallback, this );

	while( logState != State::Stopping )
	{
		eventmask_t em = chEvtWaitAny( ALL_EVENTS );
		if( em & StopRequestEvent )
			break;
		if( em & CleanRequestEvent )
		{
			rootDir.removeRecursively();
			rootDir.mkpath();
			logSize = 0;
		}
		if( em & DigitSignalTimerEvent )
		{
			chVTSet( &digitSignalTimer, dSignalPeriod, dTimerCallback, this );
			logDigitInputs();
		}
		if( em & AnalogSignalTimerEvent )
		{
			chVTSet( &analogSignalTimer, aSignalPeriod, aTimerCallback, this );
			logAnalogInputs();
		}
		if( em & PendingSensorEvent )
			logSensorData();
	}

	chVTReset( &digitSignalTimer );
	chVTReset( &analogSignalTimer );

	pendingSensors.clear();
	pendingFlags[0] = pendingFlags[1] = pendingFlags[2] = pendingFlags[3] = 0;

	chSysLock();
	logState = State::Stopped;
	chThdDequeueNextI( &waitingQueue, MSG_OK );
	exitS( MSG_OK );
}

void MarvieLog::logDigitInputs()
{
	if( onlyNewDigitSignal )
	{
		volatile bool updated = false;
		for( uint32_t i = 0; i < blockDescVect.size(); ++i )
		{
			if( blockDescVect[i].digitCount > 0 && blockDescVect[i].digitBlockData != signalProvider->digitSignals( i ) )
			{
				updated = true;
				break;
			}
		}
		if( !updated )
			return;
	}

	uint32_t recordSize = 4 + sizeof( DateTime ) + 1 + digitBlocksCount * 4 + 4;
	if( logSize + recordSize > maxSize )
	{
		if( overwritingEnabled )
		{
			if( !free( logSize + recordSize - maxSize ) )
				return;
		}
		else
			return;
	}

	DateTime dateTime = DateTimeService::currentDateTime();
	sprintf( ( char* )buffer, "%d/%d/%d", ( int )dateTime.date().year(), ( int )dateTime.date().month(), ( int )dateTime.date().day() );
	rootDir.mkpath( ( char* )buffer );
	sprintf( ( char* )buffer, "%s/%d/%d/%d/dInputs.bin", rootDir.path().c_str(), ( int )dateTime.date().year(), ( int )dateTime.date().month(), ( int )dateTime.date().day() );
	if( file.open( ( char* )buffer, FileSystem::OpenAppend | FileSystem::Write ) )
	{
		buffer[0] = '_'; buffer[1] = 'l'; buffer[2] = 'o'; buffer[3] = 'g';
		*( DateTime* )( buffer + 4 ) = dateTime;
		buffer[4 + sizeof( DateTime )] = ( uint8_t )digitBlocksCount;
		for( uint32_t i = 0, count = 0; count < digitBlocksCount; ++i )
		{
			if( !blockDescVect[i].digitCount )
				continue;
			blockDescVect[i].digitBlockData = signalProvider->digitSignals( i );
			( ( uint32_t* )( buffer + 4 + sizeof( DateTime ) + 1 ) )[count] = blockDescVect[i].digitBlockData;
			++count;
		}

		Crc32HW::acquire();
		*( uint32_t* )( buffer + 4 + sizeof( DateTime ) + 1 + digitBlocksCount * 4 ) = Crc32HW::crc32Byte( buffer, 4 + sizeof( DateTime ) + 1 + digitBlocksCount * 4, 0xFFFFFFFF );
		Crc32HW::release();

		logSize += file.write( buffer, recordSize );
		file.close();
	}
}

void MarvieLog::logAnalogInputs()
{
	uint32_t recordSize = 4 + sizeof( DateTime ) + 1 + analogChannelsCount * 4 + 4;
	if( logSize + recordSize > maxSize )
	{
		if( overwritingEnabled )
		{
			if( !free( logSize + recordSize - maxSize ) )
				return;
		}
		else
			return;
	}

	DateTime dateTime = DateTimeService::currentDateTime();
	sprintf( ( char* )buffer, "%d/%d/%d", ( int )dateTime.date().year(), ( int )dateTime.date().month(), ( int )dateTime.date().day() );
	rootDir.mkpath( ( char* )buffer );
	sprintf( ( char* )buffer, "%s/%d/%d/%d/aInputs.bin", rootDir.path().c_str(), ( int )dateTime.date().year(), ( int )dateTime.date().month(), ( int )dateTime.date().day() );
	if( file.open( ( char* )buffer, FileSystem::OpenAppend | FileSystem::Write ) )
	{
		buffer[0] = '_'; buffer[1] = 'l'; buffer[2] = 'o'; buffer[3] = 'g';
		*( DateTime* )( buffer + 4 ) = dateTime;
		*( uint8_t* )( buffer + 4 + sizeof( DateTime ) ) = analogChannelsCount;

		uint32_t offset = 0;
		for( uint32_t i = 0; i < blockDescVect.size(); ++i )
		{
			uint32_t count = blockDescVect[i].analogCount;
			if( !count )
				continue;
			for( uint32_t line = 0; line < count; ++line )
				( ( float* )( buffer + 4 + sizeof( DateTime ) + 1 ) )[offset++] = signalProvider->analogSignal( i, line );
		}

		Crc32HW::acquire();
		*( uint32_t* )( buffer + 4 + sizeof( DateTime ) + 1 + analogChannelsCount * 4 ) = Crc32HW::crc32Byte( buffer, 4 + sizeof( DateTime ) + 1 + analogChannelsCount * 4, 0xFFFFFFFF );
		Crc32HW::release();

		logSize += file.write( buffer, recordSize );
		file.close();
	}
}

void MarvieLog::logSensorData()
{
	mutex.lock();
	std::size_t count = pendingSensors.size();
	mutex.unlock();

	while( logState != State::Stopping && count-- )
	{
		mutex.lock();
		AbstractSensor* sensor = pendingSensors.front().sensor;
		const std::string* sensorName = pendingSensors.front().name;
		pendingSensors.pop_front();
		pendingFlags[sensor->userData() / 64] &= ~( 1ULL << ( sensor->userData() % 64 ) );
		mutex.unlock();

		uint32_t recordSize = 4 + sizeof( DateTime ) + sensor->sensorDataSize() + 4;
		if( logSize + recordSize > maxSize )
		{
			if( overwritingEnabled )
			{
				if( !free( logSize + recordSize - maxSize ) )
					continue;
			}
			else
				continue;
		}

		DateTime dateTime = sensor->sensorData()->time();
		sprintf( ( char* )buffer, "%d/%d/%d", ( int )dateTime.date().year(), ( int )dateTime.date().month(), ( int )dateTime.date().day() );
		rootDir.mkpath( ( char* )buffer );
		if( sensorName )
			sprintf( ( char* )buffer, "%s/%d/%d/%d/%s.%s.bin",
					 rootDir.path().c_str(), ( int )dateTime.date().year(), ( int )dateTime.date().month(), ( int )dateTime.date().day(),
					 sensorName->c_str(), sensor->name() );
		else
			sprintf( ( char* )buffer, "%s/%d/%d/%d/%d.%s.bin",
					 rootDir.path().c_str(), ( int )dateTime.date().year(), ( int )dateTime.date().month(), ( int )dateTime.date().day(),
					 ( int )sensor->userData() + 1, sensor->name() );
		if( file.open( ( char* )buffer, FileSystem::OpenAppend | FileSystem::Write ) )
		{
			auto sensorData = sensor->sensorData();
			sensorData->lock();
			if( sensorData->isValid() )
			{
				buffer[0] = '_'; buffer[1] = 'l'; buffer[2] = 'o'; buffer[3] = 'g';
				*( DateTime* )( buffer + 4 ) = dateTime;

				Crc32HW::acquire();
				uint32_t crc = Crc32HW::crc32Byte( buffer, 4 + sizeof( DateTime ), ( uint8_t* )sensorData + sizeof( SensorData ), sensor->sensorDataSize() );
				Crc32HW::release();

				logSize += file.write( buffer, 4 + sizeof( DateTime ) );
				logSize += file.write( ( uint8_t* )sensorData + sizeof( SensorData ), sensor->sensorDataSize() );
				logSize += file.write( ( uint8_t* )&crc, 4 );
			}

			sensorData->unlock();
			file.close();
		}
	}
}

bool MarvieLog::free( uint64_t size )
{
	auto yearList = rootDir.entryList( Dir::Dirs, Dir::Name );
	for( auto& iYear : yearList )
	{
		Dir yearDir( rootDir.path() + "/" + iYear );
		auto monthList = yearDir.entryList( Dir::Dirs, Dir::Name );
		for( auto& iMonth : monthList )
		{
			Dir mountDir( yearDir.path() + "/" + iMonth );
			auto dayList = mountDir.entryList( Dir::Dirs, Dir::Name );
			for( auto& iDay : dayList )
			{
				Dir dir( mountDir.path() + "/" + iDay );
				auto cs = dir.contentSize();
				if( dir.removeRecursively() )
				{
					logSize -= cs;
					if( cs >= size )
						return true;
					size -= cs;
				}
			}
			mountDir.removeRecursively();
		}
		yearDir.removeRecursively();
	}

	return false;
}

void MarvieLog::dTimerCallback( void* p )
{
	chSysLockFromISR();
	chEvtSignalI( ( ( MarvieLog* )p )->thread_ref, DigitSignalTimerEvent );
	chSysUnlockFromISR();
}

void MarvieLog::aTimerCallback( void* p )
{
	chSysLockFromISR();
	chEvtSignalI( ( ( MarvieLog* )p )->thread_ref, AnalogSignalTimerEvent );
	chSysUnlockFromISR();
}
