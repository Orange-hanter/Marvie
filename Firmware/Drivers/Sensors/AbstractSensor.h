#pragma once

#include "Core/DateTime.h"
#include "Core/IODevice.h"

enum class SensorEventFlag { DataUpdated = 1 };

class SensorData
{
public:
	enum class Error : uint8_t { NoError, NoResponseError, CrcError };

	SensorData() { errType = Error::NoError; errCode = 0; chMtxObjectInit( &mutex ); }

	inline DateTime time() { return t; }
	inline bool isValid() { return errType == Error::NoError; }
	inline Error error() { return errType; }
	inline uint8_t errorCode() { return errCode; }
	inline bool tryLock() { return chMtxTryLock( &mutex ); }
	inline void lock() { chMtxLock( &mutex ); }
	inline void unlock() { chMtxUnlock( &mutex ); }

protected:
	DateTime t;
	Error errType;
	uint8_t errCode;

private:
	mutex_t mutex;
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
		virtual bool binarySignal( uint32_t block, uint32_t line ) = 0;
	};

	Type type() final override;
	virtual void setInputSignalProvider( SignalProvider* ) = 0;
	virtual SignalProvider* inputSignalProvider() = 0;
};

class AbstractBRSensor : public AbstractSensor // Byte related sensor
{
public:
	Type type() final override;
	virtual void setIODevice( IODevice* ) = 0;
	virtual IODevice* ioDevice() = 0;

protected:
	static ByteRingIterator waitForResponse( IODevice* io, const char* response, uint32_t responseLen, sysinterval_t timeout );

private:
	static void timerCallback( void* );
};