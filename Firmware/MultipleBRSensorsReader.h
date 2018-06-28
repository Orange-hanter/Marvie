#pragma once

#include "Core/BaseDynamicThread.h"
#include "Core/NanoList.h"
#include "BRSensorReader.h"
#include "chmempools.h"

#define MULTIPLE_BR_SENSORS_READER_STACK_SIZE    192 // 192B min stack size

class MultipleBRSensorsReader : private BaseDynamicThread, public BRSensorReader
{
public:
	struct SensorDesc
	{
		SensorDesc();

		AbstractBRSensor* sensor;
		sysinterval_t normalPriod;
		sysinterval_t emergencyPeriod;
	};
	typedef NanoList< SensorDesc >::Node SensorNode;
	class SensorElement : public SensorNode
	{
		friend class MultipleBRSensorsReader;

		SensorElement();
		~SensorElement() {}

		inline void* operator new( size_t size, void* where );

		sysinterval_t delta;
		SensorElement* next;
	};

	MultipleBRSensorsReader();
	~MultipleBRSensorsReader();

	static SensorElement* createSensorElement();
	static SensorElement* createSensorElement( AbstractBRSensor* sensor, sysinterval_t normalPriod, sysinterval_t emergencyPeriod );
	static inline void deleteSensorElement( SensorElement* e );

	void addSensorElement( SensorElement* sensorElement );
	void removeAllSensorElements();
	void moveAllSensorElementsTo( NanoList< SensorDesc >& list );

	void setMinInterval( sysinterval_t minInterval ); // Minimal interval between readings

	void startReading( tprio_t prio ) final override;
	void stopReading() final override;

	AbstractBRSensor* nextUpdatedSensor();
	AbstractBRSensor* nextSensor();

private:
	void main() final override;
	inline void prepareList();
	void insert( SensorNode* node, sysinterval_t period );
	inline void addTimeError( sysinterval_t dt );

	static SensorElement* alloc();
	static inline void free( void* );
	static void timerCallback( void* );

private:
	static memory_pool_t objectPool;

	sysinterval_t minInterval;
	sysinterval_t sumTimeError;
	NanoList< SensorDesc > elist;
	AbstractBRSensor* _nextSensor;
	SensorElement* updatedSensorsRoot;
};