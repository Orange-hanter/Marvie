#include "SensorService.h"
#include "Core/Assert.h"
#include <string.h>

NanoList< SensorService::SensorTypeDesc >* SensorService::list;
SensorService* SensorService::service = nullptr;

SensorService::SensorService()
{
	assert( list && list->size() != 0 );
	_last = list->begin();
}

SensorService::~SensorService()
{

}

bool SensorService::registerSensorType( Node* node )
{
	static NanoList< SensorService::SensorTypeDesc > _list;
	list = &_list;

	if( strlen( node->value.name ) > 64 || strstr( node->value.name, "," ) != nullptr )
		return false;
	
	list->pushBack( node );
	return true;
}

uint32_t SensorService::sensorsCount()
{
	return list->size();
}

NanoList< SensorService::SensorTypeDesc >::Iterator SensorService::beginSensorsList()
{
	return list->begin();
}

NanoList< SensorService::SensorTypeDesc >::Iterator SensorService::endSensorsList()
{
	return list->end();
}

SensorService* SensorService::instance()
{
	if( service )
		return service;
	service = new SensorService;
	return service;
}

AbstractSensor* SensorService::allocate( const char* sensorName )
{
	Node* node = findByName( sensorName );
	return node->value.allocator();
}

bool SensorService::tune( AbstractSensor* sensor, tinyxml2::XMLElement* element, uint32_t defaultBaudrate )
{
	Node* node = findByName( sensor->name() );
	if( !node )
		return false;
	node->value.tuner( sensor, element, defaultBaudrate );
	return true;
}

SensorService::Node* SensorService::findByName( const char* sensorName )
{
	if( strcmp( _last->value.name, sensorName ) == 0 )
		return _last;
	auto end = list->end();
	for( auto i = list->begin(); i != end; ++i )
	{
		if( strcmp( ( *i ).name, sensorName ) == 0 )
		{
			_last = i;
			return _last;
		}
	}

	return nullptr;
}