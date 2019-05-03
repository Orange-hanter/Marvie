#include "FirmwareTransferClient.h"
#include <QCryptographicHash>
#include <QHostAddress>
#include <QDebug>
#include <string.h>

Q_DECLARE_METATYPE( FirmwareTransferClient::State )
Q_DECLARE_METATYPE( FirmwareTransferClient::Error )

FirmwareTransferClient::FirmwareTransferClient()
{
	qRegisterMetaType< FirmwareTransferClient::State >();
	qRegisterMetaType< FirmwareTransferClient::Error >();

	currentState = State::Disconnected;
	err = Error::NoError;
	socketErr = QAbstractSocket::SocketError::UnknownSocketError;
	socket = nullptr;
	offset = -1;
	QObject::connect( &timer, &QTimer::timeout, this, &FirmwareTransferClient::timeout );
}

FirmwareTransferClient::~FirmwareTransferClient()
{
	disconnectFromHost();
}

void FirmwareTransferClient::connectToHost( QHostAddress addr, QString password )
{
	if( socket )
		return;

	this->password = password;
	err = Error::NoError;
	socket = new QTcpSocket;
	QObject::connect( socket, &QTcpSocket::connected, this, &FirmwareTransferClient::socketConnected );
	QObject::connect( socket, &QTcpSocket::disconnected, this, &FirmwareTransferClient::socketDisconnected );
	QObject::connect( socket, &QTcpSocket::readyRead, this, &FirmwareTransferClient::socketReadyRead );
	QObject::connect( socket, &QTcpSocket::bytesWritten, this, &FirmwareTransferClient::socketBytesWritten );
	QObject::connect( socket, static_cast< void( QTcpSocket::* )( QAbstractSocket::SocketError ) >( &QTcpSocket::error ), this, static_cast< void( FirmwareTransferClient::* )( QAbstractSocket::SocketError ) >( &FirmwareTransferClient::socketError ) );
	socket->connectToHost( addr, 42888 );
	timer.start( ConnectingTimeout );
	setState( State::Connecting );
}

void FirmwareTransferClient::disconnectFromHost()
{
	if( !socket )
		return;

	removeSocket();
	timer.stop();
	currentState = State::Disconnected;
	stateChanged( currentState );
}

FirmwareTransferClient::State FirmwareTransferClient::state()
{
	return currentState;
}

FirmwareTransferClient::Error FirmwareTransferClient::error()
{
	return err;
}

QAbstractSocket::SocketError FirmwareTransferClient::socketError()
{
	return socketErr;
}

QString FirmwareTransferClient::firmwareVersion()
{
	return firmwareVer;
}

QString FirmwareTransferClient::bootloaderVersion()
{
	return bootloaderVer;
}

void FirmwareTransferClient::setFirmware( QByteArray binaryData )
{
	if( currentState == State::FirmwareUploading || currentState == State::BootloaderUploading )
		return;

	firmwareData = binaryData;
}

void FirmwareTransferClient::setBootloader( QByteArray binaryData )
{
	if( currentState == State::FirmwareUploading || currentState == State::BootloaderUploading )
		return;

	bootloaderData = binaryData;
}

void FirmwareTransferClient::resetFirmware()
{
	if( currentState == State::FirmwareUploading )
		return;

	firmwareData.clear();
}

void FirmwareTransferClient::resetBootloader()
{
	if( currentState == State::BootloaderUploading )
		return;

	bootloaderData.clear();
}

void FirmwareTransferClient::upload()
{
	if( currentState != State::Connected )
		return;

	if( !firmwareData.isEmpty() )
	{
		startFirmwareUploading();
		return;
	}
	if( !bootloaderData.isEmpty() )
	{
		startBootloaderUploading();
		return;
	}
}

void FirmwareTransferClient::remoteRestart()
{
	if( currentState != State::Connected && currentState != State::UploadComplete )
		return;

	Command c = Command::Restart;
	socket->write( ( const char* )&c, 1 );
	timer.start( RemoteRestartTimeout );
	setState( State::RestartSending );
}

void FirmwareTransferClient::socketConnected()
{
	Header header;
	strncpy( header.name, nameStr, sizeof( nameStr ) );
	header.version = version;
	socket->write( ( const char* )&header, sizeof( header ) );
	timer.start( ResponseTimeout );
}

void FirmwareTransferClient::socketDisconnected()
{
	removeSocket();
	timer.stop();
	setState( State::Disconnected );
}

void FirmwareTransferClient::socketError( QAbstractSocket::SocketError err )
{
	removeSocket();
	timer.stop();
	if( currentState != State::RestartSending )
	{
		socketErr = err;
		this->err = Error::SocketError;
		error( this->err );
	}
	setState( State::Disconnected );
}

void FirmwareTransferClient::setState( State state )
{
	bool changed = state != currentState;
	currentState = state;
	if( changed )
		stateChanged( currentState );
}

void FirmwareTransferClient::removeSocket()
{
	QObject::disconnect( socket, &QTcpSocket::connected, this, &FirmwareTransferClient::socketConnected );
	QObject::disconnect( socket, &QTcpSocket::disconnected, this, &FirmwareTransferClient::socketDisconnected );
	QObject::disconnect( socket, &QTcpSocket::readyRead, this, &FirmwareTransferClient::socketReadyRead );
	QObject::disconnect( socket, &QTcpSocket::bytesWritten, this, &FirmwareTransferClient::socketBytesWritten );
	QObject::disconnect( socket, static_cast< void( QTcpSocket::* )( QAbstractSocket::SocketError ) >( &QTcpSocket::error ), this, static_cast< void( FirmwareTransferClient::* )( QAbstractSocket::SocketError ) >( &FirmwareTransferClient::socketError ) );
	socket->deleteLater();
	socket = nullptr;
}

void FirmwareTransferClient::startFirmwareUploading()
{
	offset = -1;
	QCryptographicHash hash( QCryptographicHash::Sha1 );
	hash.addData( firmwareData );
	auto h = hash.result();
	Desc desc;
	qCopy( h.begin(), h.end(), desc.hash );
	desc.size = firmwareData.size();
	Command c = Command::StartFirmwareTransfer;
	socket->write( QByteArray( ( char* )&c, 1 ) + QByteArray( ( const char* )&desc, sizeof( desc ) ) );
	timer.start( ResponseTimeout );
	setState( State::FirmwareUploading );
	progress( 0 );
}

void FirmwareTransferClient::startBootloaderUploading()
{
	offset = -1;
	QCryptographicHash hash( QCryptographicHash::Sha1 );
	hash.addData( bootloaderData );
	auto h = hash.result();
	Desc desc;
	qCopy( h.begin(), h.end(), desc.hash );
	desc.size = bootloaderData.size();
	Command c = Command::StartBootloaderTransfer;
	socket->write( QByteArray( ( char* )&c, 1 ) + QByteArray( ( const char* )&desc, sizeof( desc ) ) );
	timer.start( ResponseTimeout );
	setState( State::BootloaderUploading );
	progress( 0 );
}

void FirmwareTransferClient::socketReadyRead()
{
	while( socket->bytesAvailable() )
	{
		switch( currentState )
		{
		case FirmwareTransferClient::State::Connecting:
		{
			if( socket->bytesAvailable() < sizeof( Header ) )
				return;
			Header header;
			socket->read( ( char* )&header, sizeof( header ) );
			if( QString( nameStr ) != header.name || header.version != version )
			{
				removeSocket();
				timer.stop();
				err = Error::VersionError;
				error( err );
				setState( State::Disconnected );
			}
			socket->write( password.toUtf8().append( '\0' ) );
			setState( State::Authorization );
			break;
		}
		case FirmwareTransferClient::State::Authorization:
		{
			auto data = socket->peek( 255 );
			if( ( uint8_t )data[0] == Command::AuthFailed )
			{
				removeSocket();
				timer.stop();
				err = Error::AuthError;
				error( err );
				setState( State::Disconnected );
			}
			if( data.count( '\0' ) != 3 )
				return;
			auto list = data.split( '\0' );
			firmwareVer = list[1];
			bootloaderVer = list[2];
			socket->read( data.size() );
			setState( State::Connected );
			break;
		}
		case FirmwareTransferClient::State::FirmwareUploading:
		{
			if( offset == -1 )
			{
				uint32_t n;
				socket->read( ( char* )&n, sizeof( n ) );
				offset = ( int )n;
				while( offset != firmwareData.size() )
				{
					int partSize = firmwareData.size() - offset;
					if( partSize > 1024 )
						partSize = 1024;
					socket->write( firmwareData.mid( offset, partSize ) );
					offset += partSize;
				}
				offset = ( int )n;
			}
			else
			{
				uint8_t c;
				socket->read( ( char* )&c, sizeof( c ) );
				if( c == Command::TransferOk )
				{
					if( !bootloaderData.isEmpty() )
						startBootloaderUploading();
					else
					{
						setState( State::UploadComplete );
						uploadComplete();
					}
				}
				else
					startFirmwareUploading();
			}
			break;
		}
		case FirmwareTransferClient::State::BootloaderUploading:
		{
			if( offset == -1 )
			{
				uint32_t n;
				socket->read( ( char* )&n, sizeof( n ) );
				offset = ( int )n;
				while( offset != bootloaderData.size() )
				{
					int partSize = bootloaderData.size() - offset;
					if( partSize > 1024 )
						partSize = 1024;
					socket->write( bootloaderData.mid( offset, partSize ) );
					offset += partSize;
				}
				offset = ( int )n;
			}
			else
			{
				uint8_t c;
				socket->read( ( char* )&c, sizeof( c ) );
				if( c == Command::TransferOk )
				{
					setState( State::UploadComplete );
					uploadComplete();
				}
				else
					startBootloaderUploading();
			}
			break;
		}
		default:
			socket->readAll();
			break;
		}
	}
}

void FirmwareTransferClient::socketBytesWritten( qint64 n )
{
	if( ( currentState == State::FirmwareUploading || currentState == State::BootloaderUploading ) && offset != -1 )
	{
		auto total = ( currentState == State::FirmwareUploading ? firmwareData.size() : bootloaderData.size() ) - offset;
		float p = ( float )( total - socket->bytesToWrite() ) / total;
		if( p < 0.0f )
			p = 0;
		if( p > 1.0f )
			p = 1.0f;
		progress( p );
	}
}

void FirmwareTransferClient::timeout()
{
	removeSocket();
	timer.stop();
	if( currentState == State::RestartSending )
		setState( State::Disconnected );
	else
	{
		err = Error::TimeoutError;
		error( err );
		setState( State::Disconnected );
	}
}
