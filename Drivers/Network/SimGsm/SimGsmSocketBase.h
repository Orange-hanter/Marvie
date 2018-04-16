#pragma once

#include "SimGsm.h"

class SimGsmSocketBase
{
	friend class SimGsm;

public:
	SimGsmSocketBase( SimGsm* simGsm, uint32_t inputBufferSize, uint32_t outputBufferSize );
	~SimGsmSocketBase();

protected:
	void resetHelp();
	void connectHelpS( int linkId );
	void closeHelpS( SocketError error );

protected:
	SimGsm * gsm;
	SocketState sState;
	SocketError sError;
	EvtSource eSource;
	DynamicByteRingBuffer inBuffer;
	DynamicByteRingBuffer* outBuffer;
	int linkId;
	struct SendStruct
	{
		SimGsm::SendRequest req;
		SimGsm::RequestNode node;
	}* ssend;
};