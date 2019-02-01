#include "Term0208Sensor.h"
#include "Core/DateTimeService.h"

const char Term0208Sensor::Name[] = "TERM0208";

Term0208Sensor::Term0208Sensor()
{
	address = 0;
	baudrate = 9600;
	modbusClient.setFrameType( ModbusDevice::FrameType::Ascii );
}

Term0208Sensor::~Term0208Sensor()
{
}

const char* Term0208Sensor::name() const
{
	return Name;
}

void Term0208Sensor::setAddress( uint8_t address )
{
	this->address = address;
}

void Term0208Sensor::setBaudrate( uint32_t baudrate )
{
	this->baudrate = baudrate;
}

Term0208Sensor::Data* Term0208Sensor::readData()
{
	io->acquireDevice();
#define returnError( err, code )                  \
	{                                             \
		data.lock();                              \
		data.errType = err;                       \
		data.errCode = code;                      \
		data.unlock();                            \
		io->releaseDevice();                      \
		io->read( nullptr, io->readAvailable() ); \
		return &data;                             \
	}

	if( io->isSerialDevice() )
	{
		UsartBasedDevice* serialDevice = static_cast< UsartBasedDevice* >( io );
		serialDevice->setDataFormat( UsartBasedDevice::B8N );
		serialDevice->setStopBits( UsartBasedDevice::S1 );
		serialDevice->setBaudRate( baudrate );
	}
	io->read( nullptr, io->readAvailable() );
	modbusClient.setIODevice( io );

	// read instant values
	if( !modbusClient.readInputRegisters( address, 0x2000, 0x1A, reinterpret_cast< uint16_t* >( buffer ) ) )
		returnError( ( modbusClient.error() == ModbusDevice::Error::TimeoutError ? SensorData::Error::NoResponseError : SensorData::Error::CrcError ), 1 );
	parseInstantValuesResponse();

	// read inegrated values
    if( !modbusClient.readInputRegisters( address, 0x3000, 0x60, reinterpret_cast< uint16_t* >( buffer ) ) )
		returnError( ( modbusClient.error() == ModbusDevice::Error::TimeoutError ? SensorData::Error::NoResponseError : SensorData::Error::CrcError ), 2 );
    if( !modbusClient.readInputRegisters( address, 0x3070, 0x0C, reinterpret_cast< uint16_t* >( buffer ) + 0x60 ) )
		returnError( ( modbusClient.error() == ModbusDevice::Error::TimeoutError ? SensorData::Error::NoResponseError : SensorData::Error::CrcError ), 3 );
	parseIntegratedValuesResponse();

	data.lock();
	data.t = DateTimeService::currentDateTime();
	data.errType = SensorData::Error::NoError;
	data.unlock();
	io->read( nullptr, io->readAvailable() );

	io->releaseDevice();
	return &data;
}

Term0208Sensor::Data* Term0208Sensor::sensorData()
{
	return &data;
}

uint32_t Term0208Sensor::sensorDataSize()
{
	return sizeof( Data ) - sizeof( SensorData );
}

void Term0208Sensor::parseInstantValuesResponse()
{
	struct __Instant
	{
		int32_t G[3];
		int32_t M[3];
		int32_t Q[2];
		int16_t t[4];
		int16_t p[4];
		uint8_t err[4];
	} *__inst = reinterpret_cast< __Instant* >( buffer );

	data.lock();
	data.instant.G[0] = float( __inst->G[0] ) * 1e-9f;
	data.instant.G[1] = float( __inst->G[1] ) * 1e-9f;
	data.instant.G[2] = float( __inst->G[2] ) * 1e-9f;
	data.instant.M[0] = float( __inst->M[0] ) * 1e-6f;
	data.instant.M[1] = float( __inst->M[1] ) * 1e-6f;
	data.instant.M[2] = float( __inst->M[2] ) * 1e-6f;
	data.instant.Q[0] = __inst->Q[0];
	data.instant.Q[1] = __inst->Q[1];
	for( int i = 0; i < 4; ++i )
	{
		data.instant.t[i] = float( __inst->t[i] ) * 1e-2f;
		data.instant.p[i] = float( __inst->p[i] ) * 1e-3f;
		data.instant.err[i] = __inst->err[i];
	}
	data.unlock();
}

void Term0208Sensor::parseIntegratedValuesResponse()
{
	struct __Integrated
	{
		uint64_t V1;
		uint64_t M1;
		uint64_t V2;
		uint64_t M2;
		uint64_t V3;
		uint64_t M3;

		uint64_t Q1;
		uint64_t Q2;

		uint64_t dV1_h;
		uint64_t dM1_h;
		uint64_t dV2_h;
		uint64_t dM2_h;
		uint64_t dV3_h;
		uint64_t dM3_h;

		uint64_t dQ1_h;
		uint64_t dQ2_h;

		uint64_t dV1_d;
		uint64_t dM1_d;
		uint64_t dV2_d;
		uint64_t dM2_d;
		uint64_t dV3_d;
		uint64_t dM3_d;

		uint64_t dQ1_d;
		uint64_t dQ2_d;

		uint32_t T1rab;
		uint32_t T1err;

		uint32_t T2rab;
		uint32_t T2err;

		uint8_t errFlags_h[4];
		uint8_t errFlags_d[4];
	} *__integr = reinterpret_cast< __Integrated* >( buffer );

	data.lock();
	data.integrated.V[0] = double( __integr->V1 ) * 1e-9;
	data.integrated.V[1] = double( __integr->V2 ) * 1e-9;
	data.integrated.V[2] = double( __integr->V3 ) * 1e-9;
	data.integrated.M[0] = double( __integr->M1 ) * 1e-6;
	data.integrated.M[1] = double( __integr->M2 ) * 1e-6;
	data.integrated.M[2] = double( __integr->M3 ) * 1e-6;
	data.integrated.Q[0] = __integr->Q1;
	data.integrated.Q[1] = __integr->Q2;
	data.integrated.timeWork[0] = __integr->T1rab;
	data.integrated.timeWork[1] = __integr->T2rab;
	data.integrated.timeErr[0] = __integr->T1err;
	data.integrated.timeErr[1] = __integr->T2err;

	data.integrated.perHour.V[0] = double( __integr->dV1_h ) * 1e-9;
	data.integrated.perHour.V[1] = double( __integr->dV2_h ) * 1e-9;
	data.integrated.perHour.V[2] = double( __integr->dV3_h ) * 1e-9;
	data.integrated.perHour.M[0] = double( __integr->dM1_h ) * 1e-6;
	data.integrated.perHour.M[1] = double( __integr->dM2_h ) * 1e-6;
	data.integrated.perHour.M[2] = double( __integr->dM3_h ) * 1e-6;
	data.integrated.perHour.Q[0] = __integr->dQ1_h;
	data.integrated.perHour.Q[1] = __integr->dQ2_h;
	data.integrated.perHour.err[0] = __integr->errFlags_h[0];
	data.integrated.perHour.err[1] = __integr->errFlags_h[1];
	data.integrated.perHour.err[2] = __integr->errFlags_h[2];
	data.integrated.perHour.err[3] = __integr->errFlags_h[3];

	data.integrated.perDay.V[0] = double( __integr->dV1_d ) * 1e-9;
	data.integrated.perDay.V[1] = double( __integr->dV2_d ) * 1e-9;
	data.integrated.perDay.V[2] = double( __integr->dV3_d ) * 1e-9;
	data.integrated.perDay.M[0] = double( __integr->dM1_d ) * 1e-6;
	data.integrated.perDay.M[1] = double( __integr->dM2_d ) * 1e-6;
	data.integrated.perDay.M[2] = double( __integr->dM3_d ) * 1e-6;
	data.integrated.perDay.Q[0] = __integr->dQ1_d;
	data.integrated.perDay.Q[1] = __integr->dQ2_d;
	data.integrated.perDay.err[0] = __integr->errFlags_d[0];
	data.integrated.perDay.err[1] = __integr->errFlags_d[1];
	data.integrated.perDay.err[2] = __integr->errFlags_d[2];
	data.integrated.perDay.err[3] = __integr->errFlags_d[3];
	data.unlock();
}