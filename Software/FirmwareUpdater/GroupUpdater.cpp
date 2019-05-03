#include "GroupUpdater.h"
#include <QDateTime>

GroupUpdater::GroupUpdater()
{
	started = false;
	maxConnection = 10;
	updatePolicy = UpdatePolicy::IfNewer;
	last = 0;
	nCompleted = 0;
	timer.setSingleShot( true );

	QObject::connect( &timer, &QTimer::timeout, this, &GroupUpdater::timeout );
}

GroupUpdater::~GroupUpdater()
{
	stop();
}

void GroupUpdater::setMaxConnections( int num )
{
	if( started )
		return;

	if( num < 1 )
		num = 1;
	maxConnection = num;
}

void GroupUpdater::setUpdatePolicy( UpdatePolicy policy )
{
	if( started )
		return;
	this->updatePolicy = policy;
}

void GroupUpdater::setHostList( const QVector< Host >& hosts )
{
	if( started )
		return;

	this->hosts = hosts;
}

void GroupUpdater::setHostList( QVector< Host >&& hosts )
{
	if( started )
		return;

	this->hosts = std::move( hosts );
	hostStates = QVector< State >( this->hosts.size(), State::InQueue );
}

void GroupUpdater::addFirmware( QString version, QByteArray binaryData )
{
	if( started )
		return;

	firmwares.insert( version, binaryData );
}

void GroupUpdater::addBootloader( QString version, QByteArray binaryData )
{
	if( started )
		return;

	bootloaders.insert( version, binaryData );
}

void GroupUpdater::clear()
{
	if( started )
		return;

	firmwares.clear();
	bootloaders.clear();
}

void GroupUpdater::start()
{
	if( started || ( firmwares.isEmpty() && bootloaders.isEmpty() ) )
		return;

	for( int i = 0; i < hosts.size(); ++i )
	{
		hostStates[i] = State::InQueue;
		hostStateChanged( i, HostState::InQueue );
		hostProgress( i, 0.0f );
	}
	last = nCompleted = 0;
	for( int i = 0; i < maxConnection && i < hosts.size(); ++i )
	{
		auto client = new FirmwareTransferClient;
		QObject::connect( client, &FirmwareTransferClient::stateChanged, this, &GroupUpdater::stateChanged );
		QObject::connect( client, &FirmwareTransferClient::progress, this, &GroupUpdater::progress );
		client->setProperty( "hostIndex", i );
		activeClientSet.insert( client );
		++last;

		hostStates[i] = State::Updating;
		client->connectToHost( hosts[i].addr, hosts[i].password );
	}
	started = true;
}

void GroupUpdater::stop()
{
	if( !started )
		return;

	for( auto i = activeClientSet.begin(); i != activeClientSet.end(); ++i )
	{
		( *i )->blockSignals( true );
		( *i )->deleteLater();
	}
	activeClientSet.clear();
	endTimePointIndexMap.clear();
	timer.stop();
	started = false;
}

bool GroupUpdater::isStarted()
{
	return started;
}

void GroupUpdater::stateChanged( FirmwareTransferClient::State state )
{
	FirmwareTransferClient* client = reinterpret_cast< FirmwareTransferClient* >( sender() );
	int index = client->property( "hostIndex" ).toInt();
	switch( state )
	{
	case FirmwareTransferClient::State::Disconnected:
	{
		switch( client->error() )
		{
		case FirmwareTransferClient::Error::AuthError:
			++nCompleted;
			hostStates[index] = State::Completed;
			hostStateChanged( index, HostState::AuthError );
			reassign( client );
			return;
		case FirmwareTransferClient::Error::VersionError:
			++nCompleted;
			hostStates[index] = State::Completed;
			hostStateChanged( index, HostState::VersionError );
			reassign( client );
			return;
		case FirmwareTransferClient::Error::SocketError:
			if( client->socketError() != QAbstractSocket::SocketError::RemoteHostClosedError )
			{
				++nCompleted;
				hostStates[index] = State::Completed;
				hostStateChanged( index, HostState::NetworkError );
				reassign( client );
				return;
			}
		case FirmwareTransferClient::Error::TimeoutError:
			if( hostStates[index] == State::Checking )
			{
				hostStates[index] = State::InCheckingQueue;
				hostStateChanged( index, HostState::Waiting );
			}
			else
			{
				hostStates[index] = State::InQueue;
				hostStateChanged( index, HostState::InQueue );
			}
			reassign( client );
			return;
		default: // NoError
			addWait( index );
			reassign( client );
			return;
		}
		break;
	}
	case FirmwareTransferClient::State::Connecting:
		if( hostStates[index] != State::Checking )
			hostStateChanged( index, HostState::Connecting );
		break;
	case FirmwareTransferClient::State::Authorization:
		if( hostStates[index] != State::Checking )
			hostStateChanged( index, HostState::Authorization );
		break;
	case FirmwareTransferClient::State::Connected:
	{
		hostVersions( index, client->firmwareVersion(), client->bootloaderVersion() );
		if( hostStates[index] == State::Checking )
		{
			if( firmwares.size() && !firmwares.contains( client->firmwareVersion() ) )
			{
				hostStates[index] = State::Updating;
				goto Reupload;
			}
			if( bootloaders.size() && !bootloaders.contains( client->bootloaderVersion() ) )
			{
				hostStates[index] = State::Updating;
				goto Reupload;
			}

			++nCompleted;
			hostStates[index] = State::Completed;
			hostStateChanged( index, HostState::Complete );
			hostUpdated( index );
			reassign( client );
			return;
		}

	Reupload:
		client->resetFirmware();
		client->resetBootloader();
		bool needUpdate = false;

		if( !firmwares.isEmpty() )
		{
			QString model = modelName( client->firmwareVersion() );
			quint64 remoteVersion = mainVersionToUint64( mainVersion( client->firmwareVersion() ) );
			quint64 selectedVersion = mainVersionToUint64( mainVersion( firmwares.begin().key() ) );

			if( updatePolicy == UpdatePolicy::IfNewer && remoteVersion < selectedVersion || updatePolicy == UpdatePolicy::Unconditional )
			{
				bool flag = true;
				for( auto i = firmwares.begin(); i != firmwares.end(); ++i )
				{
					if( modelName( i.key() ) == model )
					{
						client->setFirmware( i.value() );
						needUpdate = true;
						flag = false;
						break;
					}
				}
				if( flag )
				{
					++nCompleted;
					hostStates[index] = State::Completed;
					hostStateChanged( index, HostState::VersionError );
					reassign( client );
					return;
				}
			}
		}
		if( !bootloaders.isEmpty() )
		{
			QString model = modelName( client->bootloaderVersion() );
			quint64 remoteVersion = mainVersionToUint64( mainVersion( client->bootloaderVersion() ) );
			quint64 selectedVersion = mainVersionToUint64( mainVersion( bootloaders.begin().key() ) );

			if( updatePolicy == UpdatePolicy::IfNewer && remoteVersion < selectedVersion || updatePolicy == UpdatePolicy::Unconditional )
			{
				bool flag = true;
				for( auto i = bootloaders.begin(); i != bootloaders.end(); ++i )
				{
					if( modelName( i.key() ) == model )
					{
						client->setBootloader( i.value() );
						needUpdate = true;
						flag = false;
						break;
					}
				}
				if( flag )
				{
					++nCompleted;
					hostStates[index] = State::Completed;
					hostStateChanged( index, HostState::VersionError );
					reassign( client );
					return;
				}
			}
		}

		if( !needUpdate )
		{
			++nCompleted;
			hostStates[index] = State::Completed;
			hostStateChanged( index, HostState::Complete );
			hostUpdated( index );
			reassign( client );
			return;
		}

		client->upload();
		break;
	}
	case FirmwareTransferClient::State::FirmwareUploading:
		hostStateChanged( index, HostState::FirmwareUploading );
		break;
	case FirmwareTransferClient::State::BootloaderUploading:
		hostStateChanged( index, HostState::BootloaderUploading );
		break;
	case FirmwareTransferClient::State::UploadComplete:
		client->remoteRestart();
		break;
	case FirmwareTransferClient::State::RestartSending:
		hostStateChanged( index, HostState::Restarting );
		break;
	default:
		break;
	}
}

void GroupUpdater::progress( float value )
{
	FirmwareTransferClient* client = reinterpret_cast< FirmwareTransferClient* >( sender() );
	int index = client->property( "hostIndex" ).toInt();
	hostProgress( index, value );
}

void GroupUpdater::timeout()
{
	qint64 dt;
	do
	{
		auto i = endTimePointIndexMap.begin();
		int index = i.value();
		endTimePointIndexMap.erase( i );

		auto client = new FirmwareTransferClient;
		QObject::connect( client, &FirmwareTransferClient::stateChanged, this, &GroupUpdater::stateChanged );
		QObject::connect( client, &FirmwareTransferClient::progress, this, &GroupUpdater::progress );
		client->setProperty( "hostIndex", index );
		activeClientSet.insert( client );

		hostStates[index] = State::Checking;
		hostStateChanged( index, HostState::Checking );
		client->connectToHost( hosts[index].addr, hosts[index].password );
	} while( !endTimePointIndexMap.isEmpty() && ( dt = endTimePointIndexMap.begin().value() - QDateTime::currentMSecsSinceEpoch() ) < 50 );

	if( !endTimePointIndexMap.isEmpty() )
		timer.start( dt );
}

void GroupUpdater::addWait( int index )
{
	hostStates[index] = State::Waiting;
	endTimePointIndexMap.insert( QDateTime::currentMSecsSinceEpoch() + 5000, index );
	if( endTimePointIndexMap.size() == 1 )
		timer.start( 5000 );
	hostStateChanged( index, HostState::Waiting );
}

void GroupUpdater::reassign( FirmwareTransferClient* client )
{
	if( nCompleted == hosts.size() )
	{
		stop();
		allHostUpdated();
		return;
	}
	if( hosts.size() == nCompleted + endTimePointIndexMap.size() )
	{
		activeClientSet.remove( client );
		client->blockSignals( true );
		client->deleteLater();
		return;
	}
	if( last == hostStates.size() )
		last = 0;
	auto begin = last;
	for( ; hostStates[last] != State::InQueue && hostStates[last] != State::InCheckingQueue; )
	{
		if( ++last == hostStates.size() )
			last = 0;
		if( last == begin )
		{
			activeClientSet.remove( client );
			client->blockSignals( true );
			client->deleteLater();
			return;
		}
	}
	client->blockSignals( true );
	client->disconnectFromHost();
	client->blockSignals( false );
	client->setProperty( "hostIndex", last );
	if( hostStates[last] == State::InCheckingQueue )
	{
		hostStates[last] = State::Checking;
		hostStateChanged( last, HostState::Checking );
	}
	else
		hostStates[last] = State::Updating;
	client->connectToHost( hosts[last].addr, hosts[last].password );
	++last;
}

QString GroupUpdater::mainVersion( QString version )
{
	int i;
	for( i = version.size() - 1; version[i] < '0' || version[i] > '9'; --i )
		;
	return version.left( i + 1 );
}

QString GroupUpdater::modelName( QString version )
{
	int i;
	for( i = version.size() - 1; version[i] < '0' || version[i] > '9'; --i )
		;
	return version.right( version.size() - i - 1 );
}

quint64 GroupUpdater::mainVersionToUint64( QString mainVersion )
{
	auto list = mainVersion.split( '.' );
	return ( ( quint64 )list[0].toUInt() << 48 ) | ( ( quint64 )list[1].toUInt() << 32 ) | ( ( quint64 )list[2].toUInt() << 16 ) | ( quint64 )list[3].toUInt();
}
