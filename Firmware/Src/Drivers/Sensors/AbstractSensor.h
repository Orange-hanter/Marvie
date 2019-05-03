#pragma once

#include "Core/DateTime.h"
#include "Core/IODevice.h"
#include "Core/Mutex.h"

enum class SensorEventFlag { DataUpdated = 1 };

class SensorData
{
public:
	enum class Error : uint8_t { NoError, NoResponseError, CrcError/*, PackageError*/ };

	SensorData() { errType = Error::NoError; errCode = 0; }

	inline DateTime time() { return t; }
	inline bool isValid() { return errType == Error::NoError; }
	inline Error error() { return errType; }
	inline uint8_t errorCode() { return errCode; }
	inline bool tryLock() { return mutex.tryLock(); }
	inline void lock() { mutex.lock(); }
	inline void unlock() { mutex.unlock(); }

protected:
	DateTime t;
	Error errType;
	uint8_t errCode;

private:
	Mutex mutex;
};

class AbstractSensor
{
public:
	AbstractSensor() { _userData = 0; }
	virtual ~AbstractSensor() {}

	enum class Type { SR, BR };

	virtual Type type() = 0;
	virtual const char* name() const = 0;
	virtual SensorData* readData() = 0;
	virtual SensorData* sensorData() = 0;
	virtual uint32_t sensorDataSize() = 0;

	inline uint32_t userData() { return _userData; }
	inline void setUserData( uint32_t data ) { _userData = data; }

private:
	uint32_t _userData;
};

class AbstractSRSensor : public AbstractSensor // Signal related sensor
{
public:
	class SignalProvider
	{
	public:
		virtual float analogSignal( uint32_t block, uint32_t line ) = 0;
		virtual bool digitSignal( uint32_t block, uint32_t line ) = 0;
		virtual uint32_t digitSignals( uint32_t block ) = 0;
	};

	AbstractSRSensor();

	Type type() final override;
	void setInputSignalProvider( SignalProvider* );
	SignalProvider* inputSignalProvider();

protected:
	SignalProvider* signalProvider;
};

class AbstractBRSensor : public AbstractSensor // Byte related sensor
{
public:
	AbstractBRSensor();

	Type type() final override;
	virtual void setIODevice( IODevice* );
	IODevice* ioDevice();

protected:
	void sleep( sysinterval_t timeout );
	ByteRingIterator waitForResponse( const char* response, uint32_t responseLen, sysinterval_t timeout );
	bool waitForReadAvailable( uint32_t size, sysinterval_t timeout );

private:
	static void timerCallback( void* );

protected:
	IODevice* io;
};