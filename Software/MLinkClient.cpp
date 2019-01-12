#include "MLinkClient.h"
#include "Crc32SW.h"
#include <QHostAddress>
#include <cassert>
#include <QtDebug>

#define MLINK_PREAMBLE 0x203d26d1

Q_DECLARE_METATYPE( MLinkClient::State )
Q_DECLARE_METATYPE( MLinkClient::Error )

MLinkClient::MLinkClient()
{
	qRegisterMetaType< MLinkClient::State >();
	qRegisterMetaType< MLinkClient::Error >();

	_state = State::Disconnected;
	_error = Error::NoError;
	idCounter = 0;
	socket = nullptr;
	QObject::connect( &timer, &QTimer::timeout, this, &MLinkClient::timeout );
	QObject::connect( &pingTimer, &QTimer::timeout, this, &MLinkClient::pingTimeout );
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

void MLinkClient::setAuthorizationData( QString accountName, QString accountPassword )
{
	if( accountName.size() + accountPassword.size() + 2 > MSS )
		return;

	this->accountName = accountName;
	this->accountPassword = accountPassword;
}

void MLinkClient::connectToHost( QHostAddress address )
{
	if( _state != State::Disconnected )
		return;

	assert( inCDataMap.size() == 0 );
	_state = State::Connecting;
	_error = Error::NoError;
	socket = new QTcpSocket;
	socket->connectToHost( address, 16021 );
	QObject::connect( socket, &QTcpSocket::connected, this, &MLinkClient::socketConnected );
	QObject::connect( socket, &QTcpSocket::readyRead, this, &MLinkClient::processBytes );
	QObject::connect( socket, &QTcpSocket::bytesWritten, this, &MLinkClient::bytesWritten );
	QObject::connect( socket, &QTcpSocket::disconnected, this, &MLinkClient::socketDisconnected );
	QObject::connect( socket, static_cast< void( QTcpSocket::* )( QAbstractSocket::SocketError ) >( &QTcpSocket::error ), this, &MLinkClient::socketError );
	stateChanged( _state );
}

void MLinkClient::disconnectFromHost()
{
	if( !socket || _state == State::Disconnected || _state == State::Disconnecting )
		return;
	if( _state == State::Connecting )
	{
		_state = State::Disconnected;
		socket->disconnect( this );
		socket->deleteLater();
		socket = nullptr;
		stateChanged( _state );
		return;
	}

	_state = State::Disconnecting;
	reqList.clear();
	pingTimer.stop();
	socket->disconnectFromHost();
	stateChanged( _state );
}

void MLinkClient::sendPacket( uint8_t type, QByteArray data )
{
	if( _state != State::Connected || data.size() > MSS || type > ( 255 - PacketType::User ) )
		return;

	Request req;
	req.type = type + PacketType::User;
	req.data = data;

	pushBackRequest( req );
}

bool MLinkClient::sendChannelData( uint8_t channel, QByteArray data, QString name )
{
	if( _state != State::Connected || data.size() == 0 || outCDataMap.contains( channel ) )
		return false;

	OutputCData cdata;
	if( ++idCounter == 0 )
		idCounter = 1;
	cdata.id = idCounter;
	cdata.name = name.toUtf8().left( ( DataChannelMSS - ( sizeof( uint32_t ) + sizeof( uint8_t ) ) ) );
	cdata.data = data;

	outCDataMap[channel] = cdata;
	pushBackRequest( openChannelRequest( channel, cdata.id, cdata.name, data.size() ) );
	return true;
}

bool MLinkClient::cancelChannelDataSending( uint8_t channel )
{
	if( _state != State::Connected || !outCDataMap.contains( channel ) )
		return false;

	uint32_t id = outCDataMap[channel].id;
	for( auto i = reqList.begin(); i != reqList.end(); ++i )
	{
		auto& req = *i;
		if( req.type == PacketType::OpenChannel || req.type == PacketType::ChannelData || req.type == PacketType::CloseChannel )
		{
			const ChannelHeader* chHeader = reinterpret_cast< const ChannelHeader* >( req.data.constData() );
			if( chHeader->ch != channel || chHeader->id != id )
				continue;

			if( req.type == PacketType::ChannelData )
			{
				reqList.erase( i );
				pushBackRequest( closeDataChannelRequest( channel, id, true ) );
				outCDataMap.remove( channel );
			}
			else if( req.type == PacketType::OpenChannel )
			{
				reqList.erase( i );
				outCDataMap.remove( channel );
			}
			else
			{
				req.data[( int )sizeof( ChannelHeader )] = 1;
				outCDataMap.remove( channel );
			}
			break;
		}
	}

	return true;
}

bool MLinkClient::cancelChannelDataReceiving( uint8_t channel )
{
	if( _state != State::Connected || !inCDataMap.contains( channel ) )
		return false;

	pushBackRequest( remoteCloseDataChannelRequest( channel, inCDataMap[channel].id ) );
	inCDataMap.remove( channel );
	return true;
}

void MLinkClient::sendAuth()
{
	Request req;
	req.type = PacketType::AuthAck;
	req.data.append( accountName.toUtf8() );
	req.data.append( '\0' );
	req.data.append( accountPassword.toUtf8() );
	req.data.append( '\0' );
	pushBackRequest( req );
}

MLinkClient::Request MLinkClient::openChannelRequest( uint8_t channel, uint32_t id, QByteArray name, uint32_t size )
{
	Request req;
	req.type = PacketType::OpenChannel;
	ChannelHeader chHeader{ id, channel };
	req.data.append( reinterpret_cast< const char* >( &chHeader ), sizeof( chHeader ) );
	req.data.append( reinterpret_cast< const char* >( &size ), sizeof( size ) );
	req.data.append( name );
	req.data.append( '\0' );

	return req;
}

MLinkClient::Request MLinkClient::nextDataChannelPartRequest( uint8_t channel, OutputCData& cdata )
{
	uint16_t size;
	if( cdata.data.size() - cdata.numWritten > DataChannelMSS )
		size = DataChannelMSS;
	else
		size = cdata.data.size() - cdata.numWritten;
	if( size == 0 )
		return closeDataChannelRequest( channel, cdata.id, false );

	Request req;
	req.type = PacketType::ChannelData;
	ChannelHeader chHeader{ cdata.id, channel };
	req.data.append( reinterpret_cast< const char* >( &chHeader ), sizeof( chHeader ) );
	req.data.append( cdata.data.constData() + cdata.numWritten, size );
	cdata.numWritten += size;

	return req;
}

MLinkClient::Request MLinkClient::closeDataChannelRequest( uint8_t channel, uint32_t id, bool canceled )
{
	Request req;
	req.type = PacketType::CloseChannel;
	ChannelHeader chHeader{ id, channel };
	uint8_t t = canceled;
	req.data.append( reinterpret_cast< const char* >( &chHeader ), sizeof( chHeader ) );
	req.data.append( reinterpret_cast< const char* >( &t ), sizeof( t ) );

	return req;
}

MLinkClient::Request MLinkClient::remoteCloseDataChannelRequest( uint8_t channel, uint32_t id )
{
	Request req;
	req.type = PacketType::RemoteCloseChannel;
	ChannelHeader chHeader{ id, channel };
	req.data.append( reinterpret_cast< const char* >( &chHeader ), sizeof( chHeader ) );

	return req;
}

void MLinkClient::pushBackRequest( Request& req )
{
	reqList.append( req );
	if( socket->bytesToWrite() == 0 )
		bytesWritten();
}

void MLinkClient::pushFrontRequest( Request& req )
{
	reqList.push_front( req );
	if( socket->bytesToWrite() == 0 )
		bytesWritten();
}

void MLinkClient::closeLink( Error e )
{
	reqList.clear();
	inCDataMap.clear();
	outCDataMap.clear();
	timer.stop();
	pingTimer.stop();
	_state = State::Disconnected;
	_error = e;
	socket->disconnect( this );
	socket->deleteLater();
	socket = nullptr;
	if( e != Error::NoError )
		error( _error );
	stateChanged( _state );
	disconnected();
}

void MLinkClient::processBytes()
{
	int limit = 0;
	if( socket )
		limit = socket->bytesAvailable();
	while( limit > 0 )
	{
		if( socket->bytesAvailable() < sizeof( Header ) )
			return;

		Header header;
		socket->peek( reinterpret_cast< char* >( &header ), sizeof( header ) );
		if( header.preamble != MLINK_PREAMBLE )
		{
			closeLink( Error::ClientInnerError );
			return;
		}

		if( ( quint64 )socket->bytesAvailable() < sizeof( Header ) + header.size )
			return;
		socket->read( reinterpret_cast< char* >( &header ), sizeof( header ) );
		processPacket( header, socket->read( header.size ) );
		limit -= sizeof( header ) + header.size;
	}
}

void MLinkClient::processPacket( Header& header, QByteArray& packetData )
{
	if( _state == State::Connected )
	{
		timer.start( MaxPacketTransferInterval * 3 );
		if( header.type >= PacketType::User )
			newPacketAvailable( header.type - PacketType::User, packetData );
		else
		{
			switch( header.type )
			{
			case ChannelData:
			{
				const ChannelHeader* chHeader = reinterpret_cast< const ChannelHeader* >( packetData.constData() );
				if( !inCDataMap.contains( chHeader->ch ) || inCDataMap[chHeader->ch].id != chHeader->id )
					break;
				auto& cdata = inCDataMap[chHeader->ch];
				cdata.data.append( packetData.right( header.size - sizeof( ChannelHeader ) ) );
				if( cdata.size )
					channeDataReceivingProgress( chHeader->ch, cdata.name, ( float )cdata.data.size() / cdata.size );
				break;
			}
			case IAmAlive:
				break;
			case OpenChannel:
			{
				InputCData cdata;
				const ChannelHeader* chHeader = reinterpret_cast< const ChannelHeader* >( packetData.constData() );
				cdata.id = chHeader->id;
				cdata.size = *reinterpret_cast< const uint32_t* >( packetData.constData() + sizeof( ChannelHeader ) );
				cdata.name = QString( packetData.constData() + sizeof( ChannelHeader ) + sizeof( uint32_t ) );
				assert( !inCDataMap.contains( chHeader->ch ) );
				inCDataMap[chHeader->ch] = cdata;
				channeDataReceivingProgress( chHeader->ch, cdata.name, 0.0f );
				break;
			}
			case CloseChannel:
			{
				const ChannelHeader* chHeader = reinterpret_cast< const ChannelHeader* >( packetData.constData() );
				if( !inCDataMap.contains( chHeader->ch ) || inCDataMap[chHeader->ch].id != chHeader->id )
					break;
				auto cdata = inCDataMap.take( chHeader->ch );
				uint8_t canceled = packetData.constData()[sizeof( ChannelHeader )];
				if( canceled )
					channeDataReceivingProgress( chHeader->ch, cdata.name, -1.0f );
				else
					newChannelDataAvailable( chHeader->ch, cdata.name, cdata.data );
				break;
			}
			case RemoteCloseChannel:
			{
				const ChannelHeader* chHeader = reinterpret_cast< const ChannelHeader* >( packetData.constData() );
				for( auto i = reqList.begin(); i != reqList.end(); ++i )
				{
					auto& req = *i;
					if( req.type == PacketType::OpenChannel || req.type == PacketType::ChannelData || req.type == PacketType::CloseChannel )
					{
						const ChannelHeader* reqChHeader = reinterpret_cast< const ChannelHeader* >( req.data.constData() );
						if( reqChHeader->ch != chHeader->ch || reqChHeader->id != chHeader->id )
							continue;

						reqList.erase( i );
						break;
					}
				}
				auto i = outCDataMap.find( chHeader->ch );
				if( i != outCDataMap.end() && i.value().id == chHeader->id )
				{
					QString name = i.value().name;
					outCDataMap.erase( i );
					channelDataSendingProgress( chHeader->ch, name, -1.0f );
				}
				break;
			}
			default:
				assert( false );
				break;
			}
		}
	}
	else if( _state == State::Authorizing )
	{
		switch( header.type )
		{
		case PacketType::AuthAck:
		{
			_state = State::Connected;
			timer.start( MaxPacketTransferInterval * 3 );
			pingTimer.start( PingInterval );
			stateChanged( _state );
			connected();
			break;
		}
		case PacketType::AuthFail:			
			closeLink( Error::AuthenticationError );
			break;
		default:
			break;
		}
	}
}

void MLinkClient::bytesWritten()
{
	if( reqList.isEmpty() )
		return;

	if( _state == State::Connected )
		pingTimer.start( PingInterval );

	Request packet = reqList.takeFirst();
	Header header{ MLINK_PREAMBLE, ( uint16_t )packet.data.size(), packet.type };
	QByteArray packetData;
	packetData.append( reinterpret_cast< const char* >( &header ), sizeof( header ) );
	packetData.append( packet.data );
	socket->write( packetData );

	if( packet.type == PacketType::ChannelData || packet.type == PacketType::OpenChannel )
	{
		const ChannelHeader* chHeader = reinterpret_cast< const ChannelHeader* >( packet.data.constData() );
		OutputCData& cdata = outCDataMap[chHeader->ch];
		if( cdata.id != chHeader->id )
			return;
		if( cdata.numWritten != cdata.data.size() )
			channelDataSendingProgress( chHeader->ch, cdata.name, ( float )cdata.numWritten / cdata.data.size() );
		reqList.append( nextDataChannelPartRequest( chHeader->ch, cdata ) );
	}
	else if( packet.type == PacketType::CloseChannel )
	{
		const ChannelHeader* chHeader = reinterpret_cast< const ChannelHeader* >( packet.data.constData() );
		if( !outCDataMap.contains( chHeader->ch ) || outCDataMap[chHeader->ch].id != chHeader->id )
			return;
		OutputCData cdata = outCDataMap.take( chHeader->ch );
		channelDataSendingProgress( chHeader->ch, cdata.name, 1.0f );
	}
}

void MLinkClient::socketConnected()
{
	_state = State::Authorizing;
	sendAuth();
	timer.start( MaxPacketTransferInterval * 2 );
	stateChanged( _state );
}

void MLinkClient::socketDisconnected()
{
	if( _state == State::Disconnecting )
		closeLink( Error::NoError );
	else
		closeLink( Error::RemoteHostClosedError );
}

void MLinkClient::socketError( QAbstractSocket::SocketError socketError )
{
	socketDisconnected();
}

void MLinkClient::timeout()
{
	if( _state == State::Connected || _state == State::Authorizing )
		closeLink( Error::ResponseTimeoutError );
}

void MLinkClient::pingTimeout()
{
	assert( _state == State::Connected );
	Request req;
	req.type = PacketType::IAmAlive;
	pushFrontRequest( req );
}