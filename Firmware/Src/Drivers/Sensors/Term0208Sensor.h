#pragma once

#include "AbstractSensor.h"
#include "Drivers/Interfaces/UsartBasedDevice.h"
#include "Apps/Modbus/RawModbusClient.h"

class Term0208Sensor : public AbstractBRSensor
{
public:
	static const char Name[];
	class Data : public SensorData
	{
		friend class Term0208Sensor;
	public:
		Data()
		{
			constexpr size_t size = ( sizeof( Data ) - sizeof( SensorData ) ) / 4;
			for( size_t i = 0; i < size; ++i )
				reinterpret_cast< uint32_t* >( &instant )[i] = 0;
		}
		struct InstantValues
		{
			float G[3];   // m^3/sec
			int32_t M[3]; // ton/sec
			int32_t Q[2]; // J/sec
			float t[4];   // °C
			float p[4];   // MPa
			uint8_t err[4];
			uint32_t _reserved;
		} instant;
		struct IntegratedValues
		{
			double V[3];	      // m^3
			double M[3];	      // ton
			uint64_t Q[2];	      // J
			uint32_t timeWork[2]; // sec
			uint32_t timeErr[2];  // sec
			struct PerHour
			{
				double V[3];   // m^3
				double M[3];   // ton
				uint64_t Q[2]; // J
				uint8_t err[4];
				uint32_t _reserved;

				/*float avgT[5];
				float avgP[4];
				uint32_t tWork[2];
				uint16_t tOut[2];
				uint16_t tTmin[2];
				uint16_t tGmin[2];
				uint16_t tGmax[2];
				uint16_t tDt[2];
				uint16_t tDop[2];
				uint16_t tRsrv[2];*/
			} perHour;
			struct PerDay
			{
				double V[3];   // m^3
				double M[3];   // ton
				uint64_t Q[2]; // J
				uint8_t err[4];
				uint32_t _reserved;

				/*float avgT[5];
				float avgP[4];
				uint32_t tWork[2];
				uint32_t tOut[2];
				uint32_t tTmin[2];
				uint32_t tGmin[2];
				uint32_t tGmax[2];
				uint32_t tDt[2];
				uint32_t tDop[2];
				uint32_t tRsrv[2];*/
			} perDay;
		} integrated;
	};

	explicit Term0208Sensor();
	virtual ~Term0208Sensor();

	const char* name() const final override;
	void setAddress( uint8_t address );
	void setBaudrate( uint32_t baudrate );

	Data* readData() final override;
	Data* sensorData() final override;
	uint32_t sensorDataSize() final override;

private:
	inline void parseInstantValuesResponse();
    inline void parseIntegratedValuesResponse();

private:
	uint8_t address;
	uint32_t baudrate;
    RawModbusClient modbusClient;
    uint8_t buffer[( 0x60 + 0x0C ) * 2];
    int gg[5] = {};
	Data data;
};