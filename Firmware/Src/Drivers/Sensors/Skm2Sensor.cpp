#include "Skm2Sensor.h"
#include "Core/DateTimeService.h"

const char SKM2Sensor::Name[] = "SKM2";

SKM2Sensor::SKM2Sensor()
{
	address = 0;
	baudrate = 9600;
}

SKM2Sensor ::~SKM2Sensor()
{

}

const char* SKM2Sensor::name() const
{
	return this->Name;
}

void SKM2Sensor::setAddress( uint8_t address )
{
	this->address = address;
}

void SKM2Sensor::setBaudrate( uint32_t baudrate )
{
	this->baudrate = baudrate;
}

SKM2Sensor::Data* SKM2Sensor::readData()
{
	io->acquireDevice();
	
#define returnIfError( err, code ) \
{ \
	if( err != SensorData::Error::NoError ) \
	{ \
		data.lock(); \
		data.errType = err; \
		data.errCode = code; \
		data.unlock(); \
		io->releaseDevice(); \
		io->read( nullptr, io->readAvailable() ); \
		return &data; \
	} \
}
	if( io->isSerialDevice() )
	{
		UsartBasedDevice* serialDevice = static_cast< UsartBasedDevice* >( io );
		serialDevice->setDataFormat( UsartBasedDevice::B8N );
		serialDevice->setStopBits( UsartBasedDevice::S1 );
		serialDevice->setBaudRate( baudrate );
	}
	io->read( nullptr, io->readAvailable() );

	modbusMaster.setIODevice( io );
	modbusMaster.setFrameType( ModbusDevice::FrameType::Rtu );

	//read data
	auto err = modbusMaster.readInputRegisters( address, 0x8002, 2, ( uint16_t* )&data.sysInfo.sysTime );
	data.sysInfo.sysTime++;
	err = modbusMaster.writeMultipleRegisters( address, 0x8004, 2, ( uint16_t* )&data.sysInfo.sysTime );
	io->releaseDevice();
	return &data;
}

SKM2Sensor::Data * SKM2Sensor::sensorData()
{
	return &data;
}

uint32_t SKM2Sensor::sensorDataSize()
{
	return sizeof( Data ) - sizeof( SensorData );
}
