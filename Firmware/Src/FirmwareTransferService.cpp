#include "FirmwareTransferService.h"
#include "Core/CriticalSectionLocker.h"
#include "FileSystem/Dir.h"
#include "_Sha1.h"
#include <string.h>

const char FirmwareTransferService::nameStr[25] = "FirmwareTransferProtocol";

FirmwareTransferService::FirmwareTransferService() : Thread( 1536 )
{
	mState = State::Stopped;
	err = Error::NoError;
	path = "/Temp/FirmwareTransferService/";
	callback = nullptr;
	timer.setParameter( this );
	socket = nullptr;
	n = 0;
}

FirmwareTransferService::~FirmwareTransferService()
{
	stopService();
	waitForStopped();
}

void FirmwareTransferService::setCallback( Callback* callback )
{
	if( mState != State::Stopped )
		return;
	this->callback = callback;
}

FirmwareTransferService::ServiceState FirmwareTransferService::state() const
{
	if( mState == State::Stopped )
		return ServiceState::Stopped;
	else if( mState == State::Stopping )
		return ServiceState::Stopping;
	return ServiceState::Working;
}
FirmwareTransferService::Error FirmwareTransferService::error() const
{
	return err;
}

bool FirmwareTransferService::startService( tprio_t prio /*= NORMALPRIO */ )
{
	if( mState != State::Stopped )
		return false;

	mState = State::Listening;
	setPriority( prio );
	if( !start() )
	{
		mState = State::Stopped;
		return false;
	}
	evtSource.broadcastFlags( ( eventflags_t )Event::StateChanged );
	return true;
}

void FirmwareTransferService::stopService()
{
	CriticalSectionLocker locker;
	if( mState == State::Stopping || mState == State::Stopped )
		return;

	mState = State::Stopping;
	evtSource.broadcastFlags( ( eventflags_t )Event::StateChanged );
	signalEventsI( StopRequestEvent );
}

bool FirmwareTransferService::waitForStopped( sysinterval_t timeout /*= TIME_INFINITE */ )
{
	CriticalSectionLocker locker;
	msg_t msg = MSG_OK;
	if( mState == State::Stopping )
		msg = waitingQueue.enqueueSelf( timeout );
	return msg == MSG_OK;
}

EventSourceRef FirmwareTransferService::eventSource()
{
	return &evtSource;
}

void FirmwareTransferService::main()
{
	err = Error::NoError;
	EventListener serverListener;
	server.eventSource().registerMask( &serverListener, ServerEvent );
	if( !server.listen( 42888 ) )
	{
		err = Error::ServerInnerError;
		goto End;
	}

	while( mState != State::Stopping )
	{
		eventmask_t em = waitAnyEvent( ALL_EVENTS );
		if( em & StopRequestEvent )
			break;
		if( em & ServerEvent )
		{
			if( !server.isListening() )
			{
				err = Error::ServerInnerError;
				break;
			}
			TcpSocket* newSocket;
			while( ( newSocket = server.nextPendingConnection() ) != nullptr )
			{
				if( socket || mState == State::Stopping )
				{
					newSocket->disconnect();
					delete newSocket;
				}
				else
				{
					socket = newSocket;
					socket->eventSource().registerMask( &socketListener, SocketEvent );
					em |= SocketEvent;
					em &= ~TimerEvent;
					getAndClearEvents( TimerEvent );
					setState( State::WaitHeader );
					timer.start( AuthTimeout );
				}
			}
		}
		if( em & SocketEvent && socket )
		{
			if( socket->isOpen() )
				processMainSocket();
			else
			{
				delete socket;
				socket = nullptr;
				setState( State::Listening );
				timer.stop();
				file.close();
			}
		}
		if( em & TimerEvent && socket )
		{
			socket->disconnect();
			delete socket;
			socket = nullptr;
			setState( State::Listening );
			timer.stop();
			file.close();
		}
	}
	server.close();
	if( socket )
		socket->disconnect();
	delete socket;
	socket = nullptr;
	timer.stop();
	file.close();

End:
	chSysLock();
	mState = State::Stopped;
	if( err == Error::NoError )
		evtSource.broadcastFlags( ( eventflags_t )Event::StateChanged );
	else
		evtSource.broadcastFlags( ( eventflags_t )Event::StateChanged | ( eventflags_t )Event::Error );
	waitingQueue.dequeueAll( MSG_OK );
}

void FirmwareTransferService::processMainSocket()
{
	uint32_t limit = socket->readAvailable();
	while( limit )
	{
		switch( mState )
		{
		case FirmwareTransferService::State::WaitHeader:
		{
			if( limit < sizeof( Header ) )
				return;
			Header header;
			if( socket->read( reinterpret_cast< uint8_t* >( &header ), sizeof( header ), TIME_IMMEDIATE ) != sizeof( header ) ||
				strncmp( nameStr, header.name, sizeof( nameStr ) ) != 0 )
			{
				removeSocket();
				return;
			}
			if( header.version != version )
			{
				header.version = version;
				socket->write( reinterpret_cast< uint8_t* >( &header ), sizeof( header ) );
				setState( State::WaitClose );
				timer.start( CloseTimeout );
				break;
			}
			socket->write( reinterpret_cast< uint8_t* >( &header ), sizeof( header ) );
			setState( State::Authorization );
			n = 0;
			limit -= sizeof( Header );
			break;
		}
		case FirmwareTransferService::State::Authorization:
			while( limit-- )
			{
				uint8_t c;
				if( !socket->read( &c, 1, TIME_IMMEDIATE ) || n >= 64 )
				{
					removeSocket();
					return;
				}
				if( c == 0 )
				{
					buffer[n] = 0;
					if( password == ( const char* )buffer )
					{
						c = AuthOk;
						std::string str;
						str.append( 1, ( char )c );
						str.append( callback->firmwareVersion() );
						str.append( 1, '\0' );
						str.append( callback->bootloaderVersion() );
						str.append( 1, '\0' );
						socket->write( ( const uint8_t* )str.c_str(), str.size() );
						setState( State::WaitCommand );
						timer.start( InactiveTimeout );
						break;
					}
					else
					{
						c = AuthFailed;
						socket->write( &c, 1 );
						setState( State::WaitClose );
						timer.start( CloseTimeout );
						break;
					}
				}
				else
					buffer[n++] = c;
			}
			break;
		case FirmwareTransferService::State::WaitCommand:
		{
			uint8_t c;
			timer.start( InactiveTimeout );
			if( !socket->read( &c, 1, TIME_IMMEDIATE ) )
			{
				removeSocket();
				return;
			}
			--limit;
			if( c == StartFirmwareTransfer )
				setState( State::WaitFirmwareDesc );
			else if( c == StartBootloaderTransfer )
				setState( State::WaitBootloaderDesc );
			else if( c == Restart )
				callback->restartDevice();
			break;
		}
		case FirmwareTransferService::State::WaitFirmwareDesc:
		case FirmwareTransferService::State::WaitBootloaderDesc:
		{
			timer.start( InactiveTimeout );
			if( limit < sizeof( Desc ) )
				return;
			Desc desc;
			if( !socket->read( reinterpret_cast< uint8_t* >( &desc ), sizeof( desc ), TIME_IMMEDIATE ) )
			{
				removeSocket();
				return;
			}
			limit -= sizeof( Desc );
			for( int i = 0; i < 20; ++i )
				sprintf( ( char* )buffer + i * 2, "%02x", desc.hash[i] );
			Dir().mkpath( path.c_str() );
			file.setFileName( path + ( char* )buffer );
			if( file.open( FileSystem::OpenModeFlag::OpenExisting | FileSystem::OpenModeFlag::ReadWrite ) )
			{
				n = desc.size - file.size();
				if( n > desc.size ) // overflow
				{
					file.truncate();
					n = desc.size;
				}
				else
					file.seek( file.size() );
			}
			else
			{
				file.open( FileSystem::OpenModeFlag::CreateNew | FileSystem::OpenModeFlag::ReadWrite );
				n = desc.size;
			}
			if( file.lastError() != FileSystem::Error::NoError )
			{
				removeSocket();
				return;
			}
			desc.size -= n;
			socket->write( ( const uint8_t* )&desc.size, sizeof( desc.size ) );
			if( mState == State::WaitFirmwareDesc )
				setState( State::ReceiveFirmware );
			else
				setState( State::ReceiveBootloader );
			break;
		}
		case FirmwareTransferService::State::ReceiveFirmware:
		case FirmwareTransferService::State::ReceiveBootloader:
		{
			timer.start( InactiveTimeout );
			do
			{
				uint32_t partSize = limit;
				if( partSize > sizeof( buffer ) )
					partSize = sizeof( buffer );
				if( partSize > n )
					partSize = n;
				if( socket->read( buffer, partSize, TIME_IMMEDIATE ) != partSize ||
					file.write( buffer, partSize ) != partSize )
				{
					file.close();
					removeSocket();
					return;
				}
				n -= partSize;
				limit -= partSize;
			} while( limit && n );
			if( n == 0 )
			{
				file.flush();
				file.seek( 0 );
				std::unique_ptr< SHA1 > sha1( new SHA1 );
				if( !sha1 || file.lastError() != FileSystem::NoError )
				{
					file.close();
					removeSocket();
					return;
				}
				sha1->reset();
				n = file.size();
				do
				{
					uint32_t partSize = n;
					if( partSize > sizeof( buffer ) )
						partSize = sizeof( buffer );
					if( file.read( buffer, partSize ) != partSize )
					{
						file.close();
						removeSocket();
						return;
					}
					sha1->addBytes( ( const char* )buffer, partSize );
					n -= partSize;
				} while( n );
				file.close();
				auto d = sha1->result();
				for( int i = 0; i < 20; ++i )
					sprintf( ( char* )buffer + i * 2, "%02x", d.data[i] );
				std::string name = file.fileName().substr( file.fileName().find_last_of( "/" ) + 1 );
				if( name == ( const char* )buffer )
				{
					uint8_t c = Command::TransferOk;
					socket->write( &c, 1 );
					if( mState == State::ReceiveFirmware )
						callback->firmwareDownloaded( file.fileName() );
					else
						callback->bootloaderDownloaded( file.fileName() );
				}
				else
				{
					uint8_t c = Command::TransferFailed;
					socket->write( &c, 1 );
				}
				Dir().remove( file.fileName().c_str() );
				setState( State::WaitCommand );
			}
			break;
		}
		case FirmwareTransferService::State::WaitClose:
		case FirmwareTransferService::State::Stopping:
		default:
			socket->read( nullptr, limit, TIME_IMMEDIATE );
			return;
		}
	}
}

void FirmwareTransferService::removeSocket()
{
	socket->disconnect();
	delete socket;
	socket = nullptr;
	timer.stop();
	setState( State::Listening );
}

void FirmwareTransferService::setState( State newState )
{
	CriticalSectionLocker locker;
	auto prevState = state();
	if( mState != State::Stopping )
		mState = newState;
	auto newServiceState = state();
	if( prevState != newServiceState )
		evtSource.broadcastFlags( ( eventflags_t )newServiceState );
}

void FirmwareTransferService::setError( Error err )
{
	this->err = err;
	if( err != Error::NoError )
		evtSource.broadcastFlags( ( eventflags_t )Event::Error );
}

void FirmwareTransferService::timerCallback()
{
	chSysLockFromISR();
	signalEventsI( TimerEvent );
	chSysUnlockFromISR();
}
