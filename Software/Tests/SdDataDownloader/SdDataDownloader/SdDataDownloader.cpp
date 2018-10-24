#include "SdDataDownloader.h"
#include "QHostAddress"
#include <QDir>

SdDataDownloader::SdDataDownloader( QWidget *parent ) : QWidget( parent )
{
	ui.setupUi( this );
	socket = nullptr;

	QObject::connect( ui.downloadButton, &QPushButton::released, this, &SdDataDownloader::downloadButtonClicked );
}

void SdDataDownloader::downloadButtonClicked()
{
	if( ui.downloadButton->text() == "Download" )
	{
		ui.downloadButton->setEnabled( false );
		socket = new QTcpSocket;

		QObject::connect( socket, &QTcpSocket::connected, this, &SdDataDownloader::socketConnected );
		QObject::connect( socket, &QTcpSocket::disconnected, this, &SdDataDownloader::socketDisconnected );
		QObject::connect( socket, static_cast< void( QTcpSocket::* )( QAbstractSocket::SocketError ) >( &QTcpSocket::error ), this, &SdDataDownloader::socketError );
		QObject::connect( socket, &QTcpSocket::readyRead, this, &SdDataDownloader::newDataAvailable );

		socket->connectToHost( QHostAddress( ui.ipEdit->text() ), 55555 );
	}
	else
		socket->disconnectFromHost();
}

void SdDataDownloader::socketConnected()
{
	ui.downloadButton->setText( "Disconnect" );
	ui.downloadButton->setEnabled( true );

	struct Request 
	{
		uint32_t beginSector;
		uint32_t endSector;
	} req;
	req.beginSector = ui.beginSectorSpinBox->value();
	req.endSector = ui.endSectorSpinBox->value();
	socket->write( ( const char* )&req, sizeof( req ) );

	ui.progressBar->setRange( 0, ( req.endSector - req.beginSector ) * 512 );
	ui.progressBar->setValue( 0 );

	QString fileName = QString( "data[%1-%2].bin" ).arg( req.beginSector ).arg( req.endSector );
	QDir().remove( fileName );
	file.setFileName( fileName );
	file.open( QIODevice::WriteOnly );
}

void SdDataDownloader::socketDisconnected()
{
	ui.downloadButton->setText( "Download" );
	ui.downloadButton->setEnabled( true );
	QObject::disconnect( socket, &QTcpSocket::connected, this, &SdDataDownloader::socketConnected );
	QObject::disconnect( socket, &QTcpSocket::disconnected, this, &SdDataDownloader::socketDisconnected );
	QObject::disconnect( socket, static_cast< void( QTcpSocket::* )( QAbstractSocket::SocketError ) >( &QTcpSocket::error ), this, &SdDataDownloader::socketError );
	QObject::disconnect( socket, &QTcpSocket::readyRead, this, &SdDataDownloader::newDataAvailable );
	socket->deleteLater();
	socket = nullptr;
	file.close();
}

void SdDataDownloader::socketError( QAbstractSocket::SocketError socketError )
{
	ui.downloadButton->setText( "Download" );
	ui.downloadButton->setEnabled( true );
	QObject::disconnect( socket, &QTcpSocket::connected, this, &SdDataDownloader::socketConnected );
	QObject::disconnect( socket, &QTcpSocket::disconnected, this, &SdDataDownloader::socketDisconnected );
	QObject::disconnect( socket, static_cast< void( QTcpSocket::* )( QAbstractSocket::SocketError ) >( &QTcpSocket::error ), this, &SdDataDownloader::socketError );
	QObject::disconnect( socket, &QTcpSocket::readyRead, this, &SdDataDownloader::newDataAvailable );
	socket->deleteLater();
	socket = nullptr;
	file.close();
}

void SdDataDownloader::newDataAvailable()
{	
	file.write( socket->readAll() );
	ui.progressBar->setValue( file.size() );
}
