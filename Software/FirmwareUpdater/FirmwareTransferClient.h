#pragma once

#include <QTcpSocket>
#include <QTimer>

class FirmwareTransferClient : public QObject
{
	Q_OBJECT

public:
	enum class Error { NoError, VersionError, AuthError, TimeoutError, SocketError };
	enum class State { Disconnected, Connecting, Authorization, Connected, FirmwareUploading, BootloaderUploading, UploadComplete, RestartSending };

	FirmwareTransferClient();
	~FirmwareTransferClient();

	void connectToHost( QHostAddress addr, QString password );
	void disconnectFromHost();

	State state();
	Error error();
	QAbstractSocket::SocketError socketError();
	QString firmwareVersion();
	QString bootloaderVersion();
	void setFirmware( QByteArray binaryData );
	void setBootloader( QByteArray binaryData );
	void resetFirmware();
	void resetBootloader();
	void upload();
	void remoteRestart();

signals:
	void stateChanged( State state );
	void error( Error error );
	void progress( float value );
	void uploadComplete();

private slots:
	void socketConnected();
	void socketDisconnected();
	void socketError( QAbstractSocket::SocketError err );
	void socketReadyRead();
	void socketBytesWritten( qint64 );
	void timeout();

private:
	void setState( State state );
	void removeSocket();
	void startFirmwareUploading();
	void startBootloaderUploading();

private:
	enum
	{
		ConnectingTimeout = 5000,
		RemoteRestartTimeout = 5000,
		ResponseTimeout = 30000
	};
	enum Command
	{
		AuthOk,
		AuthFailed,
		StartFirmwareTransfer,
		StartBootloaderTransfer,
		TransferOk,
		TransferFailed,
		Restart
	};
	State currentState;
	Error err;
	QAbstractSocket::SocketError socketErr;
	QTcpSocket* socket;
	QString password;
	QString firmwareVer, bootloaderVer;
	QByteArray firmwareData, bootloaderData;
	int offset;
	QTimer timer;

	static constexpr char nameStr[] = "FirmwareTransferProtocol";
	static constexpr uint32_t version = 1;
#pragma pack( push, 1 )
	struct Header
	{
		char name[sizeof( nameStr )];
		uint32_t version;
		uint32_t reserved;
	};
#pragma pack( pop )
	struct Desc
	{
		uint8_t hash[20];
		uint32_t size;
	};
};