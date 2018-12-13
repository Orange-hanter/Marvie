#include "SensorService.h"
#include "Drivers/Sensors/TestSensorA.h"

using namespace tinyxml2;

static AbstractSensor* testSensorAAllocate()
{
	return new TestSensorA;
}

static bool testSensorATune( AbstractSensor* sensor, XMLElement* e, uint32_t defaultBaudrate )
{
	XMLElement* c0 = e->FirstChildElement( "baudrate" );
	if( c0 )
		defaultBaudrate = c0->UnsignedText( defaultBaudrate );
	static_cast< TestSensorA* >( sensor )->setBaudrate( defaultBaudrate );

	c0 = e->FirstChildElement( "text" );
	if( !c0 )
		return false;
	static_cast< TestSensorA* >( sensor )->setTextMessage( c0->GetText() );

	c0 = e->FirstChildElement( "delayMs" );
	int delay;
	if( !c0 || c0->QueryIntText( &delay ) != XML_SUCCESS )
		return false;
	static_cast< TestSensorA* >( sensor )->setDelay( ( uint32_t )delay );

	c0 = e->FirstChildElement( "cpuLoad" );
	int cpuLoad;
	if( !c0 || c0->QueryIntText( &cpuLoad ) != XML_SUCCESS )
		return false;
	static_cast< TestSensorA* >( sensor )->setCpuLoad( ( uint8_t )cpuLoad );

	return true;
}

static SensorService::Node* simpleSensorType = []()
{
	static SensorService::Node node;
	node.value.type = AbstractSensor::Type::BR;
	node.value.name = TestSensorA::sName();
	node.value.allocator = testSensorAAllocate;
	node.value.tuner = testSensorATune;
	SensorService::registerSensorType( &node );
	return &node;
}();