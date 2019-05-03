#include "RemoteTerminalTest.h"

RemoteTerminalTest::RemoteTerminalTest( QWidget *parent ) : QWidget( parent )
{
	ui.setupUi( this );
	QObject::connect( ui.connectButton, &QPushButton::released, this, &RemoteTerminalTest::connectButtonClicked );
}

void RemoteTerminalTest::connectButtonClicked()
{
	if( ui.connectButton->text() == "Connect" )
	{
		socket = new QTcpSocket;
		QObject::connect( socket, &QTcpSocket::connected, this, &RemoteTerminalTest::connected );
		QObject::connect( socket, &QTcpSocket::disconnected, this, &RemoteTerminalTest::disconnected );
		QObject::connect( socket, static_cast< void( QTcpSocket::* )( QAbstractSocket::SocketError ) >( &QTcpSocket::error ), this, &RemoteTerminalTest::error );
		ui.connectButton->setText( "Connecting..." );
		socket->connectToHost( ui.ipEdit->text(), 55000 );
	}
	else
		disconnected();
}

void RemoteTerminalTest::connected()
{
	ui.connectButton->setText( "Disconnect" );
	ui.terminalWidget->setIODevice( socket );
	ui.terminalWidget->synchronize();
}

void RemoteTerminalTest::disconnected()
{
	QObject::disconnect( socket, &QTcpSocket::connected, this, &RemoteTerminalTest::connected );
	QObject::disconnect( socket, &QTcpSocket::disconnected, this, &RemoteTerminalTest::disconnected );
	QObject::disconnect( socket, static_cast< void( QTcpSocket::* )( QAbstractSocket::SocketError ) >( &QTcpSocket::error ), this, &RemoteTerminalTest::error );
	ui.terminalWidget->setIODevice( nullptr );
	socket->deleteLater();
	socket = nullptr;
	ui.connectButton->setText( "Connect" );
}

void RemoteTerminalTest::error( QAbstractSocket::SocketError socketError )
{
	disconnected();
}
