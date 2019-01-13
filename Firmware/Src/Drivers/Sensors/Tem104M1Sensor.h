#pragma once

#include "AbstractSensor.h"
#include "Drivers/Interfaces/UsartBasedDevice.h"

class Tem104M1Sensor : public AbstractBRSensor
{
public:
	static const char Name[];
	class Data : public SensorData
	{
		friend class Tem104M1Sensor;
	public:
		Data()
		{
			constexpr int size = ( sizeof( Data ) - sizeof( SensorData ) ) / 4;
			for( int i = 0; i < size; ++i )
				reinterpret_cast< uint32_t* >( &instant )[i] = 0;
		}
		struct InstantValues
		{
			float rshv;		    // текущее значение объёмного расхода
			float rshm;		    // текущее значение массового расхода
			float tmp[2];	    // текущее значение температуры по каналам
			float prs[2];	    // текущее значение давления по каналам
		} instant;
		struct IntegratedValues
		{
			double intV;	    // Интегратор объема по каналу
			double intM;	    // Интегратор массы по каналу
			double intQ;	    // Интегратор энергии по каналу
			uint32_t T_Rab;     // общее время работы
			uint32_t T_offline;	// время наработки без ошибок
			uint32_t T_Fail;	// время нахождения в ошибках
			uint32_t T_Dt;		// время в ошибке dT < dTmin
			uint32_t T_Gmax;	// время нахождения в ошибке G > Gmax 
			uint32_t T_Gmin;	// время нахождения в ошибке G < Gmin
			uint16_t tmp[2];	// температуры
			uint8_t prs[2];		// давления
			uint8_t TekErr;		// текущая ошибка 1
			uint8_t TehErr;		// текущая ошибка 2
		} integrated;
	};

	explicit Tem104M1Sensor();
	virtual ~Tem104M1Sensor();

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
	uint8_t address;
	uint32_t baudrate;
	Data data;
};
