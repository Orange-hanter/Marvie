#pragma once

#include <QIODevice>
#include <QLinkedList>
#include <QTimer>
#include <QMap>

class MLinkClient : public QObject
{
	Q_OBJECT

public:
	enum class State { Disconnected, Connecting, Authorizing, Connected, Disconnecting };
	enum class Error { NoError, SequenceViolationError, AuthorizationError, RemoteHostClosedError, ResponseTimeoutError, IODeviceClosedError };

	MLinkClient();
	~MLinkClient();

	State state() const;
	Error error() const;

	void setIODevice( QIODevice* device );
	void setAuthorizationData( QString accountName, QString accountPassword );
	void connectToHost();
	void disconnectFromHost();

	void sendPacket( uint8_t type, QByteArray data ); // 0 <= size <= 255
	bool sendComplexData( uint8_t channelId, QByteArray data, QString name = QString() ); // 0 < size
	bool cancelComplexDataSending( uint8_t channelId );
	bool cancelComplexDataReceiving( uint8_t channelId );

signals:
	void stateChanged( State state );
	void error( Error error );
	void connected();
	void disconnected();
	void newPacketAvailable( uint8_t type, QByteArray data );
	void newComplexPacketAvailable( uint8_t channelId, QString name, QByteArray data );
	void complexDataSendingProgress( uint8_t channelId, QString name, float progress );
	void complexDataSendindCanceled( uint8_t channelId, QString name );
	void complexDataReceivingProgress( uint8_t channelId, QString name, float progress );
	void complexDataReceivingCanceled( uint8_t channelId, QString name );

private:
	struct Request;
	struct Header;
	struct OutputCData;
	void sendSynPacket();
	void sendAuthAck();
	void sendFinPacket();
	void sendFinAckPacket();
	Request complexDataBeginRequest( uint8_t id, QByteArray name, uint32_t size );
	Request complexDataBeginAckRequest( uint8_t id, uint32_t g );
	Request nextComplexDataPartRequest( uint8_t id, OutputCData& cdata );
	Request complexDataEndRequest( uint8_t id, bool canceled );
	void pushBackRequest( Request& req );
	void pushFrontRequest( Request& req );
	uint32_t MLinkClient::calcCrc( QByteArray& data );
	void addCrc( QByteArray& data );
	void closeLink( Error e );

private slots:
	void processBytes();
	void processPacket( Header& header, QByteArray& packetData );
	void bytesWritten();
	void aboutToClose();
	void timeout();
	void pingTimeout();

private:
	enum PacketType { Syn, SynAck, Auth, AuthAck, Fin, FinAck, Ping, Pong, ComplexDataBegin, ComplexDataBeginAck, ComplexData, ComplexDataNext, ComplexDataNextAck, ComplexDataEnd, User };
	enum
	{
		MaxPacketTransferInterval = 1500,
		PingInterval = 1500, // must be <= MaxPacketTransferInterval
		WaitAfterErrorInterval = 1000,
	};
	State _state;
	Error _error;
	QIODevice* device;
	QString accountName, accountPassword;
	uint16_t sqNumCounter;
	uint16_t sqNumNext;
	uint64_t r;
	const uint32_t g;
	QTimer timer, pingTimer;

	struct Header
	{
		uint32_t preamble;
		uint8_t size;
		uint8_t type;
		uint16_t sqNum;
	};
	struct Request
	{
		Request() { type = 0; }
		uint8_t type;
		QByteArray data;
	};
	struct InputCData
	{
		InputCData() { size = 0; needCancel = false; }
		uint32_t size;
		QString name;
		QByteArray data;
		bool needCancel;
	};
	struct OutputCData
	{
		OutputCData() { g = n = numWritten = 0; needCancel = false; }
		QByteArray name;
		QByteArray data;
		uint32_t g, n;
		uint32_t numWritten;
		bool needCancel;
	};
	QLinkedList< Request > reqList;
	QMap< uint8_t, InputCData > inCDataMap;
	QMap< uint8_t, OutputCData > outCDataMap;
};