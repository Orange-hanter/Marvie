#pragma once

#include "Core/DataTime.h"
#include "Core/IODevice.h"

enum class SensorEventFlag { DataUpdated = 1 };

class SensorData
{
public:
	SensorData() { valid = false; chMtxObjectInit( &mutex ); }

	inline DataTime time() { return t; }
	inline bool isValid() { return valid; }
	inline bool tryLock() { return chMtxTryLock( &mutex ); }
	inline void lock() { chMtxLock( &mutex ); }
	inline void unlock() { chMtxUnlock( &mutex ); }

protected:
	DataTime t;
	bool valid;

private:
	mutex_t mutex;
};

class AbstractSensor
{
public:
	virtual const char* name() const = 0;
	virtual SensorData* readData() = 0;
	virtual SensorData* sensorData() = 0;
};

class AbstractBRSensor : public AbstractSensor // Byte related sensor
{
public:
	virtual void setIODevice( IODevice* ) = 0;
	virtual IODevice* ioDevice() = 0;

protected:
	static ByteRingIterator waitForResponse( IODevice* io, const char* response, uint32_t responseLen, sysinterval_t timeout );

private:
	static void timerCallback( void* );
};

//class AbstractSRSensor : public AbstractSensor // Signal related sensor
//{
//public:
//	AbstractSRSensor( float* analogInputs, uint16_t* discreteInputs );
//
//	inline bool dInput( uint32_t index ) { return ( discreteInputs[index >> 4] >> ( index & 0xFF ) ) & 0x01; }
//	inline float aInput( uint32_t index ) { return analogInputs[index]; }
//
//private:
//	float* analogInputs;
//	uint16_t* discreteInputs;
//};