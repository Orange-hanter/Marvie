#include "SensorService.h"
#include "Core/Assert.h"
#include <string.h>

NanoList< BRSensorService::SensorTypeDesc > BRSensorService::list;
BRSensorService* BRSensorService::service = nullptr;

BRSensorService::BRSensorService()
{
	assert( list.size() != 0 );
	_last = list.begin();
}

BRSensorService::~BRSensorService()
{

}

void BRSensorService::registerSensorType( Node* node )
{
	list.pushBack( node );
}

BRSensorService* BRSensorService::instance()
{
	if( service )
		return service;
	service = new BRSensorService;
	return service;
}

const BRSensorService::SensorTypeDesc* BRSensorService::sensorTypeDesc( const char* sensorName )
{
	return &findByName( sensorName )->value;
}

AbstractBRSensor* BRSensorService::allocate( const char* sensorName )
{
	Node* node = findByName( sensorName );
	return node->value.allocator();
}

bool BRSensorService::tune( AbstractBRSensor* sensor, tinyxml2::XMLElement* element )
{
	Node* node = findByName( sensor->name() );
	if( !node )
		return false;
	node->value.tuner( sensor, element );
	return true;
}

BRSensorService::Node* BRSensorService::findByName( const char* sensorName )
{
	if( strcmp( _last->value.name, sensorName ) == 0 )
		return _last;
	auto end = list.end();
	for( auto i = list.begin(); i != end; ++i )
	{
		if( strcmp( ( *i ).name, sensorName ) == 0 )
		{
			_last = i;
			return _last;
		}
	}

	return nullptr;
}