#include "DataTransferProgressWindow.h"
#include <QHBoxLayout>
#include <QToolButton>

DataTransferProgressWindow::DataTransferProgressWindow( MLinkClient* link, uint8_t channelId, TransferDir dir, QWidget* parent, int errPacketId ) : FramelessDialog( parent )
{
	this->link = link;
	this->channelId = channelId;
	this->dir = dir;
	this->errPacketId = errPacketId;

	QWidget* centralWindget = new QWidget;
	QHBoxLayout* layout = new QHBoxLayout( centralWindget );
	layout->setContentsMargins( 12, 12, 12, 12 );
	
	progressBar = new QProgressBar;
	progressBar->setAlignment( Qt::AlignCenter );
	progressBar->setValue( 0 );
	layout->addWidget( progressBar );

	button = new QToolButton;
	button->setFixedSize( QSize( 29, 29 ) );
	button->setIconSize( QSize( 25, 25 ) );
	button->setIcon( QIcon( ":/MarvieController/icons/icons8-cancel-26.png" ) );
	button->setAutoRaise( true );
	button->setToolTip( "Cancel" );
	layout->addWidget( button );

	setPalette( parent->palette() );
	setCentralWidget( centralWindget );
	windowButtons()->setButtonColor( ButtonType::Minimize | ButtonType::Maximize | ButtonType::Close, QColor( 100, 100, 100 ) );
	windowButtons()->setButtons( 0 );

	setFixedSize( 315, 86 );
	QRect windowRect( 0, 0, 315, 86 );
	windowRect.moveCenter( parent->mapToGlobal( parent->rect().center() ) );
	setGeometry( windowRect );

	QObject::connect( button, &QToolButton::released, this, &DataTransferProgressWindow::buttonClicked );
	QObject::connect( link, &MLinkClient::stateChanged, this, &DataTransferProgressWindow::stateChanged );
	QObject::connect( link, &MLinkClient::newPacketAvailable, this, &DataTransferProgressWindow::newPacketAvailable );
	if( dir == TransferDir::Sending )
	{
		progressBar->setRange( 0, 1000 );
		QObject::connect( link, &MLinkClient::channelDataSendingProgress, this, &DataTransferProgressWindow::complexDataSendingProgress );
	}
	else
	{
		progressBar->setRange( 0, 0 );
		QObject::connect( link, &MLinkClient::newChannelDataAvailable, this, &DataTransferProgressWindow::newComplexPacketAvailable );
		QObject::connect( link, &MLinkClient::channeDataReceivingProgress, this, &DataTransferProgressWindow::complexDataReceivingProgress );
	}
}

void DataTransferProgressWindow::buttonClicked()
{
	if( button->text().isEmpty() )
	{
		if( dir == TransferDir::Sending )
		{
			if( !link->cancelChannelDataSending( channelId ) )
			{
				error();
				return;
			}
		}
		else
		{
			if( !link->cancelChannelDataReceiving( channelId ) )
			{
				error();
				return;
			}
		}

		reject();
	}
	else
		accept();
}

void DataTransferProgressWindow::stateChanged( MLinkClient::State state )
{
	error();
}

void DataTransferProgressWindow::newPacketAvailable( uint8_t type, QByteArray data )
{
	if( ( int )type == errPacketId )	
		error();	
}

void DataTransferProgressWindow::newComplexPacketAvailable( uint8_t channelId, QString name, QByteArray data )
{
	if( channelId != this->channelId )
		return;

	accept();
}

void DataTransferProgressWindow::complexDataSendingProgress( uint8_t channelId, QString name, float progress )
{
	if( channelId != this->channelId )
		return;

	progressBar->setValue( 1000 * progress );
	if( progress == 1.0f )
		accept();
}

//void DataTransferProgressWindow::complexDataSendindCanceled( uint8_t channelId, QString name )
//{
//	if( channelId != this->channelId )
//		return;
//
//	progressBar->setRange( 0, 1000 );
//	progressBar->setValue( 0 );
//	if( cancellationRequested )
//		progressBar->setFormat( "Canceled" );
//	else
//		progressBar->setFormat( "Error" );
//	okButton();
//}

void DataTransferProgressWindow::complexDataReceivingProgress( uint8_t channelId, QString name, float progress )
{
	if( channelId != this->channelId )
		return;

	progressBar->setFormat( "%p%" );
	progressBar->setRange( 0, 1000 );
	progressBar->setValue( 1000 * progress );
}

//void DataTransferProgressWindow::complexDataReceivingCanceled( uint8_t channelId, QString name )
//{
//	if( channelId != this->channelId )
//		return;
//
//	progressBar->setRange( 0, 1000 );
//	progressBar->setValue( 0 );
//	if( cancellationRequested )
//		progressBar->setFormat( "Canceled" );
//	else
//		progressBar->setFormat( "Error" );	
//}

void DataTransferProgressWindow::closeEvent( QCloseEvent * event )
{
	event->ignore();
}

void DataTransferProgressWindow::okButton()
{
	channelId = -1;
	button->setAutoRaise( false );
	button->setToolButtonStyle( Qt::ToolButtonStyle::ToolButtonTextOnly );
	button->setFixedWidth( 50 );
	button->setText( "Ok" );
	button->setToolTip( "" );
	button->show();
}

void DataTransferProgressWindow::error()
{
	channelId = -1;
	progressBar->setRange( 0, 1000 );
	progressBar->setValue( 0 );
	progressBar->setFormat( "Error" );
	okButton();
}