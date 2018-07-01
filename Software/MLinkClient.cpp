#include "MLinkClient.h"
#include "Crc32SW.h"
#include <cassert>
#include <QtDebug>

#define MLINK_PREAMBLE 0x203d26d1

MLinkClient::MLinkClient() : g( 15 )
{
	_state = State::Disconnected;
	_error = Error::NoError;
	device = nullptr;
	sqNumCounter = sqNumNext = 0;
	r = 0;
	pongWaiting = false;
	QObject::connect( &timer, &QTimer::timeout, this, &MLinkClient::timeout );
}

MLinkClient::~MLinkClient()
{

}

MLinkClient::State MLinkClient::state() const
{
	return _state;
}

MLinkClient::Error MLinkClient::error() const
{
	return _error;
}

void MLinkClient::setIODevice( QIODevice* device )
{
	if( _state == State::Disconnected )
	{
		if( this->device )
		{
			QObject::disconnect( this->device, &QIODevice::readyRead, this, &MLinkClient::processBytes );
			QObject::disconnect( this->device, &QIODevice::bytesWritten, this, &MLinkClient::bytesWritten );
			QObject::disconnect( this->device, &QIODevice::aboutToClose, this, &MLinkClient::aboutToClose );
		}

		this->device = device;
		if( device )
		{
			QObject::connect( device, &QIODevice::readyRead, this, &MLinkClient::processBytes );
			QObject::connect( device, &QIODevice::bytesWritten, this, &MLinkClient::bytesWritten );
			QObject::connect( device, &QIODevice::aboutToClose, this, &MLinkClient::aboutToClose );
		}
	}
}

void MLinkClient::connectToHost()
{
	if( !device || _state != State::Disconnected || !device->isOpen() )
		return;

	assert( inCDataMap.size() == 0 );
	_state = State::Connecting;
	_error = Error::NoError;
	sendSynPacket();
	stateChanged( _state );
}

void MLinkClient::disconnectFromHost()
{
	if( !device || _state == State::Disconnected || _state == State::Disconnecting )
		return;

	if( _state == State::Connecting )
	{
		_state = State::Disconnecting;
		timer.start( 1000 );
		stateChanged( _state );
	}
	else
	{
		_state = State::Disconnecting;
		reqList.clear();
		sendFinPacket();
		stateChanged( _state );
	}
}

void MLinkClient::sendPacket( uint8_t type, QByteArray data )
{
	if( _state != State::Connected || data.size() > 255 || type > ( 255 - PacketType::User ) )
		return;

	Request req;
	req.type = type + PacketType::User;
	req.data = data;

	pushBackRequest( req );
}

bool MLinkClient::sendComplexData( uint8_t channelId, QByteArray data, QString name )
{
	if( _state != State::Connected || data.size() == 0 || outCDataMap.contains( channelId ) )
		return false;

	OutputCData cdata;
	cdata.name = name.toUtf8();
	if( cdata.name.size() > 250 )
		cdata.name = cdata.name.left( 250 );
	cdata.data = data;

	outCDataMap[channelId] = cdata;
	pushBackRequest( complexDataBeginRequest( channelId, cdata.name, data.size() ) );
	return true;
}

bool MLinkClient::cancelComplexDataSending( uint8_t channelId )
{
	if( _state != State::Connected || !outCDataMap.contains( channelId ) )
		return false;

	outCDataMap[channelId].needCancel = true;
	return true;
}

void MLinkClient::sendSynPacket()
{
	sqNumCounter = 0;
	sqNumNext = 0;
	r = ( ( uint64_t )qrand() << 32 ) | ( uint64_t )qrand();
	Header header{ MLINK_PREAMBLE, 8, PacketType::Syn, sqNumCounter++ };
	QByteArray packetData;
	packetData.append( reinterpret_cast< const char* >( &header ), sizeof( header ) );
	packetData.append( reinterpret_cast< const char* >( &r ), sizeof( r ) );
	addCrc( packetData );
	device->write( packetData );
	timer.start( 200 );
}

void MLinkClient::sendFinPacket()
{
	Header header{ MLINK_PREAMBLE, 8, PacketType::Fin, sqNumCounter++ };
	QByteArray packetData;
	packetData.append( reinterpret_cast< const char* >( &header ), sizeof( header ) );
	packetData.append( reinterpret_cast< const char* >( &r ), sizeof( r ) );
	addCrc( packetData );
	device->write( packetData );
	timer.start( 1000 );
}

MLinkClient::Request MLinkClient::complexDataBeginRequest( uint8_t id, QByteArray name, uint32_t size )
{
	Request req;
	req.type = PacketType::ComplexDataBegin;
	req.data.append( reinterpret_cast< const char* >( &id ), sizeof( id ) );
	req.data.append( reinterpret_cast< const char* >( &size ), sizeof( size ) );
	req.data.append( name );

	return req;
}

MLinkClient::Request MLinkClient::complexDataBeginAckRequest( uint8_t id, uint32_t g )
{
	Request req;
	req.type = PacketType::ComplexDataBeginAck;
	req.data.append( reinterpret_cast< const char* >( &id ), sizeof( id ) );
	req.data.append( reinterpret_cast< const char* >( &g ), sizeof( g ) );

	return req;
}
#include <QDebug>
MLinkClient::Request MLinkClient::nextComplexDataPartRequest( uint8_t id, OutputCData& cdata )
{
	uint8_t size;
	if( cdata.data.size() - cdata.numWritten >= 254 )
		size = 254;
	else
		size = cdata.data.size() - cdata.numWritten;
	if( size == 0 )
		return complexDataEndRequest( id, false );

	Request req;
	if( ++cdata.n == cdata.g && ( cdata.data.size() - cdata.numWritten - size ) )
		req.type = PacketType::ComplexDataNext, cdata.n = 0;
	else
		req.type = PacketType::ComplexData;
	req.data.append( reinterpret_cast< const char* >( &id ), sizeof( id ) );
	req.data.append( cdata.data.constData() + cdata.numWritten, size );
	cdata.numWritten += size;

	return req;
}

MLinkClient::Request MLinkClient::complexDataEndRequest( uint8_t id, bool canceled )
{
	Request req;
	req.type = PacketType::ComplexDataEnd;
	req.data.append( reinterpret_cast< const char* >( &id ), sizeof( id ) );
	uint8_t t = canceled;
	req.data.append( reinterpret_cast< const char* >( &t ), sizeof( t ) );

	return req;
}

void MLinkClient::pushBackRequest( Request& req )
{
	reqList.append( req );
	if( device->bytesToWrite() == 0 )
		bytesWritten();
}

void MLinkClient::pushFrontRequest( Request& req )
{
	reqList.push_front( req );
	if( device->bytesToWrite() == 0 )
		bytesWritten();
}

uint32_t MLinkClient::calcCrc( QByteArray& data )
{
	return Crc32SW::crc( reinterpret_cast< const uint8_t* >( data.constData() ), data.size(), 0 );
}

void MLinkClient::addCrc( QByteArray& data )
{
	uint32_t crc = calcCrc( data );
	data.append( reinterpret_cast< const char* >( &crc ), sizeof( crc ) );
}

void MLinkClient::closeLink( Error e )
{
	reqList.clear();
	inCDataMap.clear();
	outCDataMap.clear();
	timer.stop();
	pongWaiting = false;
	_state = State::Disconnected;
	_error = e;
	r = 0;
	if( e != Error::NoError )
		error( _error );
	stateChanged( _state );
	disconnected();
}

void MLinkClient::closeLink()
{
	reqList.clear();
	inCDataMap.clear();
	outCDataMap.clear();
	timer.stop();
	pongWaiting = false;
	_state = State::Disconnected;
	r = 0;
	stateChanged( _state );
	disconnected();
}

void MLinkClient::processBytes()
{
	while( true )
	{
		if( _state == State::Disconnected )
		{
			if( device )
				device->readAll();
			return;
		}

		if( device->bytesAvailable() < sizeof( Header ) + sizeof( uint32_t ) ) // Header + crc
			return;

		Header header;
		device->peek( reinterpret_cast< char* >( &header ), sizeof( header ) );
		if( header.preamble != MLINK_PREAMBLE )
		{
			device->read( 1 );
			continue;
		}

		if( ( quint64 )device->bytesAvailable() < sizeof( Header ) + sizeof( uint32_t ) + header.size )
			return;
		QByteArray packetData = device->read( sizeof( Header ) + sizeof( uint32_t ) + header.size );
		if( calcCrc( packetData.left( sizeof( Header ) + header.size ) ) != *reinterpret_cast< const uint32_t* >( packetData.right( sizeof( uint32_t ) ).constData() ) ) // crc error
			continue;
		if( _state == State::Connected )
		{
			if( header.sqNum != sqNumNext++ ) // sequence violation
			{
				_state = State::Disconnecting;
				_error = Error::SequenceViolationError;
				reqList.clear();
				error( Error::SequenceViolationError );
				stateChanged( _state );
				timer.start( 1000 );
				continue;
			}
			else
				processPacket( header, packetData );
		}
		else
			processPacket( header, packetData );
	}
}

void MLinkClient::processPacket( Header& header, QByteArray& packetData )
{
	if( _state == State::Connected )
	{
		pongWaiting = false;
		timer.start( 500 );
		if( header.type >= PacketType::User )
			newPacketAvailable( header.type - PacketType::User, packetData.mid( sizeof( Header ), header.size ) );
		else
		{
			switch( header.type )
			{
			case ComplexData:
			case ComplexDataNext:
			{
				uint8_t id = packetData.constData()[sizeof( Header )];
				assert( inCDataMap.contains( id ) == true );
				auto& cdata = inCDataMap[id];
				cdata.data.append( packetData.mid( sizeof( Header ) + sizeof( uint8_t ), header.size - 1 ) );
				if( cdata.size )
					complexDataReceivingProgress( id, cdata.name, ( float )cdata.data.size() / cdata.size );
				if( header.type == ComplexDataNext )
				{
					Request req;
					req.type = ComplexDataNextAck;
					req.data.append( id );
					pushBackRequest( req );
				}
				break;
			}
			case ComplexDataNextAck:
			{
				uint8_t id = packetData.constData()[sizeof( Header )];
				assert( outCDataMap.contains( id ) == true );
				auto& cdata = outCDataMap[id];
				if( cdata.needCancel )
					pushBackRequest( complexDataEndRequest( id, true ) );
				else
					pushBackRequest( nextComplexDataPartRequest( id, cdata ) );
			}
			case Pong:
				break;
			case Ping:
			{
				Request req;
				req.type = PacketType::Pong;
				pushFrontRequest( req );
				break;
			}
			case ComplexDataBeginAck:
			{
				uint8_t id = packetData.constData()[sizeof( Header )];
				assert( outCDataMap.contains( id ) == true );
				auto& cdata = outCDataMap[id];
				cdata.g = *reinterpret_cast< const uint32_t* >( packetData.constData() + sizeof( Header ) + sizeof( uint8_t ) );
				if( cdata.g == 0xFFFFFFFF ) // sending rejected
				{
					outCDataMap.remove( id );
					complexDataSendindCanceled( id );
					break;
				}
				if( cdata.needCancel )
					pushBackRequest( complexDataEndRequest( id, true ) );
				else
					pushBackRequest( nextComplexDataPartRequest( id, cdata ) );
				break;
			}
			case ComplexDataBegin:
			{
				InputCData cdata;
				uint8_t id = packetData.constData()[sizeof( Header )];
				cdata.size = *reinterpret_cast< const uint32_t* >( packetData.constData() + sizeof( Header ) + sizeof( uint8_t ) );
				cdata.name = packetData.mid( sizeof( Header ) + sizeof( uint8_t ) + sizeof( uint32_t ), header.size - sizeof( uint8_t ) - sizeof( uint32_t ) );
				inCDataMap[id] = cdata;
				pushBackRequest( complexDataBeginAckRequest( id, g ) );
				complexDataReceivingProgress( id, cdata.name, 0.0f );
				break;
			}
			case ComplexDataEnd:
			{
				uint8_t id = packetData.constData()[sizeof( Header )];
				assert( inCDataMap.contains( id ) == true );
				auto cdata = inCDataMap.take( id );
				uint8_t canceled = packetData.constData()[sizeof( Header ) + sizeof( uint8_t )];
				if( canceled )
					complexDataReceivingProgress( id, cdata.name, -1.0f );
				else
					newComplexPacketAvailable( id, cdata.name, cdata.data );
				break;
			}
			case Fin:
			{
				if( *reinterpret_cast< const uint64_t* >( packetData.constData() + sizeof( header ) ) != r )
					break;
				Header header{ MLINK_PREAMBLE, 8, PacketType::FinAck, sqNumCounter++ };
				QByteArray packetData;
				packetData.append( reinterpret_cast< const char* >( &header ), sizeof( header ) );
				packetData.append( reinterpret_cast< const char* >( &r ), sizeof( r ) );
				addCrc( packetData );
				device->write( packetData );
				closeLink( Error::RemoteHostClosedError );
				break;
			}
			default:
				assert( false );
				break;
			}
		}
	}
	else
	{
		if( header.type == PacketType::SynAck || header.type == PacketType::Syn || header.type == PacketType::FinAck )
		{
			if( *reinterpret_cast< const uint64_t* >( packetData.constData() + sizeof( Header ) ) != r )
				return;
			if( _state == State::Connecting )
			{
				if( header.type == PacketType::SynAck )
				{
					_state = State::Connected;
					sqNumNext = 1;
					stateChanged( _state );
					connected();
					timer.start( 500 );
				}
				else if( header.type == PacketType::Syn )
					timer.start( 1100 );
			}
			else // _state == State::Disconnecting
			{
				if( header.type == PacketType::FinAck )
					closeLink();
				else if( header.type == PacketType::SynAck )
					sendFinPacket();
				else if( header.type == PacketType::Syn )
					timer.start( 1100 );
			}
		}
	}
}

void MLinkClient::bytesWritten()
{
	if( reqList.isEmpty() )
		return;

	Request packet = reqList.takeFirst();
	Header header{ MLINK_PREAMBLE, ( uint8_t )packet.data.size(), packet.type, sqNumCounter++ };
	QByteArray packetData;
	packetData.append( reinterpret_cast< const char* >( &header ), sizeof( header ) );
	packetData.append( packet.data );
	addCrc( packetData );
	device->write( packetData );

	if( packet.type == PacketType::ComplexData )
	{
		uint8_t id = packet.data.constData()[0];
		OutputCData& cdata = outCDataMap[id];
		if( cdata.numWritten != cdata.data.size() )
			complexDataSendingProgress( id, cdata.name, ( float )cdata.numWritten / cdata.data.size() );
		if( cdata.needCancel )
			reqList.append( complexDataEndRequest( id, true ) );
		else
			reqList.append( nextComplexDataPartRequest( id, outCDataMap[id] ) );
	}
	else if( packet.type == PacketType::ComplexDataNext )
	{
		uint8_t id = packet.data.constData()[0];
		OutputCData& cdata = outCDataMap[id];
		complexDataSendingProgress( id, cdata.name, ( float )cdata.numWritten / cdata.data.size() );
	}
	else if( packet.type == PacketType::ComplexDataEnd )
	{
		uint8_t id = packet.data.constData()[0];
		OutputCData cdata = outCDataMap.take( id );

		if( cdata.needCancel )
			complexDataSendindCanceled( id );
		else
			complexDataSendingProgress( id, cdata.name, 1.0f );
	}
}

void MLinkClient::aboutToClose()
{
	if( _state != State::Disconnected && _state != State::Disconnecting )
		closeLink( Error::IODeviceClosedError );
}

void MLinkClient::timeout()
{
	if( _state == State::Connected )
	{
		if( pongWaiting ) // pong timeout
			closeLink( Error::ResponseTimeoutError );
		else
		{
			pongWaiting = true;
			Request req;
			req.type = PacketType::Ping;
			pushFrontRequest( req );
		}
	}
	else if( _state == State::Connecting )
		sendSynPacket();
	else if( _state == State::Disconnecting )
		closeLink();
}