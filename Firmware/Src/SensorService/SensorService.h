#pragma once

#include "Core/NanoList.h"
#include "Drivers/Sensors/AbstractSensor.h"
#include "Lib/tinyxml2/tinyxml2.h"

class SensorService
{
	SensorService();
	~SensorService();

public:
	struct SensorTypeDesc
	{
		AbstractSensor::Type type;
		const char* name;
		AbstractSensor* ( *allocator )();
		bool( *tuner )( AbstractSensor*, tinyxml2::XMLElement*, uint32_t defaultValue );
	};
	typedef NanoList< SensorTypeDesc >::Node Node;

	static bool registerSensorType( Node* node );
	static uint32_t sensorsCount();
	static NanoList< SensorTypeDesc >::Iterator beginSensorsList();
	static NanoList< SensorTypeDesc >::Iterator endSensorsList();
	static SensorService* instance();

	inline bool contains( const char* sensorName )
	{
		return findByName( sensorName ) != nullptr;
	}
	inline const SensorTypeDesc* sensorTypeDesc( const char* sensorName )
	{
		const auto node = findByName( sensorName );
		if( node )
			return &node->value;
		return nullptr;
	}
	inline AbstractSensor::Type sensorType( const char* sensorName )
	{
		return findByName( sensorName )->value.type;
	}
	AbstractSensor* allocate( const char* sensorName );
	bool tune( AbstractSensor*, tinyxml2::XMLElement*, uint32_t defaultValue = 0 );

private:
	Node* findByName( const char* sensorName );

private:
	static NanoList< SensorTypeDesc >* list;
	static SensorService* service;
	Node* _last;
};