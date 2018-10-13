#pragma once

#include "AbstractSensor.h"
#include "Drivers/Interfaces/UsartBasedDevice.h"

class Tem104Sensor : public AbstractBRSensor
{
public:
	static const char Name[];
	class Data : public SensorData
	{
		friend class Tem104Sensor;
	public:
		Data()
		{
			constexpr int size = ( sizeof( Data ) - sizeof( SensorData ) ) / 4;
			for( int i = 0; i < size; ++i )
				reinterpret_cast< uint32_t* >( &instant )[i] = 0;
		}
		struct InstantValues
		{
			struct Ch
			{
				float tmp[4];       // Текущие значения температуры по каналам
				float prs[4];       // Текущие значения давления по каналам
				float ro[4];        // Текущие значения плотности теплоносителя
				float hent[4];      // Текущие значения энтальпии
				float rshv[4];      // Текущие значения объемного расхода
				float rshm[4];      // Текущие значения массового расхода
				float pwr[4];       // Текущие значения энергии
				uint8_t tekerr;     // Текущие Ошибки
				uint8_t reserved;
				uint16_t teherr;    // Текущие Ошибки
			} ch[4];
		} instant;
		struct IntegratedValues
		{
			uint8_t date[4];        // Время и дата записи (ЧЧ ДД ММ ГГ)
			uint8_t prev_date[4];   // Время и дата предыдущей записи( ЧЧ ДД ММ ГГ )
			uint32_t TRab;          // время работы прибора при поданном питании
			struct Ch
			{
				double intV;	    // Интегратор объема по каналу
				double intM;	    // Интегратор массы по каналу
				double intQ;	    // Интегратор энергии по каналу
				uint32_t TNar;      // время работы систем без ошибок
				uint32_t Tmin;      // расход меньше минимального
				uint32_t Tmax;      // расход больше максимального
				uint32_t Tdt;       // разность температур меньше минимальной
				uint32_t Ttn;       // техническая неисправность
				uint16_t teherr;    // Ошибки по системам
				uint8_t tekerr;     // Ошибки по системам
				uint8_t reserved;
				uint16_t t[3];      // Температура по системам
				uint16_t reserved2;
				uint8_t p[3];       // Давление по системам
				uint8_t reserved3;
				float rshv;         // Интегратор объемного расхода
			} ch[4];
		} integrated;
	};

	explicit Tem104Sensor();
	virtual ~Tem104Sensor();

	const char* name() const final override;
	void setAddress( uint8_t address );
	void setBaudrate( uint32_t baudrate );

	Data* readData() final override;
	Data* sensorData() final override;
	uint32_t sensorDataSize() final override;

private:
	SensorData::Error waitResponse();
	void parseTimerResponse();
	void parseRamResponse( uint32_t ch );
	template < typename T > uint8_t checksum( T, T ) const;

private:
	uint8_t address;
	uint32_t baudrate;
	Data data;
};
