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
				float tmp[4];       // ������� �������� ����������� �� �������
				float prs[4];       // ������� �������� �������� �� �������
				float ro[4];        // ������� �������� ��������� �������������
				float hent[4];      // ������� �������� ���������
				float rshv[4];      // ������� �������� ��������� �������
				float rshm[4];      // ������� �������� ��������� �������
				float pwr[4];       // ������� �������� �������
				uint8_t tekerr;     // ������� ������
				uint8_t reserved;
				uint16_t teherr;    // ������� ������
			} ch[4];
		} instant;
		struct IntegratedValues
		{
			uint8_t date[4];        // ����� � ���� ������ (�� �� �� ��)
			uint8_t prev_date[4];   // ����� � ���� ���������� ������( �� �� �� �� )
			uint32_t TRab;          // ����� ������ ������� ��� �������� �������
			struct Ch
			{
				double intV;	    // ���������� ������ �� ������
				double intM;	    // ���������� ����� �� ������
				double intQ;	    // ���������� ������� �� ������
				uint32_t TNar;      // ����� ������ ������ ��� ������
				uint32_t Tmin;      // ������ ������ ������������
				uint32_t Tmax;      // ������ ������ �������������
				uint32_t Tdt;       // �������� ���������� ������ �����������
				uint32_t Ttn;       // ����������� �������������
				uint16_t teherr;    // ������ �� ��������
				uint8_t tekerr;     // ������ �� ��������
				uint8_t reserved;
				uint16_t t[3];      // ����������� �� ��������
				uint16_t reserved2;
				uint8_t p[3];       // �������� �� ��������
				uint8_t reserved3;
				float rshv;         // ���������� ��������� �������
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
