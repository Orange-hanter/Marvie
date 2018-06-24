#include "MLinkClientTest.h"
#include <QSerialPortInfo>
#include <QSerialPort>
#include <assert.h>

QByteArray randBytes( uint size )
{
	QByteArray m( size, Qt::Initialization::Uninitialized );
	for( int i = 0; i < size; ++i )
		m[i] = qrand() % 255;
	return m;
}

MLinkClientTest::MLinkClientTest( QWidget *parent ) : QWidget( parent )
{
	ui.setupUi( this );

	ioDevice = nullptr;

	QObject::connect( ui.rs232ConnectButton, &QToolButton::released, this, &MLinkClientTest::connectButtonClicked );
	QObject::connect( ui.ch2Button, &QToolButton::released, this, &MLinkClientTest::ch2ButtonClicked );
	QObject::connect( &mlink, &MLinkClient::stateChanged, this, &MLinkClientTest::mlinkStateChanged );
	QObject::connect( &mlink, &MLinkClient::newPacketAvailable, this, &MLinkClientTest::mlinkNewPacketAvailable );
	QObject::connect( &mlink, &MLinkClient::newComplexPacketAvailable, this, &MLinkClientTest::mlinkNewComplexPacketAvailable );
	QObject::connect( &mlink, &MLinkClient::complexDataSendingProgress, this, &MLinkClientTest::mlinkComplexDataSendingProgress );
	QObject::connect( &mlink, &MLinkClient::complexDataSendindCanceled, this, &MLinkClientTest::mlinkComplexDataSendindCanceled );
	QObject::connect( &mlink, &MLinkClient::complexDataReceivingProgress, this, &MLinkClientTest::mlinkComplexDataReceivingProgress );
	ui.rs232ComboBox->installEventFilter( this );

	dwc[0] = dwc[1] = upc[0] = upc[1] = spc = 0;
}

bool MLinkClientTest::eventFilter( QObject *obj, QEvent *event )
{
	if( obj == ui.rs232ComboBox )
	{
		if( event->type() == QEvent::Type::MouseButtonPress || event->type() == QEvent::Type::KeyPress )
		{
			auto current = ui.rs232ComboBox->currentText();
			auto list = QSerialPortInfo::availablePorts();
			QStringList names;
			for( auto& i : list )
				names.append( i.portName() );
			ui.rs232ComboBox->clear();
			ui.rs232ComboBox->addItems( names );
			if( names.contains( current ) )
				ui.rs232ComboBox->setCurrentText( current );
			else
				ui.rs232ComboBox->setCurrentIndex( -1 );
		}
	}

	return false;
}

void MLinkClientTest::connectButtonClicked()
{
	if( mlink.state() != MLinkClient::State::Disconnected )
	{
		mlink.disconnectFromHost();
		return;
	}

	if( ui.rs232ComboBox->currentText().isEmpty() )
		return;
	QSerialPort* port = new QSerialPort();
	port->setBaudRate( QSerialPort::Baud115200 );
	port->setStopBits( QSerialPort::OneStop );
	port->setParity( QSerialPort::NoParity );
	port->setFlowControl( QSerialPort::NoFlowControl );
	port->setPortName( ui.rs232ComboBox->currentText() );
	if( !port->open( QIODevice::ReadWrite ) )
	{
		delete port;
		return;
	}

	ioDevice = port;
	mlink.setIODevice( ioDevice );
	mlink.connectToHost();
	ui.rs232ComboBox->setEnabled( false );
}

void MLinkClientTest::ch2ButtonClicked()
{
	if( ui.ch2Button->text() == "Upload" )
	{
		ui.ch2Button->setText( "Cancel" );
		mlink.sendComplexData( 2, randBytes( ui.ch2DataSizeSpinBox->value() ), "2.dat" );
	}
	else
	{
		if( mlink.cancelComplexDataSending( 2 ) )
			ui.ch2ProgressBar->setRange( 0, 0 );
	}
}

void MLinkClientTest::mlinkStateChanged( MLinkClient::State s )
{
	switch( s )
	{
	case MLinkClient::State::Disconnected:		
		ui.rs232ComboBox->setEnabled( true );
		ui.rs232ConnectButton->setIcon( QIcon( ":/MLinkClientTest/icons/icons8-rs-232-female-filled-50.png" ) );
		mlink.setIODevice( nullptr );
		ioDevice->close();
		delete ioDevice;
		ioDevice = nullptr;
		break;
	case MLinkClient::State::Connecting:
		ui.rs232ConnectButton->setIcon( QIcon( ":/MLinkClientTest/icons/icons8-rs-232-female-filled-50-orange.png" ) );
		break;
	case MLinkClient::State::Connected:
		ui.rs232ConnectButton->setIcon( QIcon( ":/MLinkClientTest/icons/icons8-rs-232-female-filled-50-green.png" ) );

		{
			QProgressBar* pb[3] = { ui.ch0ProgressBar, ui.ch1ProgressBar, ui.ch2ProgressBar };
			for( int i = 0; i < 3; ++i )
			{
				pb[i]->setFormat( "Uploading %p%" );
				pb[i]->setValue( 0 );
			}
		}

		cdata[0] = randBytes( ui.ch0DataSizeSpinBox->value() );
		cdata[1] = randBytes( ui.ch1DataSizeSpinBox->value() );
		if( ui.ch0EnableCheckBox->checkState() == Qt::Checked )
			mlink.sendComplexData( 0, cdata[0], "0.dat" );
		if( ui.ch1EnableCheckBox->checkState() == Qt::Checked )
			mlink.sendComplexData( 1, cdata[1], "1.dat" );
		dwc[0] = dwc[1] = upc[0] = upc[1] = spc = 0;
		if( ui.simplePacketEnableCheckBox->checkState() == Qt::Checked )
			mlink.sendPacket( 0, QByteArray( "Hello", 6 ) );
		ui.ch2Button->setText( "Upload" );
		updateLabels();

		break;
	case MLinkClient::State::Disconnecting:
		ui.rs232ConnectButton->setIcon( QIcon( ":/MLinkClientTest/icons/icons8-rs-232-female-filled-50-orange.png" ) );
		break;
	default:
		break;
	}
}

void MLinkClientTest::mlinkNewPacketAvailable( uint8_t type, QByteArray data )
{
	if( type == 0 && data == QByteArray( "Hello", 6 ) )
	{
		mlink.sendPacket( 0, QByteArray( "Hello", 6 ) );
		++spc;
		updateLabels();
	}
}

void MLinkClientTest::mlinkNewComplexPacketAvailable( uint8_t channelId, QString name, QByteArray data )
{
	assert( channelId < 2 );
	assert( QString( "%1.dat" ).arg( channelId ) == name );
	if( cdata[channelId] != data )
		return;
	++dwc[channelId];
	uint32_t chDataSize[] = { ui.ch0DataSizeSpinBox->value(), ui.ch1DataSizeSpinBox->value() };
	cdata[channelId] = randBytes( chDataSize[channelId] );
	mlink.sendComplexData( channelId, cdata[channelId], QString( "%1.dat" ).arg( channelId ) );
}

void MLinkClientTest::mlinkComplexDataSendingProgress( uint8_t channelId, QString name, float progress )
{
	assert( channelId < 3 );
	assert( QString( "%1.dat" ).arg( channelId ) == name );
	if( channelId == 2 )
	{
		if( progress == 1.0f )
			ui.ch2Button->setText( "Upload" );
	}
	else if( progress == 1.0f )
	{
		++upc[channelId];
		mlink.sendPacket( 1, QByteArray( 1, channelId ) );
	}
	QProgressBar* pb[3] = { ui.ch0ProgressBar, ui.ch1ProgressBar, ui.ch2ProgressBar };
	pb[channelId]->setFormat( "Uploading %p%" );
	pb[channelId]->setValue( progress*100.0 );
	updateLabels();
}

void MLinkClientTest::mlinkComplexDataSendindCanceled( uint8_t channelId )
{
	assert( channelId == 2 );
	ui.ch2ProgressBar->setRange( 0, 100 );
	ui.ch2ProgressBar->setValue( 0 );
	ui.ch2Button->setText( "Upload" );
}

void MLinkClientTest::mlinkComplexDataReceivingProgress( uint8_t channelId, QString name, float progress )
{
	assert( channelId < 2 );
	assert( QString( "%1.dat" ).arg( channelId ) == name );
	assert( progress <= 1.0f );
	QProgressBar* pb[2] = { ui.ch0ProgressBar, ui.ch1ProgressBar };
	pb[channelId]->setFormat( "Downloading %p%" );
	pb[channelId]->setValue( progress*100.0 );
	updateLabels();
}

void MLinkClientTest::updateLabels()
{
	ui.ch0Label->setText( QString( "%1/%2" ).arg( upc[0] ).arg( dwc[0] ) );
	ui.ch1Label->setText( QString( "%1/%2" ).arg( upc[1] ).arg( dwc[1] ) );
	ui.simplePacketLabel->setText( QString( "%1" ).arg( spc ) );
}