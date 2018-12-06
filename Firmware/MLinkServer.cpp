#include "MLinkServer.h"
#include "Crc32HW.h"
#include "Core/Assert.h"
#include "Support/Utility.h"
#include <string.h>

#define MLINK_PREAMBLE 0x203d26d1

MLinkServer::DataChannel::DataChannel( MLinkServer* link ) : link( link ), id( 0 ), ch( 0 )
{
	node.value = this;
}

MLinkServer::DataChannel::~DataChannel()
{
	close();
}

bool MLinkServer::DataChannel::open( uint8_t channel, const char* name, uint32_t size )
{
	chMtxLock( &link->ioMutex );
	if( link->sessionConfirmed )
	{
		// check if this channel is already open
		if( link->checkOutputDataChM( channel ) )
		{
			chMtxUnlock( &link->ioMutex );
			return false;
		}

		// add DataChannel object to activeOutputDataChList
		if( ++link->idCounter == 0 )
			link->idCounter = 1;
		this->ch = channel;
		this->id = link->idCounter;
		link->addOutputDataChM( &node );

		// send packet
		uint32_t len = strlen( name );
		if( len > DataChannelMSS - ( sizeof( uint32_t ) + sizeof( uint8_t ) ) )
			len = DataChannelMSS - ( sizeof( uint32_t ) + sizeof( uint8_t ) );
		Header* header = reinterpret_cast< Header* >( link->packetData );
		header->preamble = MLINK_PREAMBLE;
		header->size = sizeof( ChannelHeader ) + sizeof( uint32_t ) + len + sizeof( uint8_t );
		header->type = PacketType::OpenChannel;
		ChannelHeader* chHeader = reinterpret_cast< ChannelHeader* >( link->packetData + sizeof( Header ) );
		chHeader->id = id;
		chHeader->ch = ch;

		*( uint32_t* )( link->packetData + sizeof( Header ) + sizeof( ChannelHeader ) ) = size;
		memcpy( ( char* )link->packetData + sizeof( Header ) + sizeof( ChannelHeader ) + sizeof( uint32_t ), name, len );
		link->packetData[sizeof( Header ) + sizeof( ChannelHeader ) + sizeof( uint32_t ) + len] = 0;
		link->socketWriteM( link->packetData, sizeof( Header ) + sizeof( ChannelHeader ) + sizeof( uint32_t ) + len + sizeof( uint8_t ) );

		chMtxUnlock( &link->ioMutex );
		return true;
	}

	chMtxUnlock( &link->ioMutex );
	return false;
}

bool MLinkServer::DataChannel::sendData( const uint8_t* data, uint32_t size )
{
	while( size )
	{
		chMtxLock( &link->ioMutex );
		if( id )
		{
			uint32_t partSize = size;
			if( partSize > DataChannelMSS )
				partSize = DataChannelMSS;

			Header* header = reinterpret_cast< Header* >( link->packetData );
			header->preamble = MLINK_PREAMBLE;
			header->size = sizeof( ChannelHeader ) + partSize;
			header->type = PacketType::ChannelData;
			ChannelHeader* chHeader = reinterpret_cast< ChannelHeader* >( link->packetData + sizeof( Header ) );
			chHeader->id = id;
			chHeader->ch = ch;

			memcpy( ( char* )link->packetData + sizeof( Header ) + sizeof( ChannelHeader ), ( char* )data, partSize );
			link->socketWriteM( link->packetData, sizeof( Header ) + sizeof( ChannelHeader ) + partSize );

			data += partSize;
			size -= partSize;

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

bool MLinkServer::DataChannel::sendDataAndClose( const uint8_t* data, uint32_t size )
{
	if( sendData( data, size ) )
	{
		close( false );
		return true;
	}

	return false;
}

void MLinkServer::DataChannel::close()
{
	close( false );
}

void MLinkServer::DataChannel::cancelAndClose()
{
	close( true );
}

void MLinkServer::DataChannel::close( bool cancelFlag )
{
	chMtxLock( &link->ioMutex );
	if( id )
	{
		Header* header = reinterpret_cast< Header* >( link->packetData );
		header->preamble = MLINK_PREAMBLE;
		header->size = sizeof( ChannelHeader ) + sizeof( uint8_t );
		header->type = PacketType::CloseChannel;
		ChannelHeader* chHeader = reinterpret_cast< ChannelHeader* >( link->packetData + sizeof( Header ) );
		chHeader->id = id;
		chHeader->ch = ch;
		link->packetData[sizeof( Header ) + sizeof( ChannelHeader )] = cancelFlag;

		link->socketWriteM( link->packetData, sizeof( Header ) + sizeof( ChannelHeader ) + sizeof( uint8_t ) );
		link->removeOutputDataChM( ch );
	}
	chMtxUnlock( &link->ioMutex );
}

MLinkServer::MLinkServer() : BaseDynamicThread( MLINK_STACK_SIZE )
{
	linkState = State::Stopped;
	linkError = Error::NoError;
	socket = nullptr;
	authCallback = nullptr;
	inputDataChCallback = nullptr;
	idCounter = 0;
	sessionConfirmed = false;
	chThdQueueObjectInit( &stateWaitingQueue );
	chThdQueueObjectInit( &packetWaitingQueue );
	chMtxObjectInit( &ioMutex );
	chVTObjectInit( &timer );
	chVTObjectInit( &pingTimer );
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

void MLinkServer::setAuthenticationCallback( AuthenticationCallback* callback )
{
	if( linkState != State::Stopped )
		return;
	authCallback = callback;
}

void MLinkServer::startListening( tprio_t prio )
{
	if( linkState != State::Stopped )
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
	if( linkState == State::Stopping || linkState == State::Listening || linkState == State::Authenticating )
		msg = chThdEnqueueTimeoutS( &stateWaitingQueue, timeout );
	chSysUnlock();

	return msg == MSG_OK;
}

MLinkServer::DataChannel* MLinkServer::createDataChannel()
{
	return new DataChannel( this );
}

void MLinkServer::setDataChannelCallback( DataChannelCallback* callback )
{
	if( linkState != State::Stopped )
		return;

	this->inputDataChCallback = callback;
}

void MLinkServer::confirmSession()
{
	chSysLock();
	if( linkState == State::Connected )
		sessionConfirmed = true;
	chSysUnlock();
}

bool MLinkServer::sendPacket( uint8_t type, const uint8_t* data, uint16_t size )
{
	if( type > ( 255 - PacketType::User ) || size > MSS )
		return false;

	bool res = false;
	chMtxLock( &ioMutex );
	if( sessionConfirmed )
	{
		Header* header = reinterpret_cast< Header* >( packetData );
		header->preamble = MLINK_PREAMBLE;
		header->size = size;
		header->type = type + PacketType::User;

		memcpy( ( char* )packetData + sizeof( Header ), ( char* )data, size );
		socketWriteM( packetData, sizeof( Header ) + size );
		res = true;
	}
	chMtxUnlock( &ioMutex );

	return res;
}

bool MLinkServer::waitPacket( sysinterval_t timeout )
{
	msg_t msg;
	chMtxLock( &ioMutex );
	if( linkState != State::Connected || !sessionConfirmed )
	{
		bool res = linkState != State::Connected && packetBuffer.readAvailable();
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
	bool res = ( sessionConfirmed || linkState != State::Connected ) && packetBuffer.readAvailable();
	chMtxUnlock( &ioMutex );

	return res;
}

uint32_t MLinkServer::readPacket( uint8_t* type, uint8_t* data, uint32_t maxSize )
{
	assert( maxSize != 0 );
	uint32_t size;
	chMtxLock( &ioMutex );
	if( ( sessionConfirmed || linkState != State::Connected ) && packetBuffer.readAvailable() )
	{
		auto i = packetBuffer.begin();
		uint16_t dataSize = ( *i++ ) | ( ( *i++ ) << 8 );
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
		packetBuffer.read( nullptr, sizeof( uint16_t ) + sizeof( uint8_t ) + dataSize );
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

	EvtListener serverListener;
	server.eventSource()->registerMask( &serverListener, InnerEventMask::ServerEventFlag );
	if( !server.listen( 16021 ) )
	{
		chSysLock();
		extEventSource.broadcastFlagsI( ( eventflags_t )Event::Error );
		goto End;
	}

	while( linkState != State::Stopping || socket )
	{
		eventmask_t em = chEvtWaitAny( ALL_EVENTS );
		if( em & StopRequestFlag )
		{
			if( !socket )
				break;
			closeLink( Error::NoError );
		}
		if( em & TimeoutEventFlag )
			timeout();
		if( em & PingEventFlag && socket )
			sendIAmAlive();
		if( em & ServerEventFlag )
		{
			TcpSocket* newSocket;
			while( ( newSocket = server.nextPendingConnection() ) != nullptr )
			{
				if( socket || linkState == State::Stopping )
				{
					newSocket->disconnect();
					delete newSocket;
				}
				else
				{
					socket = newSocket;
					socket->eventSource()->registerMaskWithFlags( &socketListener, InnerEventMask::SocketEventFlag, 
																  ( eventflags_t )SocketEventFlag::Error 
																  | ( eventflags_t )SocketEventFlag::StateChanged
																  | ( eventflags_t )SocketEventFlag::InputAvailable );
					packetBuffer.clear();
					chSysLock();
					if( linkState == State::Listening )
					{
						linkState = State::Authenticating;
						chVTSetI( &timer, TIME_MS2I( MaxPacketTransferInterval * 2 ), timerCallback, this );
						extEventSource.broadcastFlagsI( ( eventflags_t )Event::StateChanged );
						chThdDequeueNextI( &stateWaitingQueue, MSG_OK );
						chSchRescheduleS();
						em |= SocketEventFlag;
					}
					chSysUnlock();
				}
			}
		}
		if( em & SocketEventFlag )
		{	
			if( socket )
			{
				if( socket->isOpen() )
					processNewBytes();
				else
				{
					chMtxLock( &ioMutex );
					closeLinkM( Error::RemoteClosedError );
					chMtxUnlock( &ioMutex );
					chEvtGetAndClearEvents( InnerEventMask::SocketEventFlag );
				}
			}
		}
	}

	server.eventSource()->unregister( &serverListener );
	server.close();	
	chVTReset( &timer );
	chVTReset( &pingTimer );

	chSysLock();
End:
	linkState = State::Stopped;
	extEventSource.broadcastFlagsI( ( eventflags_t )Event::StateChanged );
	chThdDequeueNextI( &stateWaitingQueue, MSG_OK );
	exitS( MSG_OK );
}

void MLinkServer::processNewBytes()
{
	int limit = ( int )socket->readAvailable();
	while( limit > 0 && socket )
	{
		if( parserState == ParserState::WaitData )
		{
			if( socket->readAvailable() < header.size )
				return;

			chMtxLock( &ioMutex );
			if( header.size )
				socket->read( packetData, header.size, TIME_INFINITE );
			limit -= header.size;
			parserState = ParserState::WaitHeader;
			processNewPacketM();
			chMtxUnlock( &ioMutex );
		}
		else // parserState == ParserState::WaitHeader
		{
			while( true )
			{
				if( socket->readAvailable() < sizeof( Header ) )
					return;
				
				socket->read( ( uint8_t* )&header, sizeof( Header ), TIME_INFINITE );
				limit -= sizeof( Header );
				if( header.preamble != MLINK_PREAMBLE )
				{
					closeLink( Error::ServerInnerError );
					return;
				}
				parserState = ParserState::WaitData;
				break;
			}
		}
	}
}

void MLinkServer::processNewPacketM()
{
	if( linkState == State::Connected )
	{
		chVTSet( &timer, TIME_MS2I( MaxPacketTransferInterval * 2 ), timerCallback, this );
		if( header.type >= PacketType::User )
		{
			if( packetBuffer.writeAvailable() < sizeof( header.size ) + sizeof( header.type ) + header.size )
				closeLinkM( Error::BufferOverflowError );
			else
			{
				uint8_t t = header.type - PacketType::User;
				packetBuffer.write( ( uint8_t* )&header.size, sizeof( header.size ) );
				packetBuffer.write( &t, sizeof( t ) );
				if( header.size )
					packetBuffer.write( packetData, header.size );
				chSysLock();
				extEventSource.broadcastFlagsI( ( eventflags_t )Event::NewPacketAvailable );
				chThdDequeueNextI( &packetWaitingQueue, MSG_OK );
				chSchRescheduleS();
				chSysUnlock();
			}
		}
		else
		{
			switch( header.type )
			{
			case PacketType::ChannelData:
			{
				ChannelHeader* chHeader = reinterpret_cast< ChannelHeader* >( packetData );
				if( checkInputDataChM( chHeader->ch, chHeader->id ) )
				{
					if( !inputDataChCallback->newDataReceived( chHeader->ch, packetData + sizeof( ChannelHeader ), header.size - sizeof( ChannelHeader ) ) )
					{
						removeInputDataChM( chHeader->ch );
						sendRemoteCloseChannelM( chHeader->ch, chHeader->id );
					}
				}
				break;
			}
			case PacketType::IAmAlive:
				break;
			case PacketType::OpenChannel:
			{
				ChannelHeader* chHeader = reinterpret_cast< ChannelHeader* >( packetData  );
				uint32_t size = *reinterpret_cast< uint32_t* >( packetData + sizeof( ChannelHeader ) );
				char* name = ( char* )packetData + sizeof( ChannelHeader ) + sizeof( uint32_t );
				if( inputDataChCallback )
				{
					if( inputDataChCallback->onOpennig( chHeader->ch, name, size ) )
						addInputDataChM( chHeader->ch, chHeader->id );
					else
						sendRemoteCloseChannelM( chHeader->ch, chHeader->id );
				}
				else
					sendRemoteCloseChannelM( chHeader->ch, chHeader->id );
				break;
			}
			case PacketType::CloseChannel:
			{
				ChannelHeader* chHeader = reinterpret_cast< ChannelHeader* >( packetData );
				bool canceled = *reinterpret_cast< uint8_t* >( packetData + sizeof( ChannelHeader ) );
				if( checkInputDataChM( chHeader->ch, chHeader->id ) )
				{
					inputDataChCallback->onClosing( chHeader->ch, canceled );
					removeInputDataChM( chHeader->ch );
				}
				break;
			}
			case PacketType::RemoteCloseChannel:
			{
				ChannelHeader* chHeader = reinterpret_cast< ChannelHeader* >( packetData );
				closeOutputDataChM( chHeader->ch, chHeader->id );
				break;
			}
			default:
				break;
			}
		}
	}
	else if( linkState == State::Authenticating )
	{
		chVTSet( &timer, TIME_MS2I( MaxPacketTransferInterval * 3 ), timerCallback, this );
		if( header.type == PacketType::AuthAck )
		{
			char* accountName = reinterpret_cast< char* >( packetData );
			char* password = accountName + strlen( accountName ) + 1;
			if( authCallback == nullptr || authCallback->authenticate( accountName, password ) )
			{
				chSysLock();
				if( linkState == State::Authenticating )
				{
					linkState = State::Connected;

					extEventSource.broadcastFlagsI( ( eventflags_t )Event::StateChanged );
					chThdDequeueNextI( &stateWaitingQueue, MSG_OK );
					chSchRescheduleS();
					chSysUnlock();
					sendAuthAckM();
				}
				else
					chSysUnlock();
			}
			else
			{
				sendAuthFailM();
				closeLinkM( Error::AuthenticationError );
			}
		}
		else
			closeLinkM( Error::ServerInnerError );
	}
}

uint32_t MLinkServer::socketWriteM( const uint8_t* data, uint32_t size )
{
	uint32_t n = socket->write( data, size, TIME_INFINITE );
	chVTSet( &pingTimer, TIME_MS2I( PingInterval ), pingTimerCallback, this );
	
	return n;
}

void MLinkServer::sendAuthAckM()
{
	Header* header = reinterpret_cast< Header* >( packetData );
	header->preamble = MLINK_PREAMBLE;
	header->size = 0;
	header->type = PacketType::AuthAck;
	socketWriteM( packetData, sizeof( Header ) );
}

void MLinkServer::sendAuthFailM()
{
	Header* header = reinterpret_cast< Header* >( packetData );
	header->preamble = MLINK_PREAMBLE;
	header->size = 0;
	header->type = PacketType::AuthFail;
	socket->write( packetData, sizeof( Header ), TIME_INFINITE );
}

void MLinkServer::sendIAmAlive()
{
	chMtxLock( &ioMutex );
	Header* header = reinterpret_cast< Header* >( packetData );
	header->preamble = MLINK_PREAMBLE;
	header->size = 0;
	header->type = PacketType::IAmAlive;
	socketWriteM( packetData, sizeof( Header ) );
	chMtxUnlock( &ioMutex );
}

void MLinkServer::sendRemoteCloseChannelM( uint8_t channel, uint32_t id )
{
	Header* header = reinterpret_cast< Header* >( packetData );
	header->preamble = MLINK_PREAMBLE;
	header->size = sizeof( ChannelHeader );
	header->type = PacketType::RemoteCloseChannel;
	ChannelHeader* chHeader = reinterpret_cast< ChannelHeader* >( packetData + sizeof( Header ) );
	chHeader->ch = channel;
	chHeader->id = id;
	socketWriteM( packetData, sizeof( Header ) + sizeof( ChannelHeader ) );
}

bool MLinkServer::checkInputDataChM( uint8_t channel, uint32_t id )
{
	for( auto i = activeIDCList.begin(); i != activeIDCList.end(); ++i )
	{
		if( channel == ( *i ).ch )
			return ( *i ).id == id;
		else if( channel < ( *i ).ch )
			return false;
	}

	return false;
}

void MLinkServer::addInputDataChM( uint8_t channel, uint32_t id )
{
	if( activeIDCList.empty() || activeIDCList.back().ch < channel )
		activeIDCList.push_back( IDC{ channel, id } );
	else
	{
		for( auto i = activeIDCList.begin(); i != activeIDCList.end(); ++i )
		{
			if( channel < ( *i ).ch )
			{
				activeIDCList.insert( i, IDC{ channel, id } );
				break;
			}
		}
	}
}

void MLinkServer::removeInputDataChM( uint8_t channel )
{
	for( auto i = activeIDCList.begin(); i != activeIDCList.end(); ++i )
	{
		if( channel == ( *i ).ch )
		{
			activeIDCList.erase( i );
			return;
		}
	}
	assert( false );
}

bool MLinkServer::checkOutputDataChM( uint8_t channel )
{
	for( auto i = activeODCList.begin(); i != activeODCList.end(); ++i )
	{
		if( channel == ( *i )->ch )
			return true;
		else if( channel < ( *i )->ch )
			break;
	}

	return false;
}

void MLinkServer::addOutputDataChM( NanoList< DataChannel* >::Node* node )
{
	if( activeODCList.size() == 0 || activeODCList.back()->value->ch < node->value->ch )
		activeODCList.pushBack( node );
	else
	{
		for( auto i = activeODCList.begin(); i != activeODCList.end(); ++i )
		{
			if( node->value->ch < ( *i )->ch )
			{
				activeODCList.insert( i, node );
				break;
			}
		}
	}
}

void MLinkServer::removeOutputDataChM( uint8_t channel )
{
	for( auto i = activeODCList.begin(); i != activeODCList.end(); ++i )
	{
		if( channel == ( *i )->ch )
		{
			( *i )->id = 0;
			activeODCList.remove( i );
			return;
		}
	}
	assert( false );
}

void MLinkServer::closeOutputDataChM( uint8_t channel, uint32_t id )
{
	for( auto i = activeODCList.begin(); i != activeODCList.end(); ++i )
	{
		if( channel == ( *i )->ch )
		{
			if( ( *i )->id == id )
			{
				( *i )->id = 0;
				activeODCList.remove( i );
			}
			return;
		}
		else if( channel < ( *i )->ch )
			break;
	}
}

void MLinkServer::timeout()
{
	assert( linkState == State::Connected || linkState == State::Authenticating );
	closeLink( Error::TimeoutError );
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
	socket->eventSource()->unregister( &socketListener );
	socket->disconnect();
	delete socket;
	socket = nullptr;
	NanoList< DataChannel* >::Node* node;
	while( ( node = activeODCList.popFront() ) )
		node->value->id = 0;
	activeIDCList.clear();
	if( err != Error::NoError )
	{
		linkError = err;
		flags |= ( eventflags_t )Event::Error;
	}
	extEventSource.broadcastFlags( flags );
	chVTReset( &timer );
	chVTReset( &pingTimer );
	chSysLock();
	chEvtGetAndClearEventsI( InnerEventMask::TimeoutEventFlag | InnerEventMask::PingEventFlag );
	chThdDequeueNextI( &packetWaitingQueue, MSG_RESET );
	chSchRescheduleS();
	chSysUnlock();
	parserState = ParserState::WaitHeader;
}

void MLinkServer::timerCallback( void* p )
{
	chSysLockFromISR();
	chEvtSignalI( reinterpret_cast< MLinkServer* >( p )->thread_ref, InnerEventMask::TimeoutEventFlag );
	chSysUnlockFromISR();
}

void MLinkServer::pingTimerCallback( void* p )
{
	chSysLockFromISR();
	chEvtSignalI( reinterpret_cast< MLinkServer* >( p )->thread_ref, InnerEventMask::PingEventFlag );
	chSysUnlockFromISR();
}
