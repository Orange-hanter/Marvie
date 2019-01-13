#pragma once

#include "AbstractSensor.h"
#include "Drivers/Interfaces/UsartBasedDevice.h"
#include "Apps/Modbus/RawModbusClient.h"


class SKM2Sensor : public AbstractBRSensor
{
public:
	static const char Name[];
	class Data : public SensorData
	{
		friend class Tem104USensor;
	public:
		Data()
		{
			constexpr int size = ( sizeof( Data ) - sizeof( SensorData ) ) / 4;
			for( int i = 0; i < size; ++i )
				reinterpret_cast< uint32_t* >( &sysInfo.deviceId )[i] = 0;
		}
		struct CounterSysInfo
		{
			uint32_t deviceId;
			uint32_t sysTime;
		} sysInfo;
		struct InstantValues
		{
			float rshv;		    // текущее значение объёмного расхода
			float rshm;		    // текущее значение массового расхода
			float tmp;	    // текущее значение температуры по каналам
			float prs;	    // текущее значение давления по каналам
		} instant[4];
		struct IntegratedValues
		{
			double intV;	    // Интегратор объема по каналу
			double intM;	    // Интегратор массы по каналу
			double intQ;	    // Интегратор энергии по каналу

		} integrated[4];
	};

	explicit SKM2Sensor();
	virtual ~SKM2Sensor();

	const char* name() const final override;
	void setAddress( uint8_t address );
	void setBaudrate( uint32_t baudrate );

	Data* readData() final override;
	Data* sensorData() final override;
	uint32_t sensorDataSize() final override;

private:
	SensorData::Error waitResponse( uint32_t size );
	void parseTimerResponse();
	void parseRamResponse();
	template < typename T > uint8_t checksum( T, T ) const;

private:
	RawModbusClient modbusMaster;
	uint8_t address;
	uint32_t baudrate;
	Data data;
};