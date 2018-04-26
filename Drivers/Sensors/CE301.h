#pragma once

#include "AbstractSensor.h"

class CE301 : public AbstractBRSensor
{
public:
	class Data : public SensorData
	{
		friend class CE301;
	public:
		Data()
		{
			tariffs[0] = tariffs[1] = tariffs[2] = 0;
			tariffs[3] = tariffs[4] = tariffs[5] = 0;
		}
		double tariffs[6];
	};

	explicit CE301();
	virtual ~CE301();

	const char* name() const final override;
	void setAddress( uint8_t address );
	void setIODevice( IODevice* ) final override;
	IODevice* ioDevice() final override;

	Data* readData() final override;
	Data* sensorData() final override;

private:
	bool isValidChecksum( ByteRingIterator begin, ByteRingIterator end ) const;

private:
	enum ControlCharacters : uint8_t { SOH = 1, STX, ETX, EOT, ENQ, ACK, NAK, SYN, ETB }; // ISO1745
	uint8_t deviceAddressRequest[6] = { '/', '?', 48, '!', '\r', '\n' };
	static const uint8_t deviceAddressRequestUnion[5];
	static const uint8_t programModeRequest[6];
	static const uint8_t tariffsDataRequest[13];

	IODevice* io;
	uint8_t address;
	Data data;
};