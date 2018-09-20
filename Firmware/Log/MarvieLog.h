#pragma once

#include "Core/BaseDynamicThread.h"
#include "Drivers/Sensors/AbstractSensor.h"
#include "FileSystem/File.h"
#include "FileSystem/Dir.h"
#include <vector>
#include <list>

class MarvieLog : private BaseDynamicThread
{
public:
	enum class State { Stopped, Working, Archiving, Stopping };
	struct SignalBlockDesc
	{
		uint16_t digitCount;
		uint16_t analogCount;	
	};

	MarvieLog();
	~MarvieLog();

	void setRootPath( const TCHAR* path );
	void setMaxSize( uint32_t mb );
	void setOverwritingEnabled( bool enabled );
	void setOnlyNewDigitSignal( bool enabled );
	void setDigitSignalPeriod( uint32_t msec );
	void setAnalogSignalPeriod( uint32_t msec );
	void setSignalBlockDescList( const std::list< SignalBlockDesc >& list );
	void setSignalProvider( AbstractSRSensor::SignalProvider* signalProvider );

	bool clean();

	bool startLogging( tprio_t prio = NORMALPRIO );
	void stopLogging();
	bool waitForStop( sysinterval_t timeout = TIME_INFINITE );
	State state();
	uint64_t size();

	void updateSensor( AbstractSensor* sensor, const std::string* name = nullptr );

private:
	void main() final override;
	void logDigitInputs();
	void logAnalogInputs();
	void logSensorData();
	bool free( uint64_t size );

	static void dTimerCallback( void* p );
	static void aTimerCallback( void* p );

private:
	enum : eventmask_t { StopRequestEvent = 1, CleanRequestEvent = 2, DigitSignalTimerEvent = 4, AnalogSignalTimerEvent = 8, PendingSensorEvent = 16 };
	State logState;
	threads_queue_t waitingQueue;
	uint8_t buffer[512 + 18]; // > 128

	Dir rootDir;
	uint64_t maxSize;
	uint64_t logSize;
	volatile bool overwritingEnabled, onlyNewDigitSignal;
	sysinterval_t dSignalPeriod;
	sysinterval_t aSignalPeriod;
	struct SBlockDesc
	{
		SBlockDesc() {}
		SBlockDesc( SignalBlockDesc desc ) : analogCount( desc.analogCount ), digitCount( desc.digitCount ), digitBlockData( 0 ) {}
		uint16_t analogCount;
		uint16_t digitCount;
		uint32_t digitBlockData;
	};
	std::vector< SBlockDesc > blockDescVect;
	uint16_t digitBlocksCount;
	uint16_t analogBlocksCount;
	uint16_t analogChannelsCount;
	AbstractSRSensor::SignalProvider* signalProvider;

	File file;
	Mutex mutex;
	struct SensorDesc
	{
		SensorDesc( AbstractSensor* sensor, const std::string* name ) : sensor( sensor ), name( name ) {}
		AbstractSensor* sensor;
		const std::string* name;
	};
	std::list< SensorDesc > pendingSensors;
	uint64_t pendingFlags[4];
};