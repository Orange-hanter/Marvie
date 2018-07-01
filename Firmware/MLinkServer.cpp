#include "MLinkServer.h"
#include "Crc32HW.h"
#include "Core/Assert.h"
#include "Support/Utility.h"
#include <string.h>

#define MLINK_PREAMBLE 0x203d26d1

MLinkServer::ComplexDataChannel::ComplexDataChannel( MLinkServer* link ) : link( link ), r( 0 ), id( 0 ) {}

MLinkServer::ComplexDataChannel::~ComplexDataChannel()
{
	close();
}

bool MLinkServer::ComplexDataChannel::open( uint8_t id, const char* name, uint32_t size )
{
	chMtxLock( &link->cdcMutex );
	chMtxLock( &link->ioMutex );
	if( link->confirmedR == link->r && link->r )
	{
		r = link->r;
		this->id = id;
		uint32_t len = strlen( name );
		if( len > 250 )
			len = 250;
		uint8_t exHeader[sizeof( Header ) + sizeof( uint8_t ) + sizeof( uint32_t )];
		Header* header = reinterpret_cast< Header* >( exHeader );
		header->preamble = MLINK_PREAMBLE;
		header->size = sizeof( uint8_t ) + sizeof( uint32_t ) + len;
		header->sqNum = link->sqNumCounter++;
		header->type = PacketType::ComplexDataBegin;
		exHeader[sizeof( Header )] = id;
		*reinterpret_cast< uint32_t* >( exHeader + sizeof( Header ) + sizeof( uint8_t ) ) = size;
		link->device->write( exHeader, sizeof( exHeader ), TIME_INFINITE );
		uint32_t crc;
		if( len )
		{
			link->device->write( reinterpret_cast< const uint8_t* >( name ), len, TIME_INFINITE );
			crc = calcCrc( exHeader, sizeof( exHeader ), reinterpret_cast< const uint8_t* >( name ), len );
		}
		else
			crc = calcCrc( exHeader, sizeof( exHeader ) );
		link->device->write( reinterpret_cast< uint8_t * >( &crc ), sizeof( crc ), TIME_INFINITE );

		BeginResponse resp;
		resp.id = id;
		chBSemObjectInit( &resp.sem, true );
		assert( link->cdcBResp == nullptr );
		link->cdcBResp = &resp;
		chMtxUnlock( &link->ioMutex );

		if( chBSemWait( &resp.sem ) == MSG_OK )
		{
			g = resp.g, n = 0;
			if( g == 0xFFFFFFFF ) // sending rejected
				r = 0;
		}
		else
			r = 0;

		chMtxUnlock( &link->cdcMutex );
		return r != 0;
	}

	r = 0;
	chMtxUnlock( &link->ioMutex );
	chMtxUnlock( &link->cdcMutex );
	return false;
}

bool MLinkServer::ComplexDataChannel::sendData( uint8_t* data, uint32_t size )
{
	while( size )
	{
		chMtxLock( &link->ioMutex );
		if( link->r == r && r )
		{
			uint32_t partSize = size;
			if( partSize > 254 )
				partSize = 254;

			uint8_t exHeader[sizeof( Header ) + sizeof( uint8_t )];
			Header* header = reinterpret_cast< Header* >( exHeader );
			header->preamble = MLINK_PREAMBLE;
			header->size = sizeof( uint8_t ) + partSize;
			header->sqNum = link->sqNumCounter++;
			if( ++n == g )
				header->type = PacketType::ComplexDataNext, n = 0;
			else
				header->type = PacketType::ComplexData;
			exHeader[sizeof( Header )] = id;
			uint32_t crc = calcCrc( exHeader, sizeof( exHeader ), data, partSize );
			link->device->write( exHeader, sizeof( exHeader ), TIME_INFINITE );
			link->device->write( data, partSize, TIME_INFINITE );
			link->device->write( reinterpret_cast< uint8_t * >( &crc ), sizeof( crc ), TIME_INFINITE );

			data += partSize;
			size -= partSize;

			if( header->type == PacketType::ComplexDataNext )
			{
				DataNextResponse resp;
				resp.id = id;
				chBSemObjectInit( &resp.sem, true );
				NanoList< DataNextResponse* >::Node node( &resp );
				link->cdcDataRespList.pushBack( &node );
				chMtxUnlock( &link->ioMutex );
				chBSemWait( &resp.sem );
			}
			else
				chMtxUnlock( &link->ioMutex );
		}
		else
		{
			chMtxUnlock( &link->ioMutex );
			return false;
		}
	}

	return true;
}

bool MLinkServer::ComplexDataChannel::sendDataAndClose( uint8_t* data, uint32_t size )
{
	return false;
}

void MLinkServer::ComplexDataChannel::close()
{
	close( false );
}

void MLinkServer::ComplexDataChannel::cancelAndClose()
{
	close( true );
}

void MLinkServer::ComplexDataChannel::close( bool cancelFlag )
{
	chMtxLock( &link->ioMutex );
	if( link->r == r && r )
	{
		uint8_t exHeader[sizeof( Header ) + sizeof( uint8_t ) + sizeof( uint8_t )];
		Header* header = reinterpret_cast< Header* >( exHeader );
		header->preamble = MLINK_PREAMBLE;
		header->size = sizeof( uint8_t ) + sizeof( uint8_t );
		header->sqNum = link->sqNumCounter++;
		header->type = PacketType::ComplexDataEnd;
		exHeader[sizeof( Header )] = id;
		exHeader[sizeof( Header ) + sizeof( uint8_t )] = cancelFlag;
		uint32_t crc = calcCrc( exHeader, sizeof( exHeader ) );
		link->device->write( exHeader, sizeof( exHeader ), TIME_INFINITE );
		link->device->write( reinterpret_cast< uint8_t * >( &crc ), sizeof( crc ), TIME_INFINITE );
		r = 0;
	}
	chMtxUnlock( &link->ioMutex );
}

MLinkServer::MLinkServer() : BaseDynamicThread( MLINK_STACK_SIZE )
{
	linkState = State::Stopped;
	linkError = Error::NoError;
	device = nullptr;
	callbacks = nullptr;	
	sqNumCounter = sqNumNext = 0;
	confirmedR = r = 0;
	pongWaiting = false;
	chThdQueueObjectInit( &stateWaitingQueue );
	chThdQueueObjectInit( &packetWaitingQueue );
	chMtxObjectInit( &ioMutex );
	chMtxObjectInit( &cdcMutex );
	chVTObjectInit( &timer );
	cdcBResp = nullptr;
	parserState = ParserState::WaitHeader;
}

MLinkServer::~MLinkServer()
{
	stopListening();
	waitForStateChanged();
}

MLinkServer::State MLinkServer::state() const
{
	return linkState;
}

MLinkServer::Error MLinkServer::error() const
{
	return linkError;
}

void MLinkServer::setIODevice( IODevice* device )
{
	if( linkState != State::Stopped )
		return;
	this->device = device;
}

void MLinkServer::startListening( tprio_t prio )
{
	if( linkState != State::Stopped || !device || !device->isOpen() )
		return;

	linkError = Error::NoError;
	linkState = State::Listening;
	extEventSource.broadcastFlags( ( eventflags_t )Event::StateChanged );
	start( prio );
}

void MLinkServer::stopListening()
{
	chSysLock();
	if( linkState == State::Stopped || linkState == State::Stopping )
	{
		chSysUnlock();
		return;
	}
	linkState = State::Stopping;
	extEventSource.broadcastFlagsI( ( eventflags_t )Event::StateChanged );
	chEvtSignalI( this->thread_ref, InnerEventMask::StopRequestFlag );
	chSchRescheduleS();
	chSysUnlock();
}

bool MLinkServer::waitForStateChanged( sysinterval_t timeout /*= TIME_INFINITE */ )
{
	msg_t msg = MSG_OK;
	chSysLock();
	if( linkState == State::Stopping || linkState == State::Listening )
		msg = chThdEnqueueTimeoutS( &stateWaitingQueue, timeout );
	chSysUnlock();

	return msg == MSG_OK;
}

MLinkServer::ComplexDataChannel* MLinkServer::createComplexDataChannel()
{
	return new ComplexDataChannel( this );
}

void MLinkServer::setComplexDataReceiveCallback( ComplexDataCallback* callbacks )
{
	this->callbacks = callbacks;
}

void MLinkServer::confirmSession()
{
	confirmedR = r;
}

bool MLinkServer::sendPacket( uint8_t type, const uint8_t* data, uint8_t size )
{
	if( type > ( 255 - PacketType::User ) )
		return false;

	bool res = false;
	chMtxLock( &ioMutex );
	if( linkState == State::Connected && confirmedR == r )
	{
		Header header{ MLINK_PREAMBLE, size, ( uint8_t )( type + PacketType::User ), sqNumCounter++ };
		device->write( reinterpret_cast< uint8_t * >( &header ), sizeof( header ), TIME_INFINITE );
		uint32_t crc;
		if( size )
		{
			device->write( data, size, TIME_INFINITE );
			crc = calcCrc( reinterpret_cast< uint8_t * >( &header ), sizeof( header ), data, size );
		}
		else
			crc = calcCrc( reinterpret_cast< uint8_t * >( &header ), sizeof( header ) );
		device->write( reinterpret_cast< uint8_t * >( &crc ), sizeof( crc ), TIME_INFINITE );
		res = true;
	}
	chMtxUnlock( &ioMutex );

	return res;
}

bool MLinkServer::waitPacket( sysinterval_t timeout )
{
	msg_t msg;
	chMtxLock( &ioMutex );
	if( linkState != State::Connected || confirmedR != r )
	{
		bool res = r == 0 && packetBuffer.readAvailable();
		chMtxUnlock( &ioMutex );
		return res;
	}
	chSysLock();
	chMtxUnlockS( &ioMutex );
	if( packetBuffer.readAvailable() )
	{
		msg = MSG_OK;
		chSchRescheduleS();
	}
	else
		msg = chThdEnqueueTimeoutS( &packetWaitingQueue, timeout );
	chSysUnlock();

	return msg == MSG_OK;
}

bool MLinkServer::hasPendingPacket()
{
	chMtxLock( &ioMutex );
	bool res = ( confirmedR == r || r == 0 ) && packetBuffer.readAvailable();
	chMtxUnlock( &ioMutex );

	return res;
}

uint32_t MLinkServer::readPacket( uint8_t* type, uint8_t* data, uint32_t maxSize )
{
	assert( maxSize != 0 );
	uint32_t size;
	chMtxLock( &ioMutex );
	if( ( confirmedR == r || r == 0 ) && packetBuffer.readAvailable() )
	{
		auto i = packetBuffer.begin();
		uint8_t dataSize = *i++;
		*type = *i++;
		if( dataSize )
		{
			size = dataSize;
			if( size > maxSize )
				size = maxSize;
			Utility::copy( data, i, size );
		}
		else
			size = 0;
		packetBuffer.read( nullptr, sizeof( uint8_t ) + sizeof( uint8_t ) + dataSize );
	}
	else
		size = 0, *type = 255;
	chMtxUnlock( &ioMutex );

	return size;
}

EvtSource* MLinkServer::eventSource()
{
	return &extEventSource;
}

void MLinkServer::main()
{
	parserState = ParserState::WaitHeader;

	EvtListener listener;
	device->eventSource()->registerMaskWithFlags( &listener, InnerEventMask::IODeviceEventFlag, CHN_INPUT_AVAILABLE );
	chEvtAddEvents( IODeviceEventFlag );
	while( linkState != State::Stopping || r )
	{
		eventmask_t em = chEvtWaitAny( StopRequestFlag | TimeoutEventFlag | IODeviceEventFlag );
		if( em & StopRequestFlag )
		{
			if( !r )
				break;
			sendFin();
			chSysLock();
			chVTSetI( &timer, TIME_MS2I( 1000 ), timerCallback, this );
			chEvtGetAndClearEventsI( TimeoutEventFlag );
			em &= ~TimeoutEventFlag;
			chSysUnlock();
		}
		if( em & TimeoutEventFlag )
			timeout();
		if( em & IODeviceEventFlag )
			processNewBytes();
	}

	device->eventSource()->unregister( &listener );
	chVTReset( &timer );

	chSysLock();
	linkState = State::Stopped;
	extEventSource.broadcastFlagsI( ( eventflags_t )Event::StateChanged );
	chThdDequeueNextI( &stateWaitingQueue, MSG_OK );
	exitS( MSG_OK );
}

void MLinkServer::processNewBytes()
{
	while( true )
	{
		if( parserState == ParserState::WaitData )
		{
			if( device->readAvailable() < reinterpret_cast< Header* >( packetData )->size )
				return;
			Header* header = reinterpret_cast< Header* >( packetData );
			if( header->size )
				device->read( packetData + sizeof( Header ) + sizeof( uint32_t ), header->size, TIME_INFINITE );
			parserState = ParserState::WaitHeader;

			if( calcCrc( packetData, sizeof( Header ) + header->size ) != *reinterpret_cast< uint32_t* >( packetData + sizeof( Header ) + header->size ) ) // crc error
				continue;
			if( linkState == State::Connected )
			{
				if( header->sqNum != sqNumNext++ ) // sequence violation
					closeLink( Error::SequenceViolationError );
				else
					processNewPacket();
			}
			else
				processNewPacket();
		}
		else // parserState == ParserState::WaitHeader
		{
			while( true )
			{
				if( device->readAvailable() < sizeof( Header ) + sizeof( uint32_t ) )
					return;
				uint32_t preamble;
				auto it = device->inputBuffer()->begin();
				for( int i = 0; i < 4; ++i, ++it )
					reinterpret_cast< uint8_t* >( &preamble )[i] = *it;
				if( preamble != MLINK_PREAMBLE )
				{
					device->read( nullptr, 1, TIME_INFINITE );
					continue;
				}

				device->read( packetData, sizeof( Header ) + sizeof( uint32_t ), TIME_INFINITE );
				parserState = ParserState::WaitData;
				break;
			}
		}
	}
}

void MLinkServer::processNewPacket()
{
	chMtxLock( &ioMutex );
	Header* header = reinterpret_cast< Header* >( packetData );
	if( linkState == State::Connected )
	{
		pongWaiting = false;
		chVTSet( &timer, TIME_MS2I( 600 ), timerCallback, this );
		if( header->type >= PacketType::User )
		{
			if( packetBuffer.writeAvailable() < sizeof( header->size ) + sizeof( header->type ) + header->size )
				closeLinkM( Error::BufferOverflowError );
			else
			{
				uint8_t t = header->type - PacketType::User;
				packetBuffer.write( &header->size, sizeof( header->size ) );
				packetBuffer.write( &t, sizeof( t ) );
				if( header->size )
					packetBuffer.write( packetData + sizeof( Header ), header->size );
				chSysLock();
				chThdDequeueNextI( &packetWaitingQueue, MSG_OK );
				chSchRescheduleS();
				chSysUnlock();
			}
		}
		else
		{
			switch( header->type )
			{
			case PacketType::ComplexData:
				if( callbacks )
					callbacks->newDataReceived( packetData[sizeof( Header )], packetData + sizeof( Header ) + sizeof( uint8_t ), header->size - 1 );
				break;
			case PacketType::ComplexDataNext:
				if( callbacks )
					callbacks->newDataReceived( packetData[sizeof( Header )], packetData + sizeof( Header ) + sizeof( uint8_t ), header->size - 1 );
				sendComplexDataNextAckM( packetData[sizeof( Header )] );
				break;
			case PacketType::ComplexDataNextAck:
			{
				auto node = cdcDataRespList.popFront();
				assert( node != nullptr );
				assert( node->value->id == packetData[sizeof( Header )] );
				chBSemSignal( &node->value->sem );
				break;
			}
			case PacketType::Ping:
				sendPongM();
				break;
			case PacketType::ComplexDataBegin:
			{
				packetData[sizeof( Header ) + header->size] = 0;
				uint32_t g;
				if( callbacks )
					g = callbacks->onOpennig( packetData[sizeof( Header )], ( const char * )packetData + sizeof( Header ) + sizeof( uint8_t ) + sizeof( uint32_t ),
											  *reinterpret_cast< uint32_t* >( packetData + sizeof( Header ) + sizeof( uint8_t ) ) );
				else
					g = 0;
				sendComplexDataBeginAckM( packetData[sizeof( Header )], g );
				break;
			}
			case PacketType::ComplexDataBeginAck:
				assert( cdcBResp != nullptr );
				assert( cdcBResp->id == packetData[sizeof( Header )] );
				cdcBResp->g = *reinterpret_cast< uint32_t* >( packetData + sizeof( Header ) + sizeof( uint8_t ) );
				chBSemSignal( &cdcBResp->sem );
				cdcBResp = nullptr;
				break;
			case PacketType::ComplexDataEnd:
				if( callbacks )
					callbacks->onClosing( packetData[sizeof( Header )], packetData[sizeof( Header ) + sizeof( uint8_t )] );
				break;
			case PacketType::Fin:
				if( *reinterpret_cast< uint64_t* >( packetData + sizeof( Header ) ) != r )
					break;
				sendFinAckM();
				closeLinkM( Error::NoError );
				break;
			default:
				break;
			}
		}
	}
	else if( linkState == State::Listening )
	{
		if( header->type == PacketType::Syn )
		{
			chSysLock();
			if( linkState == State::Listening )
			{
				linkState = State::Connected;
				sqNumNext = 1;
				r = *reinterpret_cast< uint64_t* >( packetData + sizeof( Header ) );
				packetBuffer.clear();
				chVTSetI( &timer, TIME_MS2I( 600 ), timerCallback, this );
				chThdDequeueNextI( &stateWaitingQueue, MSG_OK );
				chSchRescheduleS();
				chSysUnlock();
				sendSynAckM();
			}
			else
				chSysUnlock();
		}
	}
	else if( linkState == State::Stopping )
	{
		if( header->type == PacketType::FinAck )
		{
			if( *reinterpret_cast< uint64_t* >( packetData + sizeof( Header ) ) == r )
				closeLinkM( Error::NoError );
		}
	}
	chMtxUnlock( &ioMutex );
}

void MLinkServer::sendFin()
{
	chMtxLock( &ioMutex );
	Header* header = reinterpret_cast< Header* >( packetData );
	header->preamble = MLINK_PREAMBLE;
	header->size = sizeof( uint64_t );
	header->sqNum = sqNumCounter++;
	header->type = PacketType::Fin;
	*reinterpret_cast< uint64_t* >( packetData + sizeof( Header ) ) = r;
	*reinterpret_cast< uint32_t* >( packetData + sizeof( Header ) + sizeof( uint64_t ) ) = calcCrc( packetData, sizeof( Header ) + sizeof( uint64_t ) );
	device->write( packetData, sizeof( Header ) + sizeof( uint64_t ) + sizeof( uint32_t ), TIME_INFINITE );
	chMtxUnlock( &ioMutex );
}

void MLinkServer::sendPing()
{
	chMtxLock( &ioMutex );
	Header* header = reinterpret_cast< Header* >( packetData );
	header->preamble = MLINK_PREAMBLE;
	header->size = 0;
	header->sqNum = sqNumCounter++;
	header->type = PacketType::Ping;
	*reinterpret_cast< uint32_t* >( packetData + sizeof( Header ) ) = calcCrc( packetData, sizeof( Header ) );
	device->write( packetData, sizeof( Header ) + sizeof( uint32_t ), TIME_INFINITE );
	chMtxUnlock( &ioMutex );
}

void MLinkServer::sendPongM()
{
	Header* header = reinterpret_cast< Header* >( packetData );
	header->preamble = MLINK_PREAMBLE;
	header->size = 0;
	header->sqNum = sqNumCounter++;
	header->type = PacketType::Pong;
	*reinterpret_cast< uint32_t* >( packetData + sizeof( Header ) ) = calcCrc( packetData, sizeof( Header ) );
	device->write( packetData, sizeof( Header ) + sizeof( uint32_t ), TIME_INFINITE );
}

void MLinkServer::sendSynAckM()
{
	Header* header = reinterpret_cast< Header* >( packetData );
	header->preamble = MLINK_PREAMBLE;
	header->size = sizeof( uint64_t );
	header->sqNum = sqNumCounter++;
	header->type = PacketType::SynAck;
	*reinterpret_cast< uint64_t* >( packetData + sizeof( Header ) ) = r;
	*reinterpret_cast< uint32_t* >( packetData + sizeof( Header ) + sizeof( uint64_t ) ) = calcCrc( packetData, sizeof( Header ) + sizeof( uint64_t ) );
	device->write( packetData, sizeof( Header ) + sizeof( uint64_t ) + sizeof( uint32_t ), TIME_INFINITE );
}

void MLinkServer::sendFinAckM()
{
	Header* header = reinterpret_cast< Header* >( packetData );
	header->preamble = MLINK_PREAMBLE;
	header->size = sizeof( uint64_t );
	header->sqNum = sqNumCounter++;
	header->type = PacketType::FinAck;
	*reinterpret_cast< uint64_t* >( packetData + sizeof( Header ) ) = r;
	*reinterpret_cast< uint32_t* >( packetData + sizeof( Header ) + sizeof( uint64_t ) ) = calcCrc( packetData, sizeof( Header ) + sizeof( uint64_t ) );
	device->write( packetData, sizeof( Header ) + sizeof( uint64_t ) + sizeof( uint32_t ), TIME_INFINITE );
}

void MLinkServer::sendComplexDataBeginAckM( uint8_t id, uint32_t g )
{
	Header* header = reinterpret_cast< Header* >( packetData );
	header->preamble = MLINK_PREAMBLE;
	header->size = sizeof( uint8_t ) + sizeof( uint32_t );
	header->sqNum = sqNumCounter++;
	header->type = PacketType::ComplexDataBeginAck;
	*reinterpret_cast< uint8_t* >( packetData + sizeof( Header ) ) = id;
	*reinterpret_cast< uint32_t* >( packetData + sizeof( Header ) + sizeof( uint8_t ) ) = g;
	*reinterpret_cast< uint32_t* >( packetData + sizeof( Header ) + sizeof( uint8_t ) + sizeof( uint32_t ) ) = calcCrc( packetData, sizeof( Header ) + sizeof( uint8_t ) + sizeof( uint32_t ) );
	device->write( packetData, sizeof( Header ) + sizeof( uint8_t ) + sizeof( uint32_t ) + sizeof( uint32_t ), TIME_INFINITE );
}

void MLinkServer::sendComplexDataNextAckM( uint8_t id )
{
	Header* header = reinterpret_cast< Header* >( packetData );
	header->preamble = MLINK_PREAMBLE;
	header->size = sizeof( uint8_t );
	header->sqNum = sqNumCounter++;
	header->type = PacketType::ComplexDataNextAck;
	*reinterpret_cast< uint8_t* >( packetData + sizeof( Header ) ) = id;
	*reinterpret_cast< uint32_t* >( packetData + sizeof( Header ) + sizeof( uint8_t ) ) = calcCrc( packetData, sizeof( Header ) + sizeof( uint8_t ) );
	device->write( packetData, sizeof( Header ) + sizeof( uint8_t ) + sizeof( uint32_t ), TIME_INFINITE );
}

void MLinkServer::timeout()
{
	if( linkState == State::Connected )
	{
		if( pongWaiting )
			closeLink( Error::ResponseTimeoutError );
		else
		{
			pongWaiting = true;			
			sendPing();
			chVTSet( &timer, TIME_MS2I( 200 ), timerCallback, this );
		}
	}
	else if( linkState == State::Stopping )
		closeLink( Error::NoError );
}

void MLinkServer::closeLink( Error err )
{
	chMtxLock( &ioMutex );
	closeLinkM( err );
	chMtxUnlock( &ioMutex );
}

void MLinkServer::closeLinkM( Error err )
{
	chSysLock();
	eventflags_t flags = 0;
	if( linkState != State::Stopping )
	{
		linkState = State::Listening;
		flags |= ( eventflags_t )Event::StateChanged;
	}
	chSysUnlock();
	sqNumCounter = sqNumNext = 0;
	r = 0;
	pongWaiting = false;
	if( cdcBResp )
	{
		chBSemReset( &cdcBResp->sem, true );
		cdcBResp = nullptr;
	}
	NanoList< ComplexDataChannel::DataNextResponse* >::Node* node;
	while( ( node = cdcDataRespList.popFront() ) )
		chBSemReset( &node->value->sem, true );
	if( err != Error::NoError )
	{
		linkError = err;
		flags |= ( eventflags_t )Event::Error;
	}
	extEventSource.broadcastFlags( flags );
	chVTReset( &timer );
	chSysLock();
	chThdDequeueNextI( &packetWaitingQueue, MSG_RESET );
	chSchRescheduleS();
	chSysUnlock();
	parserState = ParserState::WaitHeader;
}

uint32_t MLinkServer::calcCrc( const uint8_t* data, uint32_t size )
{
	crcAcquireUnit( &CRCD1 );
	uint32_t crc = Crc32HW::crc32Byte( data, size, -1 );
	crcReleaseUnit( &CRCD1 );
	return crc;
}

uint32_t MLinkServer::calcCrc( const uint8_t* pDataA, uint32_t sizeA, const uint8_t* pDataB, uint32_t sizeB )
{
	crcAcquireUnit( &CRCD1 );
	uint32_t crc = Crc32HW::crc32Byte( pDataA, sizeA, pDataB, sizeB );
	crcReleaseUnit( &CRCD1 );
	return crc;
}

void MLinkServer::timerCallback( void* p )
{
	chSysLockFromISR();
	chEvtSignalI( reinterpret_cast< MLinkServer* >( p )->thread_ref, InnerEventMask::TimeoutEventFlag );
	chSysUnlockFromISR();
}