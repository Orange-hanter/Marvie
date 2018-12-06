#include "MLinkClientTest.h"
#include <QHostAddress>
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

	QObject::connect( ui.ethernetConnectButton, &QToolButton::released, this, &MLinkClientTest::connectButtonClicked );
	QObject::connect( ui.ch2Button, &QToolButton::released, this, &MLinkClientTest::ch2ButtonClicked );
	QObject::connect( ui.ch3Button, &QToolButton::released, this, &MLinkClientTest::ch3ButtonClicked );
	QObject::connect( &mlink, &MLinkClient::stateChanged, this, &MLinkClientTest::mlinkStateChanged );
	QObject::connect( &mlink, &MLinkClient::newPacketAvailable, this, &MLinkClientTest::mlinkNewPacketAvailable );
	QObject::connect( &mlink, &MLinkClient::newChannelDataAvailable, this, &MLinkClientTest::mlinkNewComplexPacketAvailable );
	QObject::connect( &mlink, &MLinkClient::channelDataSendingProgress, this, &MLinkClientTest::mlinkComplexDataSendingProgress );
	QObject::connect( &mlink, &MLinkClient::channeDataReceivingProgress, this, &MLinkClientTest::mlinkComplexDataReceivingProgress );

	dwc[0] = dwc[1] = upc[0] = upc[1] = spc = 0;
	ch3Size = 0;
}

void MLinkClientTest::connectButtonClicked()
{
	if( mlink.state() == MLinkClient::State::Disconnected )
		mlink.connectToHost( QHostAddress( ui.ipEdit->text() ) );
	else
		mlink.disconnectFromHost();
}

void MLinkClientTest::ch2ButtonClicked()
{
	if( ui.ch2ProgressBar->maximum() == 0 || mlink.state() != MLinkClient::State::Connected )
		return;

	if( ui.ch2Button->text() == "Upload" )
	{
		ui.ch2Button->setText( "Cancel" );
		mlink.sendChannelData( 2, randBytes( ui.ch2DataSizeSpinBox->value() ), "2.dat" );
	}
	else
	{
		if( mlink.cancelChannelDataSending( 2 ) )
		{
			ui.ch2ProgressBar->setRange( 0, 100 );
			ui.ch2ProgressBar->setValue( 0 );
			ui.ch2Button->setText( "Upload" );
		}
	}
}

void MLinkClientTest::ch3ButtonClicked()
{
	if( ui.ch3ProgressBar->maximum() == 0 || mlink.state() != MLinkClient::State::Connected )
		return;

	if( ui.ch3Button->text() == "Download" )
	{
		ui.ch3Button->setText( "Cancel" );
		ui.ch3ProgressBar->setRange( 0, 0 );
		ch3Size = ui.ch3DataSizeSpinBox->value();
		mlink.sendPacket( 2, QByteArray( ( char* )&ch3Size, sizeof( ch3Size ) ) );
	}
	else
	{
		if( mlink.cancelChannelDataReceiving( 3 ) )
		{
			ui.ch3ProgressBar->setRange( 0, 100 );
			ui.ch3ProgressBar->setValue( 0 );
			ui.ch3Button->setText( "Download" );
		}
	}
}

void MLinkClientTest::mlinkStateChanged( MLinkClient::State s )
{
	switch( s )
	{
	case MLinkClient::State::Disconnected:
		ui.ipEdit->setEnabled( true );
		ui.ethernetConnectButton->setIcon( QIcon( ":/MLinkClientTest/icons/icons8-ethernet-on-filled-50.png" ) );
		ui.ch2Button->setText( "Upload" );
		ui.ch2ProgressBar->setValue( 0 );
		ui.ch2ProgressBar->setMaximum( 100 );
		ui.ch3Button->setText( "Download" );
		ui.ch3ProgressBar->setValue( 0 );
		ui.ch3ProgressBar->setMaximum( 100 );
		break;
	case MLinkClient::State::Connecting:
		ui.ethernetConnectButton->setIcon( QIcon( ":/MLinkClientTest/icons/icons8-ethernet-on-filled-50-orange.png" ) );
		break;
	case MLinkClient::State::Connected:
		ui.ethernetConnectButton->setIcon( QIcon( ":/MLinkClientTest/icons/icons8-ethernet-on-filled-50-green.png" ) );

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
			mlink.sendChannelData( 0, cdata[0], "0.dat" );
		if( ui.ch1EnableCheckBox->checkState() == Qt::Checked )
			mlink.sendChannelData( 1, cdata[1], "1.dat" );
		dwc[0] = dwc[1] = upc[0] = upc[1] = spc = 0;
		if( ui.simplePacketEnableCheckBox->checkState() == Qt::Checked )
			mlink.sendPacket( 0, QByteArray( "Hello", 6 ) );
		updateLabels();

		break;
	case MLinkClient::State::Disconnecting:
		ui.ethernetConnectButton->setIcon( QIcon( ":/MLinkClientTest/icons/icons8-ethernet-on-filled-50-orange.png" ) );
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
	assert( QString( "%1.dat" ).arg( channelId ) == name );
	if( channelId == 3 )
	{
		if( data.size() == ch3Size )
		{
			for( int i = 0; i < data.size(); ++i )
			{
				if( ( char )data[i] != ( '0' + i % 10 ) )
				{
					ui.ch3ProgressBar->setFormat( "Error" );
					break;
				}
			}
		}
		else
			ui.ch3ProgressBar->setFormat( "Error" );

		ui.ch3Button->setText( "Download" );

		return;
	}

	assert( channelId < 2 );
	if( cdata[channelId] != data )
		return;
	++dwc[channelId];
	uint32_t chDataSize[] = { ( uint32_t )ui.ch0DataSizeSpinBox->value(), ( uint32_t )ui.ch1DataSizeSpinBox->value() };
	cdata[channelId] = randBytes( chDataSize[channelId] );
	mlink.sendChannelData( channelId, cdata[channelId], QString( "%1.dat" ).arg( channelId ) );
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

void MLinkClientTest::mlinkComplexDataReceivingProgress( uint8_t channelId, QString name, float progress )
{
	assert( channelId <= 3 && channelId != 2 );
	assert( QString( "%1.dat" ).arg( channelId ) == name );
	assert( progress <= 1.0f );
	QProgressBar* pb[4] = { ui.ch0ProgressBar, ui.ch1ProgressBar, nullptr, ui.ch3ProgressBar };
	pb[channelId]->setFormat( "Downloading %p%" );
	pb[channelId]->setValue( progress*100.0 );
	pb[channelId]->setRange( 0, 100 );
	updateLabels();
}

void MLinkClientTest::updateLabels()
{
	ui.ch0Label->setText( QString( "%1/%2" ).arg( upc[0] ).arg( dwc[0] ) );
	ui.ch1Label->setText( QString( "%1/%2" ).arg( upc[1] ).arg( dwc[1] ) );
	ui.simplePacketLabel->setText( QString( "%1" ).arg( spc ) );
}