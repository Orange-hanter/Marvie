#pragma once

#include "Core/NanoList.h"
#include "Drivers/Sensors/AbstractSensor.h"
#include "Lib/tinyxml2/tinyxml2.h"

class BRSensorService
{
	BRSensorService();
	~BRSensorService();

public:
	struct SensorTypeDesc
	{
		const char* name;
		AbstractBRSensor* ( *allocator )( );
		bool( *tuner )( AbstractBRSensor*, tinyxml2::XMLElement* );
	};
	typedef NanoList< SensorTypeDesc >::Node Node;

	static void registerSensorType( Node* node );
	static BRSensorService* instance();

	inline const SensorTypeDesc* sensorTypeDesc( const char* sensorName );
	AbstractBRSensor* allocate( const char* sensorName );
	bool tune( AbstractBRSensor*, tinyxml2::XMLElement* );

private:
	Node* findByName( const char* sensorName );

private:
	static NanoList< SensorTypeDesc > list;
	static BRSensorService* service;
	Node* _last;
};