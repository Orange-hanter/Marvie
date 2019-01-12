#pragma once

#include <QLinkedList>
#include <QTcpSocket>
#include <QTimer>
#include <QMap>

class MLinkClient : public QObject
{
	Q_OBJECT

public:
	enum class State { Disconnected, Connecting, Authorizing, Connected, Disconnecting };
	enum class Error { NoError, ClientInnerError, AuthenticationError, RemoteHostClosedError, ResponseTimeoutError };

	MLinkClient();
	~MLinkClient();

	State state() const;
	Error error() const;

	void setAuthorizationData( QString accountName, QString accountPassword );
	void connectToHost( QHostAddress address );
	void disconnectFromHost();

	void sendPacket( uint8_t type, QByteArray data ); // 0 <= size <= MSS
	bool sendChannelData( uint8_t channel, QByteArray data, QString name = QString() ); // 0 < size
	bool cancelChannelDataSending( uint8_t channel );
	bool cancelChannelDataReceiving( uint8_t channel );

signals:
	void stateChanged( MLinkClient::State state );
	void error( MLinkClient::Error error );
	void connected();
	void disconnected();
	void newPacketAvailable( uint8_t type, QByteArray data );
	void newChannelDataAvailable( uint8_t channel, QString name, QByteArray data );
	void channelDataSendingProgress( uint8_t channel, QString name, float progress );
	void channeDataReceivingProgress( uint8_t channel, QString name, float progress );

private:
	struct Request;
	struct Header;
	struct OutputCData;
	void sendAuth();
	Request openChannelRequest( uint8_t channel, uint32_t id, QByteArray name, uint32_t size );
	Request nextDataChannelPartRequest( uint8_t channel, OutputCData& cdata );
	Request closeDataChannelRequest( uint8_t channel, uint32_t id, bool canceled );
	Request remoteCloseDataChannelRequest( uint8_t channel, uint32_t id );
	void pushBackRequest( Request& req );
	void pushFrontRequest( Request& req );
	void closeLink( Error e );

private slots:
	void processBytes();
	void processPacket( Header& header, QByteArray& packetData );
	void bytesWritten();
	void socketConnected();
	void socketDisconnected();
	void socketError( QAbstractSocket::SocketError socketError );
	void timeout();
	void pingTimeout();

private:
	enum PacketType { Auth, AuthAck, AuthFail, IAmAlive, OpenChannel, ChannelData, CloseChannel, RemoteCloseChannel, User };
#pragma pack( push, 1 )
	struct Header
	{
		uint32_t preamble;
		uint16_t size;
		uint8_t type;
	};
	struct ChannelHeader
	{
		uint32_t id;
		uint8_t ch;
	};
#pragma pack( pop )
	enum
	{
		MTU = 1400,
		MSS = MTU - sizeof( Header ),
		DataChannelMSS = MTU - sizeof( Header ) - sizeof( ChannelHeader ),
		MaxPacketTransferInterval = 1500,
		PingInterval = 1000, // must be <= MaxPacketTransferInterval
	};
	State _state;
	Error _error;
	uint32_t idCounter;
	QTcpSocket* socket;
	QString accountName, accountPassword;
	QTimer timer, pingTimer;
	
	struct Request
	{
		Request() { type = 0; }
		uint8_t type;
		QByteArray data;
	};
	struct InputCData
	{
		InputCData() { id = size = 0; }
		uint32_t id;
		QString name;
		uint32_t size;
		QByteArray data;
	};
	struct OutputCData
	{
		OutputCData() { id = numWritten = 0; }
		uint32_t id;
		QByteArray name;
		QByteArray data;
		uint32_t numWritten;
	};
	QLinkedList< Request > reqList;
	QMap< uint8_t, InputCData > inCDataMap;
	QMap< uint8_t, OutputCData > outCDataMap;
};