#pragma once

#include "AbstractSensor.h"

class TestSensorB : public AbstractBRSensor
{
public:
	class Data : public SensorData
	{
		friend class TestSensorB;
	public:
		Data() { readNum = 0; }
		int readNum;
	};
	enum class ErrorType { NoResp, Crc };

	explicit TestSensorB();
	~TestSensorB();

	inline static const char* sName() { return "TestSensorB"; }
	const char* name() const final override;

	void setBaudrate( uint32_t baudrate );
	void setTextMessage( const char* text );
	void setGoodNum( uint32_t n );
	void setBadNum( uint32_t n );
	void setErrorType( ErrorType errType );
	void setErrorCode( uint8_t errCode );

	Data* readData() final override;
	Data* sensorData() final override;
	uint32_t sensorDataSize() final override;

private:
	bool nextIsGood();

private:
	uint32_t _baudrate;
	char* _text;
	uint32_t goodN, badN;
	ErrorType errType;
	Data data;
};