#include "SimGsmSocketBase.h"

SimGsmSocketBase::SimGsmSocketBase( SimGsmModem* simGsm, uint32_t inputBufferSize, uint32_t outputBufferSize ) : inBuffer( inputBufferSize )
{
	gsm = simGsm;
	if( outputBufferSize )
	{
		outBuffer = new DynamicByteRingBuffer( outputBufferSize );
		ssend = new SendStruct;
		ssend->node.value = &ssend->req;
	}
	else
	{
		outBuffer = nullptr;
		ssend = nullptr;
	}
	linkId = -1;
	sState = SocketState::Unconnected;
	sError = SocketError::NoError;
}

SimGsmSocketBase::~SimGsmSocketBase()
{
	delete outBuffer;
	delete ssend;
}

void SimGsmSocketBase::resetHelp()
{
	sError = SocketError::NoError;
	if( ssend )
		ssend->req.use = false;
	inBuffer.clear();
}

void SimGsmSocketBase::connectHelpS( int linkId )
{
	this->linkId = linkId;
	sState = SocketState::Connected;
	eSource.broadcastFlagsI( ( eventflags_t )SocketEventFlag::StateChanged );
}

void SimGsmSocketBase::closeHelpS( SocketError error )
{
	linkId = -1;
	sState = SocketState::Unconnected;
	sError = error;
	eSource.broadcastFlagsI( ( eventflags_t )SocketEventFlag::StateChanged );
	if( sError != SocketError::NoError )
		eSource.broadcastFlagsI( ( eventflags_t )SocketEventFlag::Error );
	if( outBuffer )
		outBuffer->clear();	
}