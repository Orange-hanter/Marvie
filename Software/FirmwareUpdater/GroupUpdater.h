#pragma once

#include "FirmwareTransferClient.h"
#include <QHostAddress>
#include <QVector>
#include <QMap>
#include <QTimer>
#include <memory>

class GroupUpdater : public QObject
{
	Q_OBJECT

public:
	enum class UpdatePolicy
	{
		Unconditional,
		IfNewer
	};
	enum class HostState
	{
		InQueue,
		Connecting,
		Authorization,
		FirmwareUploading,
		BootloaderUploading,
		Restarting,
		Waiting,
		Checking,
		Complete,
		AuthError,
		VersionError,
		ServerVersionError,
		NetworkError
	};

	GroupUpdater();
	~GroupUpdater();

	struct Host 
	{
		QHostAddress addr;
		QString password;
	};
	void setMaxConnections( int num );
	void setUpdatePolicy( UpdatePolicy policy );
	void setHostList( const QVector< Host >& hosts );
	void setHostList( QVector< Host >&& hosts );
	void addFirmware( QString version, QByteArray binaryData );
	void addBootloader( QString version, QByteArray binaryData );
	void clear();

	void start();
	void stop();
	bool isStarted();

signals:
	void hostStateChanged( int index, HostState state );
	void hostVersions( int index, QString firmwareVersion, QString bootloaderVersion );
	void hostProgress( int index, float value );
	void hostUpdated( int index );
	void allHostUpdated();

private slots:
	void stateChanged( FirmwareTransferClient::State state );
	void progress( float value );
	void timeout();

private:
	void addWait( int index );
	void reassign( FirmwareTransferClient* client );
	static QString mainVersion( QString version );
	static QString modelName( QString version );
	static quint64 mainVersionToUint64( QString mainVersion );

private:
	bool started;
	int maxConnection;
	UpdatePolicy updatePolicy;
	QVector< Host > hosts;
	QMap< QString, QByteArray > firmwares;
	QMap< QString, QByteArray > bootloaders;
	QSet< FirmwareTransferClient* > activeClientSet;
	enum class State : uint8_t { InQueue, Updating, Waiting, Checking, InCheckingQueue, Completed };
	QVector< State > hostStates;
	QMultiMap< qint64, int > endTimePointIndexMap;
	QTimer timer;
	int last, nCompleted;
};