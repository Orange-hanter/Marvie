#pragma once

#include "SimGsmModem.h"

class SimGsmSocketBase
{
	friend class SimGsmModem;

public:
	SimGsmSocketBase( SimGsmModem* simGsm, uint32_t inputBufferSize, uint32_t outputBufferSize );
	~SimGsmSocketBase();

protected:
	void resetHelp();
	void connectHelpS( int linkId );
	void closeHelpS( SocketError error );

protected:
	SimGsmModem * gsm;
	SocketState sState;
	SocketError sError;
	EvtSource eSource;
	DynamicByteRingBuffer inBuffer;
	DynamicByteRingBuffer* outBuffer;
	int linkId;
	struct SendStruct
	{
		SimGsmModem::SendRequest req;
		SimGsmModem::RequestNode node;
	}* ssend;
};