#include "SensorService.h"
#include "Drivers/Sensors/TestSensorB.h"

using namespace tinyxml2;

static AbstractSensor* testSensorBAllocate()
{
	return new TestSensorB;
}

static bool testSensorBTune( AbstractSensor* sensor, XMLElement* e, uint32_t defaultBaudrate )
{
	XMLElement* c0 = e->FirstChildElement( "baudrate" );
	if( c0 )
		defaultBaudrate = c0->UnsignedText( defaultBaudrate );
	static_cast< TestSensorB* >( sensor )->setBaudrate( defaultBaudrate );

	c0 = e->FirstChildElement( "text" );
	if( !c0 )
		return false;
	static_cast< TestSensorB* >( sensor )->setTextMessage( c0->GetText() );

	c0 = e->FirstChildElement( "goodNum" );
	int goodNum;
	if( !c0 || c0->QueryIntText( &goodNum ) != XML_SUCCESS )
		return false;
	static_cast< TestSensorB* >( sensor )->setGoodNum( ( uint32_t )goodNum );

	c0 = e->FirstChildElement( "badNum" );
	int badNum;
	if( !c0 || c0->QueryIntText( &badNum ) != XML_SUCCESS )
		return false;
	static_cast< TestSensorB* >( sensor )->setBadNum( ( uint32_t )badNum );

	c0 = e->FirstChildElement( "errType" );
	if( !c0 )
		return false;
	static_cast< TestSensorB* >( sensor )->setErrorType( strcmp( c0->GetText(), "NoResp" ) == 0 ? TestSensorB::ErrorType::NoResp : TestSensorB::ErrorType::Crc );

	c0 = e->FirstChildElement( "errCode" );
	int errCode;
	if( !c0 || c0->QueryIntText( &errCode ) != XML_SUCCESS )
		return false;
	static_cast< TestSensorB* >( sensor )->setErrorCode( ( uint8_t )errCode );

	return true;
}

static SensorService::Node* simpleSensorType = []()
{
	static SensorService::Node node;
	node.value.type = AbstractSensor::Type::BR;
	node.value.name = TestSensorB::sName();
	node.value.allocator = testSensorBAllocate;
	node.value.tuner = testSensorBTune;
	SensorService::registerSensorType( &node );
	return &node;
}();