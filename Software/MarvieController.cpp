#include "MarvieController.h"
#include <QSerialPortInfo>
#include <QSerialPort>
#include <QXmlSchema>
#include <QDomDocument>
#include <QFile>
#include <QDir>
#include <QCryptographicHash>
#include <QAbstractUriResolver>
#include <QFileDialog>
#include <QtCharts/QPieSeries>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>
#include <QStack>
#include <limits>
#include <assert.h>

QT_CHARTS_USE_NAMESPACE

#define ENUM_PATTERN "[A-Za-z0-9_]+(\\|[A-Za-z0-9_]+)*"
#define DEVICE_LOAD_FREQ 5
#define CPU_LOAD_SERIES_WINDOW 10 // sec

QByteArray randBytes( uint size )
{
	QByteArray m( size, Qt::Initialization::Uninitialized );
	for( int i = 0; i < size; ++i )
		m[i] = qrand() % 255;
	return m;
}

MarvieController::MarvieController( QWidget *parent ) : FramelessWidget( parent ), vPortIdComboBoxEventFilter( vPorts )
{
	QWidget* centralWindget = new QWidget;
	ui.setupUi( centralWindget );
	setPalette( centralWindget->palette() );
	setCentralWidget( centralWindget );
	windowButtons()->setButtonColor( ButtonType::Minimize | ButtonType::Maximize | ButtonType::Close, QColor( 100, 100, 100 ) );
	setTitleText( "MarvieController" );

	//setMinimumSize( QSize( 830, 560 ) );
	QRect mainWindowRect( 0, 0, 540, 680 );
	mainWindowRect.moveCenter( qApp->desktop()->rect().center() );
	setGeometry( mainWindowRect );

	mlinkIODevice = nullptr;

	sensorsInit();

	sensorNameTimer.setInterval( 300 );
	sensorNameTimer.setSingleShot( true );

	popupSensorsListWidget = new QTreeWidget;
	popupSensorsListWidget->setWindowFlags( Qt::Popup );
	popupSensorsListWidget->setFocusPolicy( Qt::NoFocus );
	popupSensorsListWidget->setFocusProxy( ui.sensorNameEdit );
	popupSensorsListWidget->setMouseTracking( true );
	popupSensorsListWidget->setColumnCount( 1 );
	popupSensorsListWidget->setUniformRowHeights( true );
	popupSensorsListWidget->setRootIsDecorated( false );
	popupSensorsListWidget->setEditTriggers( QTreeWidget::NoEditTriggers );
	popupSensorsListWidget->setSelectionBehavior( QTreeWidget::SelectRows );
	popupSensorsListWidget->setFrameStyle( QFrame::Box | QFrame::Plain );
	popupSensorsListWidget->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	popupSensorsListWidget->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	popupSensorsListWidget->header()->hide();
	popupSensorsListWidget->installEventFilter( this );

	ui.sensorSettingsTreeWidget->setColumnWidth( 0, 250 );
	ui.sensorSettingsTreeWidget->header()->setSectionResizeMode( 0, QHeaderView::ResizeMode::Stretch );
	ui.sensorSettingsTreeWidget->header()->setSectionResizeMode( 1, QHeaderView::ResizeMode::Fixed );
	ui.sensorSettingsTreeWidget->header()->resizeSection( 1, 150 );
	ui.mainStackedWidget->setCurrentIndex( 0 );
	ui.settingsStackedWidget->setCurrentIndex( 0 );

	ui.gsmModemConfigWidget->hide();

	ui.vPortsOverGsmTableView->setModel( &vPortsOverGsmModel );
	ui.vPortsOverGsmTableView->setItemDelegate( &vPortsOverIpDelegate );
	ui.vPortsOverGsmTableView->horizontalHeader()->setSectionResizeMode( QHeaderView::ResizeMode::Fixed  );
	ui.vPortsOverGsmTableView->horizontalHeader()->resizeSection( 0, 100 );
	ui.vPortsOverGsmTableView->horizontalHeader()->resizeSection( 1, 50 );
	ui.vPortsOverGsmTableView->verticalHeader()->setSectionResizeMode( QHeaderView::ResizeMode::Fixed );
	
	ui.vPortsOverEthernetTableView->setModel( &vPortsOverEthernetModel );
	ui.vPortsOverEthernetTableView->setItemDelegate( &vPortsOverIpDelegate );
	ui.vPortsOverEthernetTableView->horizontalHeader()->setSectionResizeMode( QHeaderView::ResizeMode::Fixed );
	ui.vPortsOverEthernetTableView->horizontalHeader()->resizeSection( 0, 100 );
	ui.vPortsOverEthernetTableView->horizontalHeader()->resizeSection( 1, 50 );
	ui.vPortsOverEthernetTableView->verticalHeader()->setSectionResizeMode( QHeaderView::ResizeMode::Fixed );

	QPieSeries* cpuLoadSeries = new QPieSeries;
	cpuLoadSeries->append( new QPieSlice( "", 15 ) );
	cpuLoadSeries->append( new QPieSlice( "", 85 ) );
	cpuLoadSeries->slices()[0]->setColor( QColor( 76, 180, 221 ) );
	cpuLoadSeries->slices()[1]->setColor( QColor( 165, 218, 238 ) );
	cpuLoadSeries->setPieSize( 1.0 );
	cpuLoadSeries->setHoleSize( 0.8 );
	QChart* cpuLoadChart = new QChart;
	cpuLoadChart->legend()->hide();
	cpuLoadChart->setMargins( QMargins( 0, 0, 0, 0 ) );
	cpuLoadChart->layout()->setContentsMargins( 0, 0, 0, 0 );
	cpuLoadChart->setBackgroundRoundness( 0 );
	cpuLoadChart->addSeries( cpuLoadSeries );
	cpuLoadChart->setBackgroundVisible( false );
	ui.cpuLoadChartView->setChart( cpuLoadChart );
	ui.cpuLoadChartView->setRenderHint( QPainter::Antialiasing );
	
	QSplineSeries* cpuLoadSplineSeries = new QSplineSeries;
	for( int i = 0; i < 10 * 3; ++i )
		cpuLoadSplineSeries->append( i * 0.3333333, ( qrand() % 100 ) );
	cpuLoadSplineSeries->setColor( QColor( 76, 180, 221 ) );
	QScatterSeries* cpuLoadScatterSeries = new QScatterSeries;
	cpuLoadScatterSeries->append( cpuLoadSplineSeries->points().last() );
	cpuLoadScatterSeries->setColor( QColor( 44, 127, 159 ) );
	cpuLoadScatterSeries->setMarkerSize( 10 );
	QChart* cpuLoadLineChart = new QChart;
	cpuLoadLineChart->legend()->hide();
	cpuLoadLineChart->setMargins( QMargins( 0, 0, 0, 0 ) );
	cpuLoadLineChart->layout()->setContentsMargins( 0, 0, 0, 0 );
	cpuLoadLineChart->setBackgroundRoundness( 0 );
	cpuLoadLineChart->addSeries( cpuLoadSplineSeries );
	cpuLoadLineChart->addSeries( cpuLoadScatterSeries );
	cpuLoadLineChart->createDefaultAxes();
	cpuLoadLineChart->axisX()->setGridLineVisible( false );
	cpuLoadLineChart->axisX()->setVisible( false );
	cpuLoadLineChart->axisX()->setRange( 0, 10 );
	cpuLoadLineChart->axisY()->setGridLineVisible( false );
	cpuLoadLineChart->axisY()->setLabelsVisible( false );
	dynamic_cast< QValueAxis* >( cpuLoadLineChart->axisY() )->setTickCount( 2 );
	cpuLoadLineChart->axisY()->setRange( 0.0, 100.0 );
	cpuLoadLineChart->setBackgroundVisible( false );
	ui.cpuLoadSeriesChartView->setChart( cpuLoadLineChart );
	ui.cpuLoadSeriesChartView->setRenderHint( QPainter::Antialiasing );

	QPieSeries* memoryLoadSeries = new QPieSeries;
	memoryLoadSeries->append( new QPieSlice( "", 15 ) );
	memoryLoadSeries->append( new QPieSlice( "", 30 ) );
	memoryLoadSeries->append( new QPieSlice( "", 55 ) );
	memoryLoadSeries->slices()[0]->setColor( QColor( 44, 127, 159 ) );
	memoryLoadSeries->slices()[1]->setColor( QColor( 76, 180, 221 ) );
	memoryLoadSeries->slices()[2]->setColor( QColor( 165, 218, 238 ) );
	memoryLoadSeries->setPieSize( 1.0 );
	memoryLoadSeries->setHoleSize( 0.8 );
	QChart* memoryLoadChart = new QChart;
	memoryLoadChart->legend()->hide();
	memoryLoadChart->setMargins( QMargins( 0, 0, 0, 0 ) );
	memoryLoadChart->layout()->setContentsMargins( 0, 0, 0, 0 );
	memoryLoadChart->setBackgroundRoundness( 0 );
	memoryLoadChart->addSeries( memoryLoadSeries );
	memoryLoadChart->setBackgroundVisible( false );
	ui.memoryLoadChartView->setChart( memoryLoadChart );
	ui.memoryLoadChartView->setRenderHint( QPainter::Antialiasing );
	
	QPieSeries* sdLoadSeries = new QPieSeries;
	sdLoadSeries->append( new QPieSlice( "", 15 ) );
	sdLoadSeries->append( new QPieSlice( "", 85 ) );
	sdLoadSeries->slices()[0]->setColor( QColor( 76, 180, 221 ) );
	sdLoadSeries->slices()[1]->setColor( QColor( 165, 218, 238 ) );
	sdLoadSeries->setPieSize( 1.0 );
	sdLoadSeries->setHoleSize( 0.8 );
	QChart* sdLoadChart = new QChart;
	sdLoadChart->legend()->hide();
	sdLoadChart->setMargins( QMargins( 0, 0, 0, 0 ) );
	sdLoadChart->layout()->setContentsMargins( 0, 0, 0, 0 );
	sdLoadChart->setBackgroundRoundness( 0 );
	sdLoadChart->addSeries( sdLoadSeries );
	sdLoadChart->setBackgroundVisible( false );
	ui.sdLoadChartView->setChart( sdLoadChart );
	ui.sdLoadChartView->setRenderHint( QPainter::Antialiasing );

	resetDeviceLoad();

	monitoringDataViewMenu = new QMenu( this );
	monitoringDataViewMenu->addAction( "Copy value" );
	monitoringDataViewMenu->addAction( "Copy row" );
	monitoringDataViewMenu->addSeparator();
	monitoringDataViewMenu->addAction( "Expand all" );
	monitoringDataViewMenu->addAction( "Collapse all" );
	monitoringDataViewMenu->addSeparator();
	monitoringDataViewMenu->addAction( "Hexadecimal output" )->setCheckable( true );

	QObject::connect( ui.controlButton, &QToolButton::released, this, &MarvieController::mainMenuButtonClicked );
	QObject::connect( ui.monitoringButton, &QToolButton::released, this, &MarvieController::mainMenuButtonClicked );
	QObject::connect( ui.settingsButton, &QToolButton::released, this, &MarvieController::mainMenuButtonClicked );
	QObject::connect( ui.mainSettingsButton, &QToolButton::released, this, &MarvieController::settingsMenuButtonClicked );
	QObject::connect( ui.sensorsSettingsButton, &QToolButton::released, this, &MarvieController::settingsMenuButtonClicked );

	QObject::connect( ui.nextInterfaceButton, &QToolButton::released, this, &MarvieController::nextInterfaceButtonClicked );
	QObject::connect( ui.rs232ConnectButton, &QToolButton::released, this, &MarvieController::connectButtonClicked );
	QObject::connect( ui.ethernetConnectButton, &QToolButton::released, this, &MarvieController::connectButtonClicked );
	QObject::connect( ui.bluetoothConnectButton, &QToolButton::released, this, &MarvieController::connectButtonClicked );
	QObject::connect( &mlink, &MLinkClient::stateChanged, this, &MarvieController::mlinkStateChanged );
	QObject::connect( &mlink, &MLinkClient::newPacketAvailable, this, &MarvieController::mlinkNewPacketAvailable );
	QObject::connect( &mlink, &MLinkClient::newComplexPacketAvailable, this, &MarvieController::mlinkNewComplexPacketAvailable );
	QObject::connect( &mlink, &MLinkClient::complexDataSendingProgress, this, &MarvieController::mlinkComplexDataSendingProgress );
	QObject::connect( &mlink, &MLinkClient::complexDataReceivingProgress, this, &MarvieController::mlinkComplexDataReceivingProgress );

	QObject::connect( ui.addVPortOverGsmButton, &QToolButton::released, this, &MarvieController::addVPortOverIpButtonClicked );
	QObject::connect( ui.removeVPortOverGsmButton, &QToolButton::released, this, &MarvieController::removeVPortOverIpButtonClicked );
	QObject::connect( ui.addVPortOverEthernetButton, &QToolButton::released, this, &MarvieController::addVPortOverIpButtonClicked );
	QObject::connect( ui.removeVPortOverEthernetButton, &QToolButton::released, this, &MarvieController::removeVPortOverIpButtonClicked );

	QObject::connect( ui.comPortsConfigWidget, &ComPortsConfigWidget::assignmentChanged, this, &MarvieController::comPortAssignmentChanged );
	QObject::connect( &vPortsOverGsmModel, &VPortOverIpModel::dataChanged, this, &MarvieController::updateVPortsList );
	QObject::connect( &vPortsOverGsmModel, &VPortOverIpModel::rowsInserted, this, &MarvieController::updateVPortsList );
	QObject::connect( &vPortsOverGsmModel, &VPortOverIpModel::rowsRemoved, this, &MarvieController::updateVPortsList );
	QObject::connect( &vPortsOverEthernetModel, &VPortOverIpModel::dataChanged, this, &MarvieController::updateVPortsList );
	QObject::connect( &vPortsOverEthernetModel, &VPortOverIpModel::rowsInserted, this, &MarvieController::updateVPortsList );
	QObject::connect( &vPortsOverEthernetModel, &VPortOverIpModel::rowsRemoved, this, &MarvieController::updateVPortsList );

	QObject::connect( ui.sensorNameEdit, &QLineEdit::textChanged, &sensorNameTimer, static_cast< void( QTimer::* )( ) >( &QTimer::start ) );
	QObject::connect( ui.sensorNameEdit, &QLineEdit::returnPressed, this, &MarvieController::sensorNameEditReturnPressed );
	QObject::connect( &sensorNameTimer, &QTimer::timeout, this, &MarvieController::sensorNameTimerTimeout );

	QObject::connect( popupSensorsListWidget, &QTreeWidget::itemClicked, this, &MarvieController::sensorNameSearchCompleted );

	QObject::connect( ui.sensorAddButton, &QToolButton::clicked, this, &MarvieController::sensorAddButtonClicked );
	QObject::connect( ui.sensorRemoveButton, &QToolButton::clicked, this, &MarvieController::sensorRemoveButtonClicked );
	QObject::connect( ui.sensorMoveUpButton, &QToolButton::clicked, this, &MarvieController::sensorMoveButtonClicked );
	QObject::connect( ui.sensorMoveDownButton, &QToolButton::clicked, this, &MarvieController::sensorMoveButtonClicked );
	QObject::connect( ui.sensorsListClearButton, &QToolButton::clicked, this, &MarvieController::sensorsClearButtonClicked );

	QObject::connect( ui.targetDeviceComboBox, &QComboBox::currentTextChanged, this, &MarvieController::targetDeviceChanged );
	QObject::connect( ui.newConfigButton, &QToolButton::clicked, this, &MarvieController::newConfigButtonClicked );
	QObject::connect( ui.importConfigButton, &QToolButton::clicked, this, &MarvieController::importConfigButtonClicked );
	QObject::connect( ui.exportConfigButton, &QToolButton::clicked, this, &MarvieController::exportConfigButtonClicked );
	QObject::connect( ui.uploadConfigButton, &QToolButton::clicked, this, &MarvieController::uploadConfigButtonClicked );
	QObject::connect( ui.downloadConfigButton, &QToolButton::clicked, this, &MarvieController::downloadConfigButtonClicked );

	QObject::connect( ui.monitoringDataTreeView, &QTreeView::customContextMenuRequested, this, &MarvieController::monitoringDataViewMenuRequested );
	QObject::connect( monitoringDataViewMenu, &QMenu::triggered, this, &MarvieController::monitoringDataViewMenuActionTriggered );

	ui.rs232ComboBox->installEventFilter( this );

	targetDeviceChanged( ui.targetDeviceComboBox->currentText() );

	//////////////////////////////////////////////////////////////////////////
	ui.monitoringDataTreeView->setModel( &monitoringDataModel );
	ui.monitoringDataTreeView->header()->resizeSection( 0, 175 );
	ui.monitoringDataTreeView->header()->resizeSection( 1, 175 );
	struct Data
	{
		uint64_t dataTime;
		int a;
		int _reseved;
		float b[3];
		int _reseved2;
		double d;
		struct Ch 
		{
			uint8_t c;
			char str[3];
			uint16_t m[2];
			uint32_t _reseved;
		} ch[2];
	} data;
	data.a = 1;
	data.b[0] = 3.14f; data.b[1] = 0.0000042f; data.b[2] = 4200000000000.0f;
	data.d = 42.12345678900123456789;
	data.ch[0].c = 2;
	data.ch[0].str[0] = 'a'; data.ch[0].str[1] = 'b'; data.ch[0].str[2] = 'c';
	data.ch[0].m[0] = 3; data.ch[0].m[1] = 4;
	data.ch[1].c = 5;
	data.ch[1].str[0] = 'd'; data.ch[1].str[1] = 'e'; data.ch[1].str[2] = 'f';
	data.ch[1].m[0] = 6; data.ch[1].m[1] = 7;
	//for( int i  = 0; i < 32; ++i )
	updateSensorData( 0, "SimpleSensor", reinterpret_cast< uint8_t* >( &data ) );

	float ai[8];
	for( int i = 0; i < ARRAYSIZE( ai ); ++i )
		ai[i] = 0.1 * i;
	updateAnalogData( 0, ai, ARRAYSIZE( ai ) );
	uint16_t di = 0x4288;
	updateDiscreteData( 0, di, 16 );

	/*updateSensorData( 15, "SimpleSensor", reinterpret_cast< uint8_t* >( &data ) );

	updateAnalogData( 10, ai, ARRAYSIZE( ai ) );
	updateDiscreteData( 13, di, 16 );
	updateAnalogData( 15, ai, ARRAYSIZE( ai ) );
	updateDiscreteData( 15, di, 16 );
	updateAnalogData( 0, ai, ARRAYSIZE( ai ) );
	updateDiscreteData( 5, di, 16 );

	updateSensorData( 9, "SimpleSensor", reinterpret_cast< uint8_t* >( &data ) );*/

	ui.vPortTileListWidget->setTilesCount( 5 );
	ui.vPortTileListWidget->tile( 0 )->setNextSensorRead( 0, "CE301", 61 );
	ui.vPortTileListWidget->tile( 0 )->setNextSensorRead( 0, "CE301", 4 );
	ui.vPortTileListWidget->tile( 1 )->setNextSensorRead( 1, "CE301", 0 );
	ui.vPortTileListWidget->tile( 2 )->setNextSensorRead( 2, "CE301", 5);
	ui.vPortTileListWidget->tile( 2 )->resetNextSensorRead();


	QTimer* timer = new QTimer;
	timer->setInterval( 1000 / DEVICE_LOAD_FREQ );
	timer->setSingleShot( false );
	QObject::connect( timer, &QTimer::timeout, [this]() 
	{
		DeviceLoad load;
		load.cpuLoad = qrand() % 100;
		load.totalMemory = 128 * 1024;
		load.allocatedCoreMemory = 64 * 1024;
		load.allocatedHeapMemory = 32 * 1024;
		load.sdCapacity = 4 * 1024 * 1024 * 1024ULL;
		load.freeSdSpace = 1 * 1024 * 1024 * 1024ULL;
		updateDeviceLoad( &load );
	} );
	timer->start();
}

MarvieController::~MarvieController()
{
	popupSensorsListWidget->deleteLater();
}

bool MarvieController::eventFilter( QObject *obj, QEvent *event )
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
	if( obj == popupSensorsListWidget )
	{
		if( event->type() == QEvent::MouseButtonPress )
		{
			popupSensorsListWidget->hide();
			popupSensorsListWidget->setFocus();
			return true;
		}

		if( event->type() == QEvent::KeyPress )
		{
			bool consumed = false;
			int key = static_cast< QKeyEvent* >( event )->key();
			switch( key )
			{
			case Qt::Key_Enter:
			case Qt::Key_Return:
				sensorNameSearchCompleted();
				consumed = true;
				break;
			case Qt::Key_Escape:
				ui.sensorNameEdit->setFocus();
				popupSensorsListWidget->hide();
				consumed = true;
				break;
			case Qt::Key_Up:
			case Qt::Key_Down:
			case Qt::Key_Home:
			case Qt::Key_End:
			case Qt::Key_PageUp:
			case Qt::Key_PageDown:
				break;
			default:
				ui.sensorNameEdit->setFocus();
				ui.sensorNameEdit->event( event );
				popupSensorsListWidget->hide();
				break;
			}

			return consumed;
		}
	}

	return false;
}

void MarvieController::mainMenuButtonClicked()
{
	QToolButton* buttons[] = { ui.controlButton, ui.monitoringButton, ui.settingsButton };
	QObject* s = sender();
	for( int i = 0; i < sizeof( buttons ) / sizeof( *buttons ); ++i )
	{
		if( buttons[i] == s )
		{
			ui.mainStackedWidget->setCurrentIndex( i );
			buttons[i]->setChecked( true );
		}
		else
			buttons[i]->setChecked( false );
	}
}

void MarvieController::settingsMenuButtonClicked()
{
	QToolButton* buttons[] = { ui.mainSettingsButton, ui.sensorsSettingsButton };
	QObject* s = sender();
	for( int i = 0; i < sizeof( buttons ) / sizeof( *buttons ); ++i )
	{
		if( buttons[i] == s )
		{
			ui.settingsStackedWidget->setCurrentIndex( i );
			buttons[i]->setChecked( true );
		}
		else
			buttons[i]->setChecked( false );
	}
}

void MarvieController::nextInterfaceButtonClicked()
{
	ui.interfaceStackedWidget->setCurrentIndex( ( ui.interfaceStackedWidget->currentIndex() + 1 ) % 3 );
}

void MarvieController::connectButtonClicked()
{
	if( mlink.state() != MLinkClient::State::Disconnected )
	{
		mlink.disconnectFromHost();
		return;
	}

	if( mlinkIODevice )
	{
		mlink.setIODevice( nullptr );
		mlinkIODevice->close();
		delete mlinkIODevice;
		mlinkIODevice = nullptr;
	}

	if( sender() == ui.rs232ConnectButton )
	{
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

		mlinkIODevice = port;
		mlink.setIODevice( mlinkIODevice );
		mlink.connectToHost();
		ui.rs232ComboBox->setEnabled( false );
	}
	else if( sender() == ui.ethernetConnectButton )
	{
		ui.nextInterfaceButton->setEnabled( true );
	}
	else if( sender() == ui.bluetoothConnectButton )
	{
		ui.nextInterfaceButton->setEnabled( true );
	}
	ui.nextInterfaceButton->setEnabled( false );
}

void MarvieController::monitoringDataViewMenuRequested( const QPoint& point )
{
	//QModelIndex index = table->indexAt( pos );
	monitoringDataViewMenu->popup( ui.monitoringDataTreeView->viewport()->mapToGlobal( point ) );
}

void MarvieController::monitoringDataViewMenuActionTriggered( QAction* action )
{
	if( action->text() == "Copy value" )
	{
		QModelIndex index = ui.monitoringDataTreeView->currentIndex();
		if( !index.isValid() )
			return;
		QApplication::clipboard()->setText( index.sibling( index.row(), 1 ).data( Qt::DisplayRole ).toString() );
	}
	else if( action->text() == "Copy row" )
	{
		QModelIndex index = ui.monitoringDataTreeView->currentIndex();
		if( !index.isValid() )
			return;
		QString fullName( index.sibling( index.row(), 0 ).data( Qt::DisplayRole ).toString() );
		QString part = fullName;
		QModelIndex parent = index.parent();
		while( parent.isValid() )
		{
			if( part[0] != '[' )
				fullName.insert( 0, '.' );
			part = parent.data( Qt::DisplayRole ).toString();
			fullName.insert( 0, part );
			parent = parent.parent();
		}
		QApplication::clipboard()->setText( fullName + '\t' +
											index.sibling( index.row(), 1 ).data( Qt::DisplayRole ).toString() + '\t' +
											index.sibling( index.row(), 2 ).data( Qt::DisplayRole ).toString() );
	}
	else if( action->text() == "Expand all" )
		ui.monitoringDataTreeView->expandAll();
	else if( action->text() == "Collapse all" )
		ui.monitoringDataTreeView->collapseAll();
	else if( action->text() == "Hexadecimal output" )
		monitoringDataModel.setHexadecimalOutput( action->isChecked() );
}

void MarvieController::targetDeviceChanged( QString text )
{
	static QVector< QVector< ComPortsConfigWidget::Assignment > > qxPortAssignments = []()
	{
		QVector< QVector< ComPortsConfigWidget::Assignment > > portAssignments;
		QVector< ComPortsConfigWidget::Assignment > com0;
		com0.append( ComPortsConfigWidget::Assignment::VPort );
		com0.append( ComPortsConfigWidget::Assignment::GsmModem );
		com0.append( ComPortsConfigWidget::Assignment::ModbusRtuSlave );
		portAssignments.append( com0 );

		QVector< ComPortsConfigWidget::Assignment > com1;
		com1.append( ComPortsConfigWidget::Assignment::VPort );
		com1.append( ComPortsConfigWidget::Assignment::ModbusRtuSlave );
		portAssignments.append( com1 );

		QVector< ComPortsConfigWidget::Assignment > com2;
		com2.append( ComPortsConfigWidget::Assignment::VPort );
		com2.append( ComPortsConfigWidget::Assignment::ModbusRtuSlave );
		com2.append( ComPortsConfigWidget::Assignment::Multiplexer );
		portAssignments.append( com2 );

		return portAssignments;
	}();
	static QVector< QVector< ComPortsConfigWidget::Assignment > > vxPortAssignments = []()
	{
		QVector< QVector< ComPortsConfigWidget::Assignment > > portAssignments;		

		return portAssignments;
	}();

	if( text == "QX" )
	{
		ui.comPortsConfigWidget->init( qxPortAssignments );
		ui.ethernetConfigWidget->show();
	}
	else
	{
		ui.comPortsConfigWidget->init( vxPortAssignments );
		ui.ethernetConfigWidget->hide();
		vPortsOverEthernetModel.removeRows( 0, vPortsOverEthernetModel.rowCount() );
	}
}

void MarvieController::newConfigButtonClicked()
{
	targetDeviceChanged( ui.targetDeviceComboBox->currentText() );

	ui.gsmModbusRtuCheckBox->setCheckState( Qt::Unchecked );
	ui.gsmModbusRtuSpinBox->setValue( 502 );
	ui.gsmModbusTcpCheckBox->setCheckState( Qt::Unchecked );
	ui.gsmModbusTcpSpinBox->setValue( 503 );
	ui.gsmModbusAsciiCheckBox->setCheckState( Qt::Unchecked );
	ui.gsmModbusAsciiSpinBox->setValue( 504 );

	ui.ethernetModbusRtuCheckBox->setCheckState( Qt::Unchecked );
	ui.ethernetModbusRtuSpinBox->setValue( 502 );
	ui.ethernetModbusTcpCheckBox->setCheckState( Qt::Unchecked );
	ui.ethernetModbusTcpSpinBox->setValue( 503 );
	ui.ethernetModbusAsciiCheckBox->setCheckState( Qt::Unchecked );
	ui.ethernetModbusAsciiSpinBox->setValue( 504 );

	vPortsOverGsmModel.removeRows( 0, vPortsOverGsmModel.rowCount() );
	vPortsOverEthernetModel.removeRows( 0, vPortsOverEthernetModel.rowCount() );

	sensorsClearButtonClicked();
}

void MarvieController::importConfigButtonClicked()
{
	QSettings setting( "settings.ini", QSettings::Format::IniFormat );
	QString name = QFileDialog::getOpenFileName( this, "Import config file", setting.value( "confDir", QDir::currentPath() ).toString(), "XML files (*.xml)" );
	if( name.isEmpty() )
		return;
	setting.setValue( "confDir", QDir( name ).absolutePath().remove( QDir( name ).dirName() ) );
	QFile file( name );
	
	file.open( QIODevice::ReadOnly );
	if( !loadConfigFromXml( file.readAll() ) )
	{
		// ADD // FIX
	}
}

void MarvieController::exportConfigButtonClicked()
{
	QByteArray data = saveConfigToXml();
	if( data.isEmpty() )
	{
		// ADD // FIX
		return;
	}

	QSettings setting( "settings.ini", QSettings::Format::IniFormat );
	QString name = QFileDialog::getSaveFileName( this, "Export config file", setting.value( "confDir", QDir::currentPath() ).toString(), "XML files (*.xml)" );
	if( name.isEmpty() )
		return;
	setting.setValue( "confDir", QDir( name ).absolutePath().remove( QDir( name ).dirName() ) );
	
	QFile file( name );
	file.open( QIODevice::WriteOnly );
	file.write( data );
}

void MarvieController::uploadConfigButtonClicked()
{

}

void MarvieController::downloadConfigButtonClicked()
{

}

void MarvieController::addVPortOverIpButtonClicked()
{
	if( sender() == ui.addVPortOverGsmButton )
		vPortsOverGsmModel.insertRows( vPortsOverGsmModel.rowCount(), 1 );
	else
		vPortsOverEthernetModel.insertRows( vPortsOverEthernetModel.rowCount(), 1 );
}

void MarvieController::removeVPortOverIpButtonClicked()
{
	if( sender() == ui.removeVPortOverGsmButton )
		vPortsOverGsmModel.removeRows( ui.vPortsOverGsmTableView->currentIndex().row(), 1 );
	else
		vPortsOverEthernetModel.removeRows( ui.vPortsOverEthernetTableView->currentIndex().row(), 1 );
}

void MarvieController::comPortAssignmentChanged( unsigned int id, ComPortsConfigWidget::Assignment previous, ComPortsConfigWidget::Assignment current )
{
	if( previous == ComPortsConfigWidget::Assignment::GsmModem )
	{
		ui.gsmModemConfigWidget->hide();
		vPortsOverGsmModel.removeRows( 0, vPortsOverGsmModel.rowCount() );
	}
	if( current == ComPortsConfigWidget::Assignment::GsmModem )
		ui.gsmModemConfigWidget->show();
	updateVPortsList();
}

void MarvieController::updateVPortsList()
{
	vPorts.clear();
	auto comAssignments = ui.comPortsConfigWidget->assignments();
	for( int i = 0; i < comAssignments.size(); ++i )
	{
		if( comAssignments[i] == ComPortsConfigWidget::Assignment::VPort )
			vPorts.append( QString( "COM%1" ).arg( i ) );
		if( comAssignments[i] == ComPortsConfigWidget::Assignment::Multiplexer )
		{
			for( int i2 = 0; i2 < 5; ++i2 )
				vPorts.append( QString( "Multiplexer.COM%1" ).arg( i2 ) );
		}
	}

	auto vPortsOverIp = vPortsOverGsmModel.modelData();
	for( const auto& i : vPortsOverIp )
		vPorts.append( QString( "Gsm[%1]" ).arg( i ) );

	vPortsOverIp = vPortsOverEthernetModel.modelData();
	for( const auto& i : vPortsOverIp )
		vPorts.append( QString( "Ethernet[%1]" ).arg( i ) );
}

void MarvieController::sensorNameEditReturnPressed()
{
	sensorNameTimer.stop();
	sensorNameTimerTimeout();
}

void MarvieController::sensorNameTimerTimeout()
{
	if( ui.sensorNameEdit->text().isEmpty() )
		return;
	QRegExp reg( ui.sensorNameEdit->text() );
	reg.setCaseSensitivity( Qt::CaseInsensitive );
	if( !reg.isValid() )
		return;
	QStringList supportedSensorsList = sensorDescMap.keys();
	QStringList matches;
	for( const auto& i : supportedSensorsList )
	{
		if( reg.indexIn( i ) != -1 )
			matches.append( i );
	}
	qSort( matches.begin(), matches.end(), []( const QString& a, const QString& b )
	{
		return a.size() < b.size();
	} );

	if( matches.isEmpty() )
		return;
	else
		matches = matches.mid( 0, 10 );

	const QPalette &pal = ui.sensorNameEdit->palette();
	QColor color = pal.color( QPalette::Disabled, QPalette::WindowText );

	popupSensorsListWidget->setUpdatesEnabled( false );
	popupSensorsListWidget->clear();

	for( const auto &i : matches )
	{
		auto item = new QTreeWidgetItem( popupSensorsListWidget );
		item->setText( 0, i );
		item->setTextColor( 0, color );
	}

	popupSensorsListWidget->setCurrentItem( popupSensorsListWidget->topLevelItem( 0 ) );
	popupSensorsListWidget->resizeColumnToContents( 0 );
	popupSensorsListWidget->setUpdatesEnabled( true );

	popupSensorsListWidget->move( ui.sensorNameEdit->mapToGlobal( QPoint( 0, ui.sensorNameEdit->height() ) ) );
	popupSensorsListWidget->setFocus();
	popupSensorsListWidget->show();
}

void MarvieController::sensorNameSearchCompleted()
{
	sensorNameTimer.stop();
	popupSensorsListWidget->hide();
	ui.sensorNameEdit->setFocus();
	QTreeWidgetItem *item = popupSensorsListWidget->currentItem();
	if( item )
	{
		ui.sensorNameEdit->blockSignals( true );
		ui.sensorNameEdit->setText( item->text( 0 ) );
		ui.sensorNameEdit->blockSignals( false );
		//sensorAddButtonClicked();
	}
}

void MarvieController::sensorAddButtonClicked()
{
	if( !sensorDescMap.contains( ui.sensorNameEdit->text() ) )
		return;
	insertSensorSettings( ui.sensorSettingsTreeWidget->topLevelItemCount(), ui.sensorNameEdit->text() );
	ui.sensorSettingsTreeWidget->setCurrentIndex( ui.sensorSettingsTreeWidget->model()->index( ui.sensorSettingsTreeWidget->topLevelItemCount() - 1, 0 ) );
}

void MarvieController::sensorRemoveButtonClicked()
{
	auto index = ui.sensorSettingsTreeWidget->currentIndex();
	if( !index.isValid() )
		return;
	if( index.parent().isValid() )
		index = index.parent();
	int row = index.row();
	removeSensorSettings( row, true );
	if( row < ui.sensorSettingsTreeWidget->topLevelItemCount() )
		ui.sensorSettingsTreeWidget->setCurrentIndex( ui.sensorSettingsTreeWidget->model()->index( row, 0 ) );
	else
		ui.sensorSettingsTreeWidget->setCurrentIndex( ui.sensorSettingsTreeWidget->model()->index( ui.sensorSettingsTreeWidget->topLevelItemCount() - 1, 0 ) );
}

void MarvieController::sensorMoveButtonClicked()
{
	auto item = ui.sensorSettingsTreeWidget->currentItem();
	if( !item || ui.sensorSettingsTreeWidget->topLevelItemCount() == 1 )
		return;
	if( item->parent() )
		item = item->parent();
	int index = ui.sensorSettingsTreeWidget->indexOfTopLevelItem( item );
	if( sender() == ui.sensorMoveUpButton )
	{
		if( index == 0 )
			return;
		auto values = sensorSettingsValues( index );
		QString sensorName = item->text( 0 ).split( ". " )[1];
		bool exp = item->isExpanded();
		setSensorSettingsNameNum( index - 1, index + 1 );
		removeSensorSettings( index, false );
		insertSensorSettings( index - 1, sensorName, values )->setExpanded( exp );
		ui.sensorSettingsTreeWidget->setCurrentIndex( ui.sensorSettingsTreeWidget->model()->index( index - 1, 0 ) );
	}
	else
	{
		if( index == ui.sensorSettingsTreeWidget->topLevelItemCount() - 1 )
			return;
		auto values = sensorSettingsValues( index );
		QString sensorName = item->text( 0 ).split( ". " )[1];
		bool exp = item->isExpanded();
		setSensorSettingsNameNum( index + 1, index + 1 );
		removeSensorSettings( index, false );
		insertSensorSettings( index + 1, sensorName, values )->setExpanded( exp );
		ui.sensorSettingsTreeWidget->setCurrentIndex( ui.sensorSettingsTreeWidget->model()->index( index + 1, 0 ) );
	}
}

void MarvieController::sensorsClearButtonClicked()
{
	ui.sensorSettingsTreeWidget->clear();
}

void MarvieController::mlinkStateChanged( MLinkClient::State s )
{
	if( ui.interfaceStackedWidget->currentWidget() == ui.rs232Page )
	{
		switch( s )
		{
		case MLinkClient::State::Disconnected:
			ui.nextInterfaceButton->setEnabled( true );
			ui.rs232ComboBox->setEnabled( true );
			ui.rs232ConnectButton->setIcon( QIcon( ":/MarvieController/icons/icons8-rs-232-female-filled-50.png" ) );
			break;
		case MLinkClient::State::Connecting:
			ui.rs232ConnectButton->setIcon( QIcon( ":/MarvieController/icons/icons8-rs-232-female-filled-50-orange.png" ) );
			break;
		case MLinkClient::State::Connected:
			ui.rs232ConnectButton->setIcon( QIcon( ":/MarvieController/icons/icons8-rs-232-female-filled-50-green.png" ) );
			break;
		case MLinkClient::State::Disconnecting:
			ui.rs232ConnectButton->setIcon( QIcon( ":/MarvieController/icons/icons8-rs-232-female-filled-50-orange.png" ) );
			break;
		default:
			break;
		}
	}
	else if( ui.interfaceStackedWidget->currentWidget() == ui.ethernetPage )
	{

	}
	else if( ui.interfaceStackedWidget->currentWidget() == ui.bluetoothPage )
	{

	}
}

void MarvieController::mlinkNewPacketAvailable( uint8_t type, QByteArray data )
{

}

void MarvieController::mlinkNewComplexPacketAvailable( uint8_t channelId, QString name, QByteArray data )
{

}

void MarvieController::mlinkComplexDataSendingProgress( uint8_t channelId, QString name, float progress )
{

}

void MarvieController::mlinkComplexDataReceivingProgress( uint8_t channelId, QString name, float progress )
{

}

void MarvieController::sensorsInit()
{
	QCryptographicHash hash( QCryptographicHash::Sha512 ), hashTmp( QCryptographicHash::Sha512 );

	QXmlSchema settingsDescSchema;
	settingsDescSchema.setMessageHandler( &xmlMessageHandler );
	settingsDescSchema.load( QUrl::fromLocalFile( ":/MarvieController/Xml/schemas/SensorSettings.xsd" ) );
	QXmlSchemaValidator settingsDescValidator( settingsDescSchema );

	QXmlSchema dataDescSchema;
	dataDescSchema.setMessageHandler( &xmlMessageHandler );
	dataDescSchema.load( QUrl::fromLocalFile( ":/MarvieController/Xml/schemas/SensorData.xsd" ) );
	QXmlSchemaValidator dataDescValidator( dataDescSchema );

	QDir dir( "Sensors" );
	auto dirs = dir.entryList( QDir::Filter::Dirs | QDir::Filter::NoDotAndDotDot );
	for( auto i : dirs )
	{
		if( i.contains( '*' ) )
		{
			// FIX: add a log entry
			continue;
		}
		QFile file( QString( "Sensors/" ) + i + "/SettingsDesc.xml" );
		if( !file.exists() )
			continue;
		file.open( QIODevice::ReadOnly );
		QByteArray data = file.readAll();
		hash.addData( data );
		hashTmp.addData( data );
		file.close();
		if( !settingsDescValidator.validate( data, QUrl::fromLocalFile( "GG42" ) ) )
		{
			// FIX: add a log entry
			continue;
		}
		SensorDesc desc;

		QDomDocument doc;
		if( !doc.setContent( data ) )
		{
			// FIX: add a log entry
			continue;
		}
		auto c0 = doc.firstChildElement( "SettingsDesc" );
		if( c0.attribute( "target" ) == "BRSensor" )
			desc.settings.target = SensorDesc::Settings::Target::BR;
		else
			desc.settings.target = SensorDesc::Settings::Target::SR;
		auto c1 = c0.firstChildElement();
		typedef QSharedPointer< SensorDesc::Settings::Parameter > PrmPointer;
		QSet< QString > namesSet;
		while( !c1.isNull() )
		{
			if( c1.tagName() == "int" )
			{
				static const QString minStr = QString( "%1" ).arg( std::numeric_limits< int >::min() );
				static const QString maxStr = QString( "%1" ).arg( std::numeric_limits< int >::max() );
				desc.settings.prmList.append( PrmPointer(
					new SensorDesc::Settings::IntParameter( c1.text(),
															c1.attribute( "default" ).toInt(),
															c1.attribute( "min", minStr ).toInt(),
															c1.attribute( "max", maxStr ).toInt() ) ) );
			}
			else if( c1.tagName() == "float" )
			{
				static const QString minStr = QString( "%1" ).arg( std::numeric_limits< float >::min() );
				static const QString maxStr = QString( "%1" ).arg( std::numeric_limits< float >::max() );
				desc.settings.prmList.append( PrmPointer(
					new SensorDesc::Settings::FloatParameter( c1.text(),
															  c1.attribute( "default" ).toFloat(),
															  c1.attribute( "min", minStr ).toFloat(),
															  c1.attribute( "max", maxStr ).toFloat() ) ) );
			}
			else if( c1.tagName() == "double" )
			{
				static const QString minStr = QString( "%1" ).arg( std::numeric_limits< double >::min() );
				static const QString maxStr = QString( "%1" ).arg( std::numeric_limits< double >::max() );
				desc.settings.prmList.append( PrmPointer(
					new SensorDesc::Settings::DoubleParameter( c1.text(),
															   c1.attribute( "default" ).toDouble(),
															   c1.attribute( "min", minStr ).toDouble(),
															   c1.attribute( "max", maxStr ).toDouble() ) ) );
			}
			else if( c1.tagName() == "bool" )
			{
				QString str = c1.attribute( "default" );
				bool v = false;
				if( str == "true" || str == "1" )
					v = true;
				desc.settings.prmList.append( PrmPointer( new SensorDesc::Settings::BoolParameter( c1.text(), v ) ) );
			}
			else if( c1.tagName() == "string" )
			{
				QString defaultStr = c1.attribute( "default" );
				QString patternStr = c1.attribute( "pattern" );
				QRegExp reg( patternStr );
				if( !patternStr.isEmpty() )
				{
					if( !reg.isValid() || !reg.exactMatch( defaultStr ) )
					{
						// FIX: add a log entry
						goto Skip;
					}
				}
				desc.settings.prmList.append( PrmPointer( new SensorDesc::Settings::StringParameter( c1.text(), defaultStr, reg ) ) );
			}
			else if( c1.tagName() == "enum" )
			{
				auto c2 = c1.firstChildElement();
				QString name = c2.text();
				QMap< QString, int > map;
				int lastIndex = 0;
				c2 = c2.nextSiblingElement();
				QString defaultValue = c2.text();
				c2 = c2.nextSiblingElement();
				while( !c2.isNull() )
				{
					QString s = c2.attribute( "value" );
					if( !s.isEmpty() )
						lastIndex = s.toInt();
					map[c2.text()] = lastIndex++;
					c2 = c2.nextSiblingElement();
				}
				if( !map.contains( defaultValue ) )
				{
					// FIX: add a log entry
					goto Skip;
				}
				desc.settings.prmList.append( PrmPointer( new SensorDesc::Settings::EnumParameter( name, map, defaultValue ) ) );
			}
			if( namesSet.contains( desc.settings.prmList.last()->name ) )
			{
				// FIX: add a log entry
				goto Skip;
			}
			namesSet.insert( desc.settings.prmList.last()->name );
			c1 = c1.nextSiblingElement();
		}

		file.setFileName( QString( "Sensors/" ) + i + "/DataDesc.xml" );
		if( file.exists() )
		{
			file.open( QIODevice::ReadOnly );
			data = file.readAll();
			hash.addData( data );
			hashTmp.addData( data );
			file.close();
			if( dataDescValidator.validate( data, QUrl::fromLocalFile( "GG42" ) ) && doc.setContent( data ) )
			{
				c0 = doc.firstChildElement( "DataDesc" );
				c1 = c0.firstChildElement();
				typedef QSharedPointer< SensorDesc::Data::Node > DataPointer;
				static const std::function< int( SensorDesc::Data::Node* node, QDomElement c, int bias ) > parse = []( SensorDesc::Data::Node* node, QDomElement c, int bias ) -> int
				{
					while( !c.isNull() )
					{
						SensorDesc::Data::Type type = SensorDesc::Data::toType( c.tagName() );
						int biasAttr = c.attribute( "bias", "-1" ).toInt();
						if( biasAttr != -1 )
							bias = biasAttr;
						switch( type )
						{
						case MarvieController::SensorDesc::Data::Type::Array:
						{
							SensorDesc::Data::Type arrayType = SensorDesc::Data::toType( c.attribute( "type" ) );
							int count = c.attribute( "count" ).toInt();
							auto newNode = new SensorDesc::Data::Node( type, bias, c.text() );
							for( int i = 0; i < count; ++i )
							{
								newNode->childNodes.append( new SensorDesc::Data::Node( arrayType, bias, QString( "[%1]" ).arg( i ) ) );
								bias += SensorDesc::Data::typeSize( arrayType );
							}
							node->childNodes.append( newNode );
							break;
						}
						case MarvieController::SensorDesc::Data::Type::Group:
						{
							auto newNode = new SensorDesc::Data::Node( type, bias, c.attribute( "name" ) );
							bias = parse( newNode, c.firstChildElement(), bias );
							node->childNodes.append( newNode );
							break;
						}
						case MarvieController::SensorDesc::Data::Type::GroupArray:
						{
							int count = c.attribute( "count" ).toInt();
							auto c2 = c.firstChildElement();
							auto c3 = c2.firstChildElement();
							auto groupArrayNode = new SensorDesc::Data::Node( SensorDesc::Data::Type::Group, bias, c2.attribute( "name" ) );
							for( int i = 0; i < count; ++i )
							{
								auto newNode = new SensorDesc::Data::Node( SensorDesc::Data::Type::Group, bias, QString( "[%1]" ).arg( i ) );
								bias = parse( newNode, c3, bias );
								groupArrayNode->childNodes.append( newNode );
							}
							node->childNodes.append( groupArrayNode );
							break;
						}
						case MarvieController::SensorDesc::Data::Type::Unused:
							bias += c.attribute( "size" ).toInt();
							break;
						default:
							node->childNodes.append( new SensorDesc::Data::Node( type, bias, c.text() ) );
							bias += SensorDesc::Data::typeSize( type );
							break;
						}
						c = c.nextSiblingElement();
					}
					return bias;
				};
				SensorDesc::Data::Node* root = new SensorDesc::Data::Node( SensorDesc::Data::Type::Group, 0 );
				parse( root, c1, 0 );
				desc.data.root = DataPointer( root );
			}
		}

		sensorDescMap[i] = desc;
Skip:
		volatile int nop = 42;
	}

	QDir().mkdir( "Xml" );
	QDir( "Xml" ).mkdir( "Schemas" );
	auto checkAndRestore = []( QString srcPath, QString dstPath )
	{
		QFile srcFile( srcPath );
		srcFile.open( QIODevice::ReadOnly );
		QByteArray srcData = srcFile.readAll();
		QFile dstFile( dstPath );
		QByteArray dstData;
		if( dstFile.exists() )
		{
			dstFile.open( QIODevice::ReadOnly );
			dstData = dstFile.readAll();
			dstFile.close();
		}
		if( srcData != dstData )
		{
			dstFile.remove();
			dstFile.open( QIODevice::WriteOnly );
			dstFile.write( srcData );
			dstFile.close();
		}

		return srcData;
	};

	QByteArray baseXsdData = checkAndRestore( ":/MarvieController/Xml/schemas/Base.xsd", "Xml/Schemas/Base.xsd" );
	checkAndRestore( ":/MarvieController/Xml/schemas/SensorSettings.xsd", "Xml/Schemas/SensorSettings.xsd" );
	checkAndRestore( ":/MarvieController/Xml/schemas/SensorData.xsd", "Xml/Schemas/SensorData.xsd" );
	QByteArray configXsdData = checkAndRestore( ":/MarvieController/Xml/schemas/Config.xsd", "Xml/Schemas/Config.xsd" );

	QFile sensorsXsd( "Xml/Schemas/Sensors.xsd" );
	sensorsXsd.open( QIODevice::ReadWrite );
	QByteArray sensorsXsdData = sensorsXsd.readAll();
	hashTmp.addData( sensorsXsdData );
	sensorsXsd.close();

	QSettings settings( "settings.ini", QSettings::IniFormat );
	QByteArray hashStr = QByteArray::fromHex( settings.value( "hash", "" ).toByteArray() );
	if( hashStr.isEmpty() || hashStr != hashTmp.result() )
	{
		sensorsXsdData = generateSensorsXSD();
		hash.addData( sensorsXsdData );
		settings.setValue( "hash", hash.result().toHex() );
		sensorsXsd.remove();
		sensorsXsd.open( QIODevice::WriteOnly );
		sensorsXsd.write( sensorsXsdData );
		sensorsXsd.close();
	}

	const auto sa = "<xs:schema xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">";
	const auto sb = "</xs:schema>";

	auto cut = []( QByteArray& data, const char* const begin, const char* const end )
	{
		int i = data.indexOf( begin ) + qstrlen( begin );
		int i2 = data.indexOf( end );
		return data.mid( i, i2 - 1 - i );
	};

	baseXsdData = cut( baseXsdData, sa, sb );
	sensorsXsdData.replace( "<xs:include schemaLocation=\"Base.xsd\"/>", baseXsdData );
	sensorsXsdData = cut( sensorsXsdData, sa, sb );
	configXsdData.replace( "<xs:include schemaLocation=\"Sensors.xsd\"/>", sensorsXsdData );

	configValidator.setMessageHandler( &xmlMessageHandler );
	static QXmlSchema schema;
	schema.load( configXsdData );
	configValidator.setSchema( schema );
}

QByteArray MarvieController::generateSensorsXSD()
{
	QDomDocument doc;
	auto root = doc.createElement( "xs:schema" );
	root.setAttribute( "xmlns:xs", "http://www.w3.org/2001/XMLSchema" );
	auto includeElement = doc.createElement( "xs:include" );
	includeElement.setAttribute( "schemaLocation", "Base.xsd" );
	doc.appendChild( root );
	root.appendChild( includeElement );

	auto c0 = doc.createElement( "xs:element" );
	auto c1 = doc.createElement( "xs:complexType" );
	auto c2 = doc.createElement( "xs:sequence" );
	auto c3 = doc.createElement( "xs:choice" );
	c0.setAttribute( "name", "sensorsConfig" );
	c3.setAttribute( "minOccurs", 1 );
	c3.setAttribute( "maxOccurs", "unbounded" );
	for( auto i = sensorDescMap.begin(); i != sensorDescMap.end(); ++i )
	{
		auto c4 = doc.createElement( "xs:element" );
		c4.setAttribute( "ref", i.key() );
		c3.appendChild( c4 );
	}
	root.appendChild( c0 );
	c0.appendChild( c1 );
	c1.appendChild( c2 );
	c2.appendChild( c3 );

	root.appendChild( doc.createComment( "===============================================" ) );
	root.appendChild( doc.createComment( "===============================================" ) );
	root.appendChild( doc.createComment( "===============================================" ) );

	// Sensor elements
	for( auto i = sensorDescMap.begin(); i != sensorDescMap.end(); ++i )
	{
		auto c0 = doc.createElement( "xs:element" );
		root.appendChild( c0 );
		c0.setAttribute( "name", i.key() );
		auto c1 = doc.createElement( "xs:complexType" );
		c0.appendChild( c1 );
		auto c2 = doc.createElement( "xs:complexContent" );
		c1.appendChild( c2 );
		auto c3 = doc.createElement( "xs:extension" );
		if( i->settings.target == SensorDesc::Settings::Target::BR )
			c3.setAttribute( "base", "brSensorBase" );
		else
			c3.setAttribute( "base", "srSensorBase" );
		c2.appendChild( c3 );
		auto c4 = doc.createElement( "xs:sequence" );
		c3.appendChild( c4 );
		for( auto i2 = i->settings.prmList.begin(); i2 != i->settings.prmList.end(); ++i2 )
		{
			switch( ( *i2 )->type )
			{
			case SensorDesc::Settings::Parameter::Type::Int:
			{
				SensorDesc::Settings::IntParameter* prm = static_cast< SensorDesc::Settings::IntParameter* >( ( *i2 ).data() );
				auto c5 = doc.createElement( "xs:element" );
				c5.setAttribute( "name", prm->name );
				if( prm->max == std::numeric_limits< int >::max() && prm->min == std::numeric_limits< int >::min() )
					c5.setAttribute( "type", "xs:int" );
				else
				{
					auto c6 = doc.createElement( "xs:simpleType" );
					auto c7 = doc.createElement( "xs:restriction" );
					c7.setAttribute( "base", "xs:int" );
					if( prm->min != std::numeric_limits< int >::min() )
					{
						auto c8 = doc.createElement( "xs:minInclusive" );
						c8.setAttribute( "value", prm->min );
						c7.appendChild( c8 );
					}
					if( prm->max != std::numeric_limits< int >::max() )
					{
						auto c8 = doc.createElement( "xs:maxInclusive" );
						c8.setAttribute( "value", prm->max );
						c7.appendChild( c8 );
					}
					c5.appendChild( c6 );
					c6.appendChild( c7 );
				}
				c4.appendChild( c5 );
				break;
			}
			case SensorDesc::Settings::Parameter::Type::Float:
			{
				SensorDesc::Settings::FloatParameter* prm = static_cast< SensorDesc::Settings::FloatParameter* >( ( *i2 ).data() );
				auto c5 = doc.createElement( "xs:element" );
				c5.setAttribute( "name", prm->name );
				if( prm->min <= std::numeric_limits< float >::min() * 10.0f && prm->max >= std::numeric_limits< float >::max() / 10.0f )
					c5.setAttribute( "type", "xs:float" );
				else
				{
					auto c6 = doc.createElement( "xs:simpleType" );
					auto c7 = doc.createElement( "xs:restriction" );
					c7.setAttribute( "base", "xs:float" );
					if( prm->min > std::numeric_limits< float >::min() * 10.0f )
					{
						auto c8 = doc.createElement( "xs:minInclusive" );
						c8.setAttribute( "value", prm->min );
						c7.appendChild( c8 );
					}
					if( prm->max < std::numeric_limits< float >::max() / 10.0f )
					{
						auto c8 = doc.createElement( "xs:maxInclusive" );
						c8.setAttribute( "value", prm->max );
						c7.appendChild( c8 );
					}
					c5.appendChild( c6 );
					c6.appendChild( c7 );
				}
				c4.appendChild( c5 );
				break;
			}
			case SensorDesc::Settings::Parameter::Type::Double:
			{
				SensorDesc::Settings::DoubleParameter* prm = static_cast< SensorDesc::Settings::DoubleParameter* >( ( *i2 ).data() );
				auto c5 = doc.createElement( "xs:element" );
				c5.setAttribute( "name", prm->name );
				if( prm->min <= std::numeric_limits< double >::min() * 10.0f && prm->max >= std::numeric_limits< double >::max() / 10.0f )
					c5.setAttribute( "type", "xs:double" );
				else
				{
					auto c6 = doc.createElement( "xs:simpleType" );
					auto c7 = doc.createElement( "xs:restriction" );
					c7.setAttribute( "base", "xs:double" );
					if( prm->min > std::numeric_limits< double >::min() * 10.0f )
					{
						auto c8 = doc.createElement( "xs:minInclusive" );
						c8.setAttribute( "value", prm->min );
						c7.appendChild( c8 );
					}
					if( prm->max < std::numeric_limits< double >::max() / 10.0f )
					{
						auto c8 = doc.createElement( "xs:maxInclusive" );
						c8.setAttribute( "value", prm->max );
						c7.appendChild( c8 );
					}
					c5.appendChild( c6 );
					c6.appendChild( c7 );
				}
				c4.appendChild( c5 );
				break;
			}
			case SensorDesc::Settings::Parameter::Type::Bool:
			{
				SensorDesc::Settings::BoolParameter* prm = static_cast< SensorDesc::Settings::BoolParameter* >( ( *i2 ).data() );
				auto c5 = doc.createElement( "xs:element" );
				c5.setAttribute( "name", prm->name );
				c5.setAttribute( "type", "xs:boolean" );
				c4.appendChild( c5 );
				break;
			}
			case SensorDesc::Settings::Parameter::Type::String:
			{
				SensorDesc::Settings::StringParameter* prm = static_cast< SensorDesc::Settings::StringParameter* >( ( *i2 ).data() );
				auto c5 = doc.createElement( "xs:element" );
				c5.setAttribute( "name", prm->name );
				if( prm->regExp.pattern().isEmpty() )
					c5.setAttribute( "type", "xs:string" );
				else
				{
					auto c6 = doc.createElement( "xs:simpleType" );
					auto c7 = doc.createElement( "xs:restriction" );
					if( QRegExp( ENUM_PATTERN ).exactMatch( prm->regExp.pattern() ) )
					{
						auto list = prm->regExp.pattern().split( '|' );
						for( auto i : list )
						{
							auto c8 = doc.createElement( "xs:enumeration" );
							c8.setAttribute( "value", i );
							c7.appendChild( c8 );
						}
					}
					else
					{
						auto c8 = doc.createElement( "xs:pattern" );
						c8.setAttribute( "value", prm->regExp.pattern() );
						c7.appendChild( c8 );
					}
					c7.setAttribute( "base", "xs:string" );
					c5.appendChild( c6 );
					c6.appendChild( c7 );
				}
				c4.appendChild( c5 );
				break;
			}
			case SensorDesc::Settings::Parameter::Type::Enum:
			{
				SensorDesc::Settings::EnumParameter* prm = static_cast< SensorDesc::Settings::EnumParameter* >( ( *i2 ).data() );
				auto c5 = doc.createElement( "xs:element" );
				c5.setAttribute( "name", prm->name );
				auto c6 = doc.createElement( "xs:simpleType" );
				auto c7 = doc.createElement( "xs:restriction" );
				c7.setAttribute( "base", "xs:int" );
				for( auto i = prm->map.begin(); i != prm->map.end(); ++i )
				{
					auto c8 = doc.createElement( "xs:enumeration" );
					c8.setAttribute( "value", i.value() );
					c7.appendChild( c8 );
				}
				c5.appendChild( c6 );
				c6.appendChild( c7 );
				c4.appendChild( c5 );
				break;
			}
			default:
				break;
			}
		}
		//root.appendChild( doc.createComment( "===============================================" ) );
	}

	return QByteArray( "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" ) + doc.toByteArray();
}

QTreeWidgetItem* MarvieController::insertSensorSettings( int position, QString sensorName, QMap< QString, QString > sensorSettingsValues )
{
	assert( sensorDescMap.contains( sensorName ) );
	auto& desc = sensorDescMap[sensorName].settings.prmList;
	QTreeWidgetItem* topItem = new QTreeWidgetItem;
	topItem->setSizeHint( 1, QSize( 1, 32 ) );
	topItem->setText( 0, QString( "%1. %2" ).arg( position + 1 ).arg( sensorName ) );
	ui.sensorSettingsTreeWidget->insertTopLevelItem( position, topItem );

	auto addContent = [this, topItem, position]( QString name, QWidget* content )
	{
		QTreeWidgetItem* item = new QTreeWidgetItem( topItem );
		item->setText( 0, name );

		QWidget* widget = new QWidget;
		QHBoxLayout* layout = new QHBoxLayout( widget );
		layout->setContentsMargins( QMargins( 4, 4, 4, 4 ) );
		layout->addWidget( content );
		content->setObjectName( QString( "%1*" ).arg( position + 1 ) + name );
		ui.sensorSettingsTreeWidget->setItemWidget( item, 1, widget );
	};

	if( sensorDescMap[sensorName].settings.target == SensorDesc::Settings::Target::BR )
	{
		QComboBox* vPortIdComboBox = new QComboBox;
		vPortIdComboBox->addItems( vPortFullNames() );
		if( sensorSettingsValues.contains( "vPortID" ) )
			vPortIdComboBox->setCurrentIndex( sensorSettingsValues["vPortID"].toInt() );
		else
			vPortIdComboBox->setCurrentIndex( -1 );
		vPortIdComboBox->installEventFilter( &vPortIdComboBoxEventFilter );
		addContent( "vPortID", vPortIdComboBox );

		QComboBox* baudrateComboBox = new QComboBox;
		baudrateComboBox->addItems( QStringList() << "default"
									<< "110" << "150" << "300" << "1200" << "2400" << "4800"
									<< "9600" << "19200" << "38400" << "57600" << "115200" );
		if( sensorSettingsValues.contains( "baudrate" ) )
			baudrateComboBox->setCurrentText( sensorSettingsValues["baudrate"] );
		else
			baudrateComboBox->setCurrentIndex( 0 );
		addContent( "baudrate", baudrateComboBox );

		QSpinBox* spinBox = new QSpinBox;
		spinBox->setMinimum( 0 );
		spinBox->setMaximum( 86400 );
		if( sensorSettingsValues.contains( "normalPeriod" ) )
			spinBox->setValue( sensorSettingsValues["normalPeriod"].toInt() );
		addContent( "normalPeriod", spinBox );

		spinBox = new QSpinBox;
		spinBox->setMinimum( 0 );
		spinBox->setMaximum( 86400 );
		if( sensorSettingsValues.contains( "emergencyPeriod" ) )
			spinBox->setValue( sensorSettingsValues["emergencyPeriod"].toInt() );
		addContent( "emergencyPeriod", spinBox );
	}

	for( const auto& i : desc )
	{
		switch( i->type )
		{
		case SensorDesc::Settings::Parameter::Type::Int:
		{
			SensorDesc::Settings::IntParameter* prm = static_cast< SensorDesc::Settings::IntParameter* >( i.data() );
			QSpinBox* spinBox = new QSpinBox;
			spinBox->setMaximum( prm->max );
			spinBox->setMinimum( prm->min );
			if( sensorSettingsValues.contains( i->name ) )
				spinBox->setValue( sensorSettingsValues[i->name].toInt() );
			else
				spinBox->setValue( prm->defaultValue );
			addContent( i->name, spinBox );
			break;
		}
		case SensorDesc::Settings::Parameter::Type::Float:
		{
			SensorDesc::Settings::FloatParameter* prm = static_cast< SensorDesc::Settings::FloatParameter* >( i.data() );
			QDoubleSpinBox* spinBox = new QDoubleSpinBox;
			spinBox->setMaximum( prm->max );
			spinBox->setMinimum( prm->min );
			spinBox->setDecimals( 7 );
			if( sensorSettingsValues.contains( i->name ) )
				spinBox->setValue( ( float )sensorSettingsValues[i->name].toDouble() );
			else
				spinBox->setValue( prm->defaultValue );
			addContent( i->name, spinBox );
			break;
		}
		case SensorDesc::Settings::Parameter::Type::Double:
		{
			SensorDesc::Settings::DoubleParameter* prm = static_cast< SensorDesc::Settings::DoubleParameter* >( i.data() );
			QDoubleSpinBox* spinBox = new QDoubleSpinBox;
			spinBox->setMaximum( prm->max );
			spinBox->setMinimum( prm->min );
			spinBox->setDecimals( 14 );
			if( sensorSettingsValues.contains( i->name ) )
				spinBox->setValue( sensorSettingsValues[i->name].toDouble() );
			else
				spinBox->setValue( prm->defaultValue );
			addContent( i->name, spinBox );
			break;
		}
		case SensorDesc::Settings::Parameter::Type::Bool:
		{
			SensorDesc::Settings::BoolParameter* prm = static_cast< SensorDesc::Settings::BoolParameter* >( i.data() );
			QCheckBox* checkBox = new QCheckBox;
			checkBox->setTristate( false );
			if( sensorSettingsValues.contains( i->name ) )
				checkBox->setCheckState( ( sensorSettingsValues[i->name] == "true" || sensorSettingsValues[i->name].toInt() ) ? Qt::Checked : Qt::Unchecked );
			else
				checkBox->setCheckState( prm->defaultValue ? Qt::Checked : Qt::Unchecked );
			addContent( i->name, checkBox );
			break;
		}
		case SensorDesc::Settings::Parameter::Type::String:
		{
			SensorDesc::Settings::StringParameter* prm = static_cast< SensorDesc::Settings::StringParameter* >( i.data() );
			QRegExp reg( ENUM_PATTERN );
			if( reg.exactMatch( prm->regExp.pattern() ) )
			{
				auto list = prm->regExp.pattern().split( '|' );
				QComboBox* comboBox = new QComboBox;
				comboBox->addItems( list );
				if( sensorSettingsValues.contains( i->name ) )
					comboBox->setCurrentText( sensorSettingsValues[i->name] );
				else
					comboBox->setCurrentText( prm->defaultValue );
				addContent( i->name, comboBox );
			}
			else
			{
				QLineEdit* lineEdit = new QLineEdit;
				lineEdit->setValidator( new QRegExpValidator( prm->regExp, lineEdit ) );
				lineEdit->setProperty( "defaultValue", prm->defaultValue );
				class Filter : public QObject
				{
					bool eventFilter( QObject *obj, QEvent *event )
					{
						QLineEdit* edit = static_cast< QLineEdit* > ( obj );
						if( event->type() == QEvent::FocusOut )
						{
							if( edit->hasAcceptableInput() )
								obj->setProperty( "defaultValue", edit->text() );
							else
								edit->setText( obj->property( "defaultValue" ).toString() );
						}
						return false;
					}
				};
				static QObject* filter = new Filter;
				if( sensorSettingsValues.contains( i->name ) )
					lineEdit->setText( sensorSettingsValues[i->name] );
				else
					lineEdit->setText( prm->defaultValue );
				lineEdit->installEventFilter( filter );
				addContent( i->name, lineEdit );
			}
			break;
		}
		case SensorDesc::Settings::Parameter::Type::Enum:
		{
			SensorDesc::Settings::EnumParameter* prm = static_cast< SensorDesc::Settings::EnumParameter* >( i.data() );
			QComboBox* comboBox = new QComboBox;
			comboBox->addItems( prm->map.keys() );
			if( sensorSettingsValues.contains( i->name ) )
				comboBox->setCurrentText( prm->map.keys()[prm->map.values().indexOf( sensorSettingsValues[i->name].toInt(), 0 )] );
			else
				comboBox->setCurrentText( prm->defaultValue );
			addContent( i->name, comboBox );
			break;
		}
		default:
			assert( false );
			break;
		}
	}

	return topItem;
}

void MarvieController::removeSensorSettings( int position, bool needUpdateName )
{
	ui.sensorSettingsTreeWidget->model()->removeRow( position );
	if( !needUpdateName )
		return;
	while( position < ui.sensorSettingsTreeWidget->model()->rowCount() )
		setSensorSettingsNameNum( position++, position + 1 );
}

void MarvieController::setSensorSettingsNameNum( int index, int num )
{
	auto item = ui.sensorSettingsTreeWidget->topLevelItem( index );
	auto mlist = item->text( 0 ).split( ". " );
	QString sensorName = mlist[1];
	item->setText( 0, QString( "%1. %2" ).arg( num ).arg( sensorName ) );
	auto list = ui.sensorSettingsTreeWidget->findChildren< QObject* >( QRegExp( mlist[0] + "\\*" ) );
	for( auto i : list )
		i->setObjectName( QString( "%1*" ).arg( num ) + i->objectName().split( '*' )[1] );
}

QMap< QString, QString > MarvieController::sensorSettingsValues( int position )
{
#define ENAME( prmName ) ( QString ( "%1*" ).arg( position + 1 ) + prmName  )
	QMap< QString, QString > map;
	QTreeWidgetItem* item = ui.sensorSettingsTreeWidget->topLevelItem( position );
	QString sensorName = item->text( 0 ).split( ". " )[1];
	auto& desc = sensorDescMap[sensorName].settings.prmList;
	if( sensorDescMap[sensorName].settings.target == SensorDesc::Settings::Target::BR )
	{
		QComboBox* comboBox = ui.sensorSettingsTreeWidget->findChild< QComboBox* >( ENAME( "vPortID" ) );
		if( comboBox->currentIndex() != -1 && vPorts[comboBox->currentIndex()] == comboBox->currentText().split( " : " )[1] )
			map["vPortID"] = QString( "%1" ).arg( comboBox->currentIndex() );
		comboBox = ui.sensorSettingsTreeWidget->findChild< QComboBox* >( ENAME( "baudrate" ) );
		if( comboBox->currentIndex() != 0 )
			map["baudrate"] = comboBox->currentText();
		map["normalPeriod"] = QString( "%1" ).arg( ui.sensorSettingsTreeWidget->findChild< QSpinBox* >( ENAME( "normalPeriod" ) )->value() );
		map["emergencyPeriod"] = QString( "%1" ).arg( ui.sensorSettingsTreeWidget->findChild< QSpinBox* >( ENAME( "emergencyPeriod" ) )->value() );
	}

	for( const auto& i : desc )
	{
		switch( i->type )
		{
		case SensorDesc::Settings::Parameter::Type::Int:
			map[i->name] = QString( "%1" ).arg( ui.sensorSettingsTreeWidget->findChild< QSpinBox* >( ENAME( i->name ) )->value() );
			break;
		case SensorDesc::Settings::Parameter::Type::Float:
		case SensorDesc::Settings::Parameter::Type::Double:
			map[i->name] = QString( "%1" ).arg( ui.sensorSettingsTreeWidget->findChild< QDoubleSpinBox* >( ENAME( i->name ) )->value() );
			break;
		case SensorDesc::Settings::Parameter::Type::Bool:
			map[i->name] = QString( "%1" ).arg( ui.sensorSettingsTreeWidget->findChild< QCheckBox* >( ENAME( i->name ) )->checkState() == Qt::CheckState::Checked ? 1 : 0 );
			break;
		case SensorDesc::Settings::Parameter::Type::String:
		{
			auto editor = ui.sensorSettingsTreeWidget->findChild< QComboBox* >( ENAME( i->name ) );
			if( editor )
				map[i->name] = QString( "%1" ).arg( editor->currentText() );
			else
				map[i->name] = QString( "%1" ).arg( ui.sensorSettingsTreeWidget->findChild< QLineEdit* >( ENAME( i->name ) )->text() );
			break;
		}
		case SensorDesc::Settings::Parameter::Type::Enum:
		{
			SensorDesc::Settings::EnumParameter* prm = static_cast< SensorDesc::Settings::EnumParameter* >( i.data() );
			map[i->name] = QString( "%1" ).arg( prm->map[ui.sensorSettingsTreeWidget->findChild< QComboBox* >( ENAME( i->name ) )->currentText()] );
			break;
		}
		default:
			assert( false );
			break;
		}
	}

	return map;
}

QStringList MarvieController::vPortFullNames()
{
	QStringList names;
	for( int i = 0; i < vPorts.size(); ++i )
		names.append( QString( "%1 : %2" ).arg( i ).arg( vPorts[i] ) );

	return names;
}

bool MarvieController::loadConfigFromXml( QByteArray xmlData )
{
	xmlData = QString( xmlData ).remove( QRegExp( "(xmlns:xsi|xsi:schemaLocation)=\"[^\"]+\"" ) ).toLocal8Bit();
	xmlMessageHandler.description.clear();
	QDomDocument doc;
	if( !doc.setContent( xmlData, &xmlMessageHandler.description ) )
	{
		// FIX?
		return false;
	}	
	enum class Target { QX, VX } target;
	QDomElement configRoot;
	if( !( configRoot = doc.firstChildElement( "qxConfig" ) ).isNull() )
		target = Target::QX;
	else if( !( configRoot = doc.firstChildElement( "vxConfig" ) ).isNull() )
		target = Target::VX;
	else
	{
		xmlMessageHandler.description = "Configuration not recognized.";
		return false;
	}
	auto c0 = configRoot.firstChildElement( "sensorsConfig" );
	QSet< QString > missingSensors;
	if( !c0.isNull() )
	{
		auto c1 = c0.firstChildElement();
		while( !c1.isNull() )
		{
			auto cs = c1;
			c1 = c1.nextSiblingElement();

			if( !sensorDescMap.contains( cs.tagName() ) )
			{
				missingSensors.insert( cs.tagName() );
				c0.removeChild( cs );
			}
		}
	}

	if( missingSensors.count() )
	{
		QString str( "The description of the following sensors is missing:" );
		for( const auto& i : missingSensors )
		{
			str.append( "\n\t" );
			str.append( i );
			str.append( ';' );
		}
		str[str.size() - 1] = '.';
		xmlMessageHandler.description = str;
		xmlData = doc.toString().toLocal8Bit();
	}
	if( !configValidator.validate( xmlData, QUrl::fromLocalFile( "GG42" ) ) )
	{
		// FIX?
		return false;
	}
	if( !doc.setContent( xmlData ) )
	{
		xmlMessageHandler.description = "Unknown error";
		return false;
	}

	ui.targetDeviceComboBox->blockSignals( true );
	if( target == Target::QX )
		ui.targetDeviceComboBox->setCurrentText( "QX" );
	else
		ui.targetDeviceComboBox->setCurrentText( "VX" );
	ui.targetDeviceComboBox->blockSignals( false );
	newConfigButtonClicked();

	auto c1 = configRoot.firstChildElement( "comPortsConfig" );
	auto c2 = c1.firstChildElement();
	for( int i = 0; i < ui.comPortsConfigWidget->comPortsCount(); ++i )
	{
		auto c3 = c2.firstChildElement();		
		if( c3.tagName() == "vPort" )
		{
			ui.comPortsConfigWidget->setAssignment( i, ComPortsConfigWidget::Assignment::VPort );
			QVector< QVariant > prms;
			prms.append( c3.attribute( "frameFormat", "B8N" ) );
			prms.append( c3.attribute( "baudrate", "9600" ).toInt() );
			ui.comPortsConfigWidget->setRelatedParameters( i, prms );
		}
		else if( c3.tagName() == "modbusRtuSlave" )
		{
			ui.comPortsConfigWidget->setAssignment( i, ComPortsConfigWidget::Assignment::ModbusRtuSlave );
			QVector< QVariant > prms;
			prms.append( c3.attribute( "frameFormat", "B8N" ) );
			prms.append( c3.attribute( "baudrate", "9600" ).toInt() );
			prms.append( c3.attribute( "address", "0" ).toInt() );
			ui.comPortsConfigWidget->setRelatedParameters( i, prms );
		}
		else if( c3.tagName() == "gsmModem" )
		{
			ui.comPortsConfigWidget->setAssignment( i, ComPortsConfigWidget::Assignment::GsmModem );
			QVector< QVariant > prms;
			prms.append( c3.attribute( "pinCode", "" ) );
			prms.append( c3.attribute( "vpn", "9600" ) );
			ui.comPortsConfigWidget->setRelatedParameters( i, prms );
			auto c4 = c3.firstChildElement( "modbusRtuServer" );
			if( !c4.isNull() )
			{
				ui.gsmModbusRtuCheckBox->setCheckState( Qt::Checked );
				ui.gsmModbusRtuSpinBox->setValue( c4.attribute( "port" ).toInt() );
			}
			c4 = c3.firstChildElement( "modbusTcpServer" );
			if( !c4.isNull() )
			{
				ui.gsmModbusTcpCheckBox->setCheckState( Qt::Checked );
				ui.gsmModbusTcpSpinBox->setValue( c4.attribute( "port" ).toInt() );
			}
			c4 = c3.firstChildElement( "modbusAsciiServer" );
			if( !c4.isNull() )
			{
				ui.gsmModbusAsciiCheckBox->setCheckState( Qt::Checked );
				ui.gsmModbusAsciiSpinBox->setValue( c4.attribute( "port" ).toInt() );
			}
			c4 = c3.firstChildElement( "vPortOverIp" );
			while( !c4.isNull() )
			{
				int row = vPortsOverGsmModel.rowCount();
				vPortsOverGsmModel.insertRow( row );
				vPortsOverGsmModel.setData( vPortsOverGsmModel.index( row, 0 ), c4.attribute( "ip" ) );
				vPortsOverGsmModel.setData( vPortsOverGsmModel.index( row, 1 ), c4.attribute( "port" ).toInt() );

				c4 = c4.nextSiblingElement();
			}
		}
		else if( c3.tagName() == "multiplexer" )
		{
			ui.comPortsConfigWidget->setAssignment( i, ComPortsConfigWidget::Assignment::Multiplexer );
			QVector< QVariant > prms;
			auto c4 = c3.firstChildElement();
			while( !c4.isNull() )
			{
				prms.append( c4.attribute( "frameFormat", "B8N" ) );
				prms.append( c4.attribute( "baudrate", "9600" ).toInt() );

				c4 = c4.nextSiblingElement();
			}
			ui.comPortsConfigWidget->setRelatedParameters( i, prms );
		}

		c2 = c2.nextSiblingElement();
	}

	c1 = configRoot.firstChildElement( "ethernetConfig" );
	c2 = c1.firstChildElement( "modbusRtuServer" );
	if( !c2.isNull() )
	{
		ui.ethernetModbusRtuCheckBox->setCheckState( Qt::Checked );
		ui.ethernetModbusRtuSpinBox->setValue( c2.attribute( "port" ).toInt() );
	}
	c2 = c1.firstChildElement( "modbusTcpServer" );
	if( !c2.isNull() )
	{
		ui.ethernetModbusTcpCheckBox->setCheckState( Qt::Checked );
		ui.ethernetModbusTcpSpinBox->setValue( c2.attribute( "port" ).toInt() );
	}
	c2 = c1.firstChildElement( "modbusAsciiServer" );
	if( !c2.isNull() )
	{
		ui.ethernetModbusAsciiCheckBox->setCheckState( Qt::Checked );
		ui.ethernetModbusAsciiSpinBox->setValue( c2.attribute( "port" ).toInt() );
	}
	c2 = c1.firstChildElement( "vPortOverIp" );
	while( !c2.isNull() )
	{
		int row = vPortsOverEthernetModel.rowCount();
		vPortsOverEthernetModel.insertRow( row );
		vPortsOverEthernetModel.setData( vPortsOverEthernetModel.index( row, 0 ), c2.attribute( "ip" ) );
		vPortsOverEthernetModel.setData( vPortsOverEthernetModel.index( row, 1 ), c2.attribute( "port" ).toInt() );

		c2 = c2.nextSiblingElement();
	}

	c1 = configRoot.firstChildElement( "sensorsConfig" );
	if( c1.isNull() )
		return true;
	c2 = c1.firstChildElement();
	while( !c2.isNull() )
	{
		QMap< QString, QString > map;
		auto c3 = c2.firstChildElement();
		while( !c3.isNull() )
		{
			map[c3.tagName()] = c3.text();
			c3 = c3.nextSiblingElement();
		}
		insertSensorSettings( ui.sensorSettingsTreeWidget->topLevelItemCount(), c2.tagName(), map );

		c2 = c2.nextSiblingElement();
	}

	return true;
}

QByteArray MarvieController::saveConfigToXml()
{
	xmlMessageHandler.description.clear();
	QStringList vPortNames = vPortFullNames();
	auto vPortIdComboBoxObjectsList = ui.sensorSettingsTreeWidget->findChildren< QComboBox* >( QRegExp( "\\d+\\*vPortID" ) );
	for( auto i : vPortIdComboBoxObjectsList )
	{
		if( !vPortNames.contains( i->currentText() ) )
		{
			i->setCurrentIndex( -1 );
			QString name = ui.sensorSettingsTreeWidget->topLevelItem( i->objectName().split( '*' )[0].toInt() - 1 )->text( 0 );
			xmlMessageHandler.description.append( QString( "vPortID of \"%1\" is not specified\n" ).arg( name ) );
		}
	}
	if( !xmlMessageHandler.description.isEmpty() )
		return QByteArray();

	QDomDocument doc;
	QDomElement root;
	if( ui.targetDeviceComboBox->currentText() == "QX" )
		root = doc.createElement( "qxConfig" );
	else
		root = doc.createElement( "vxConfig" );
	doc.appendChild( root );
	root.setAttribute( "xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance" );
	root.setAttribute( "xsi:schemaLocation", QString( "file:// " ) + QDir::currentPath() + "/Xml/Schemas/Config.xsd" );
	auto c0 = doc.createElement( "comPortsConfig" );
	root.appendChild( c0 );
	auto comAssignments = ui.comPortsConfigWidget->assignments();
	int comCounter = 0;
	for( auto i : comAssignments )
	{
		auto c1 = doc.createElement( QString( "com%1" ).arg( comCounter ) );
		c0.appendChild( c1 );
		switch( i )
		{
		case ComPortsConfigWidget::Assignment::VPort:
		{
			auto c2 = doc.createElement( "vPort" );
			c1.appendChild( c2 );
			auto prms = ui.comPortsConfigWidget->relatedParameters( ( unsigned int )comCounter );
			c2.setAttribute( "frameFormat", prms[0].toString() );
			c2.setAttribute( "baudrate", prms[1].toInt() );
			break;
		}
		case ComPortsConfigWidget::Assignment::ModbusRtuSlave:
		{
			auto c2 = doc.createElement( "modbusRtuSlave" );
			c1.appendChild( c2 );
			auto prms = ui.comPortsConfigWidget->relatedParameters( ( unsigned int )comCounter );
			c2.setAttribute( "frameFormat", prms[0].toString() );
			c2.setAttribute( "baudrate", prms[1].toInt() );
			c2.setAttribute( "address", prms[2].toInt() );
			break;
		}
		case ComPortsConfigWidget::Assignment::GsmModem:
		{
			auto c2 = doc.createElement( "gsmModem" );
			c1.appendChild( c2 );
			auto prms = ui.comPortsConfigWidget->relatedParameters( ( unsigned int )comCounter );
			c2.setAttribute( "vpn", prms[1].toString() );
			if( !prms[0].toString().isEmpty() )
				c2.setAttribute( "pinCode", prms[0].toString() );
			if( ui.gsmModbusRtuCheckBox->checkState() == Qt::Checked )
			{
				auto c3 = doc.createElement( "modbusRtuServer" );
				c2.appendChild( c3 );
				c3.setAttribute( "port", ui.gsmModbusRtuSpinBox->value() );
			}
			if( ui.gsmModbusTcpCheckBox->checkState() == Qt::Checked )
			{
				auto c3 = doc.createElement( "modbusTcpServer" );
				c2.appendChild( c3 );
				c3.setAttribute( "port", ui.gsmModbusTcpSpinBox->value() );
			}
			if( ui.gsmModbusAsciiCheckBox->checkState() == Qt::Checked )
			{
				auto c3 = doc.createElement( "modbusAsciiServer" );
				c2.appendChild( c3 );
				c3.setAttribute( "port", ui.gsmModbusAsciiSpinBox->value() );
			}
			for( int i = 0; i < vPortsOverGsmModel.rowCount(); ++i )
			{
				auto c3 = doc.createElement( "vPortOverIp" );
				c2.appendChild( c3 );
				c3.setAttribute( "port", vPortsOverGsmModel.data( vPortsOverGsmModel.index( i, 1 ), Qt::DisplayRole ).toInt() );
				c3.setAttribute( "ip", vPortsOverGsmModel.data( vPortsOverGsmModel.index( i, 0 ), Qt::DisplayRole ).toString() );
			}
			break;
		}
		case ComPortsConfigWidget::Assignment::Multiplexer:
		{
			auto c2 = doc.createElement( "multiplexer" );
			c1.appendChild( c2 );
			auto prms = ui.comPortsConfigWidget->relatedParameters( ( unsigned int )comCounter );
			for( int i = 0; i < 5; ++i )
			{
				auto c3 = doc.createElement( QString( "com%1" ).arg( i ) );
				c2.appendChild( c3 );
				c3.setAttribute( "frameFormat", prms[i * 2].toString() );
				c3.setAttribute( "baudrate", prms[i * 2 + 1].toInt() );
			}
			break;
		}
		default:
			break;
		}
		++comCounter;
	}

	root.appendChild( doc.createComment( "==================================================================" ) );
	if( ui.targetDeviceComboBox->currentText() == "QX" )
	{
		auto c1 = doc.createElement( "ethernetConfig" );
		root.appendChild( c1 );
		if( ui.ethernetModbusRtuCheckBox->checkState() == Qt::Checked )
		{
			auto c2 = doc.createElement( "modbusRtuServer" );
			c1.appendChild( c2 );
			c2.setAttribute( "port", ui.ethernetModbusRtuSpinBox->value() );
		}
		if( ui.ethernetModbusTcpCheckBox->checkState() == Qt::Checked )
		{
			auto c2 = doc.createElement( "modbusTcpServer" );
			c1.appendChild( c2 );
			c2.setAttribute( "port", ui.ethernetModbusTcpSpinBox->value() );
		}
		if( ui.ethernetModbusAsciiCheckBox->checkState() == Qt::Checked )
		{
			auto c2 = doc.createElement( "modbusAsciiServer" );
			c1.appendChild( c2 );
			c2.setAttribute( "port", ui.ethernetModbusAsciiSpinBox->value() );
		}
		for( int i = 0; i < vPortsOverEthernetModel.rowCount(); ++i )
		{
			auto c2 = doc.createElement( "vPortOverIp" );
			c1.appendChild( c2 );
			c2.setAttribute( "port", vPortsOverEthernetModel.data( vPortsOverEthernetModel.index( i, 1 ), Qt::DisplayRole ).toInt() );
			c2.setAttribute( "ip", vPortsOverEthernetModel.data( vPortsOverEthernetModel.index( i, 0 ), Qt::DisplayRole ).toString() );
		}
		root.appendChild( doc.createComment( "==================================================================" ) );
	}

	if( ui.sensorSettingsTreeWidget->topLevelItemCount() )
	{
		auto c1 = doc.createElement( "sensorsConfig" );
		root.appendChild( c1 );
		for( int i = 0; i < ui.sensorSettingsTreeWidget->topLevelItemCount(); ++i )
		{
			QString sensorName = ui.sensorSettingsTreeWidget->topLevelItem( i )->text( 0 ).split( ". " )[1];
			auto settingsValues = sensorSettingsValues( i );
			auto settings = sensorDescMap[sensorName].settings;
			auto c2 = doc.createElement( sensorName );
			c1.appendChild( c2 );
			if( settings.target == SensorDesc::Settings::Target::BR )
			{
				auto c3 = doc.createElement( "vPortID" );
				c2.appendChild( c3 );
				c3.appendChild( doc.createTextNode( settingsValues["vPortID"] ) );

				if( settingsValues.contains( "baudrate" ) )
				{
					c3 = doc.createElement( "baudrate" );
					c2.appendChild( c3 );
					c3.appendChild( doc.createTextNode( settingsValues["baudrate"] ) );
				}

				c3 = doc.createElement( "normalPeriod" );
				c2.appendChild( c3 );
				c3.appendChild( doc.createTextNode( settingsValues["normalPeriod"] ) );

				c3 = doc.createElement( "emergencyPeriod" );
				c2.appendChild( c3 );
				c3.appendChild( doc.createTextNode( settingsValues["emergencyPeriod"] ) );
			}
			for( const auto& i : settings.prmList )
			{
				if( settingsValues.contains( i->name ) )
				{
					auto c3 = doc.createElement( i->name );
					c2.appendChild( c3 );
					c3.appendChild( doc.createTextNode( settingsValues[i->name] ) );
				}
			}
		}
	}

	return QByteArray( "<?xml version=\"1.0\"?>\n" ) + doc.toByteArray();
}

void MarvieController::updateDeviceLoad( DeviceLoad* deviceLoad )
{
	static_cast< QPieSeries* >( ui.cpuLoadChartView->chart()->series()[0] )->slices()[0]->setValue( deviceLoad->cpuLoad );
	static_cast< QPieSeries* >( ui.cpuLoadChartView->chart()->series()[0] )->slices()[1]->setValue( 100.0 - deviceLoad->cpuLoad );
	QSplineSeries* splineSeries = static_cast< QSplineSeries* >( ui.cpuLoadSeriesChartView->chart()->series()[0] );
	if( splineSeries->count() == DEVICE_LOAD_FREQ * CPU_LOAD_SERIES_WINDOW )
		splineSeries->remove( 0 );
	if( splineSeries->points().size() )
		splineSeries->append( QPointF( splineSeries->points().last().x() + 1.0 / DEVICE_LOAD_FREQ, deviceLoad->cpuLoad ) );
	else
		splineSeries->append( QPointF( 0, deviceLoad->cpuLoad ) );
	QScatterSeries* scatterSeries = static_cast< QScatterSeries* >( ui.cpuLoadSeriesChartView->chart()->series()[1] );
	scatterSeries->clear();
	auto last = splineSeries->points().last();
	scatterSeries->append( last );
	ui.cpuLoadSeriesChartView->chart()->axisX()->setRange( last.x() - CPU_LOAD_SERIES_WINDOW, last.x() );
	ui.cpuLoadLabel->setText( QString( "CPU\n%1%" ).arg( deviceLoad->cpuLoad ) );

	static_cast< QPieSeries* >( ui.memoryLoadChartView->chart()->series()[0] )->slices()[0]->setValue( deviceLoad->allocatedHeapMemory );
	static_cast< QPieSeries* >( ui.memoryLoadChartView->chart()->series()[0] )->slices()[1]->setValue( deviceLoad->allocatedCoreMemory - deviceLoad->allocatedHeapMemory );
	static_cast< QPieSeries* >( ui.memoryLoadChartView->chart()->series()[0] )->slices()[2]->setValue( deviceLoad->totalMemory - deviceLoad->allocatedCoreMemory );
	ui.memoryLoadLabel->setText( QString( "Memory\n%1KB" ).arg( ( deviceLoad->allocatedHeapMemory + 512 ) / 1024 ) );

	if( deviceLoad->sdCapacity != 0 )
	{
		static_cast< QPieSeries* >( ui.sdLoadChartView->chart()->series()[0] )->slices()[0]->setValue( deviceLoad->sdCapacity - deviceLoad->freeSdSpace );
		static_cast< QPieSeries* >( ui.sdLoadChartView->chart()->series()[0] )->slices()[1]->setValue( deviceLoad->freeSdSpace );
		ui.sdLoadLabel->setText( QString( "SD\n%1MB" ).arg( ( deviceLoad->sdCapacity - deviceLoad->freeSdSpace + 1024 * 1024 / 2 ) / 1024 / 1024 ) );
	}
	else
	{
		ui.sdLoadChartView->hide();
		ui.sdLoadLabel->hide();
	}
}

void MarvieController::resetDeviceLoad()
{
	static_cast< QPieSeries* >( ui.cpuLoadChartView->chart()->series()[0] )->slices()[0]->setValue( 0.0 );
	static_cast< QPieSeries* >( ui.cpuLoadChartView->chart()->series()[0] )->slices()[1]->setValue( 100.0 );
	static_cast< QSplineSeries* >( ui.cpuLoadSeriesChartView->chart()->series()[0] )->clear();
	static_cast< QScatterSeries* >( ui.cpuLoadSeriesChartView->chart()->series()[1] )->clear();
	ui.cpuLoadLabel->setText( "CPU\n0%" );

	static_cast< QPieSeries* >( ui.memoryLoadChartView->chart()->series()[0] )->slices()[0]->setValue( 0.0 );
	static_cast< QPieSeries* >( ui.memoryLoadChartView->chart()->series()[0] )->slices()[1]->setValue( 0.0 );
	static_cast< QPieSeries* >( ui.memoryLoadChartView->chart()->series()[0] )->slices()[2]->setValue( 100.0 );
	ui.memoryLoadLabel->setText( "Memory\n0KB" );

	static_cast< QPieSeries* >( ui.sdLoadChartView->chart()->series()[0] )->slices()[0]->setValue( 0.0 );
	static_cast< QPieSeries* >( ui.sdLoadChartView->chart()->series()[0] )->slices()[1]->setValue( 100.0 );
	ui.sdLoadLabel->setText( "SD\n0MB" );

	ui.sdLoadChartView->show();
	ui.sdLoadLabel->show();
}

template< typename T >
QVector< T > getVector( T* array, int size )
{
	QVector< T > v( size );
	for( int i = 0; i < size; ++i )
		v[i] = array[i];

	return v;
}

void MarvieController::updateSensorData( uint id, QString sensorName, uint8_t* data )
{
	QString itemName = QString( "%1. %2" ).arg( id ).arg( sensorName );
	MonitoringDataItem* item = monitoringDataModel.findItem( itemName );
	if( !item )
	{
		item = new MonitoringDataItem( itemName );
		attachSensorRelatedMonitoringDataItems( item, sensorName );
		insertTopLevelMonitoringDataItem( item );
	}
	item->setValue( QDateTime::currentDateTime() );

	if( !sensorDescMap.contains( sensorName ) )
		return;
	auto desc = sensorDescMap[sensorName];
	if( !desc.data.root )
		return;
#define BASE_SIZE 8
	static std::function< void( MonitoringDataItem* item, SensorDesc::Data::Node* node ) > set = [data]( MonitoringDataItem* item, SensorDesc::Data::Node* node )
	{
		int itemChildIndex = 0;
		for( auto i : node->childNodes )
		{
			switch( i->type )
			{
			case SensorDesc::Data::Type::Group:
			{
				set( item->child( itemChildIndex ), i );
				break;
			}
			case SensorDesc::Data::Type::Array:
			{
				if( i->childNodes[0]->type == SensorDesc::Data::Type::Char )
				{
					QString s;
					for( auto i2 : i->childNodes )
					{
						char c = data[i2->bias + BASE_SIZE];
						if( c == 0 )
							break;
						s.append( c );
					}
					item->child( itemChildIndex )->setValue( s );
				}
				switch( i->childNodes[0]->type )
				{
				case SensorDesc::Data::Type::Int8:
					item->child( itemChildIndex )->setValue( getVector( reinterpret_cast< int8_t* >( data + i->bias + BASE_SIZE ), i->childNodes.size() ) );
					break;
				case SensorDesc::Data::Type::Uint8:
					item->child( itemChildIndex )->setValue( getVector( reinterpret_cast< uint8_t* >( data + i->bias + BASE_SIZE ), i->childNodes.size() ) );
					break;
				case SensorDesc::Data::Type::Int16:
					item->child( itemChildIndex )->setValue( getVector( reinterpret_cast< int16_t* >( data + i->bias + BASE_SIZE ), i->childNodes.size() ) );
					break;
				case SensorDesc::Data::Type::Uint16:
					item->child( itemChildIndex )->setValue( getVector( reinterpret_cast< uint16_t* >( data + i->bias + BASE_SIZE ), i->childNodes.size() ) );
					break;
				case SensorDesc::Data::Type::Int32:
					item->child( itemChildIndex )->setValue( getVector( reinterpret_cast< int32_t* >( data + i->bias + BASE_SIZE ), i->childNodes.size() ) );
					break;
				case SensorDesc::Data::Type::Uint32:
					item->child( itemChildIndex )->setValue( getVector( reinterpret_cast< uint32_t* >( data + i->bias + BASE_SIZE ), i->childNodes.size() ) );
					break;
				case SensorDesc::Data::Type::Int64:
					item->child( itemChildIndex )->setValue( getVector( reinterpret_cast< int64_t* >( data + i->bias + BASE_SIZE ), i->childNodes.size() ) );
					break;
				case SensorDesc::Data::Type::Uint64:
					item->child( itemChildIndex )->setValue( getVector( reinterpret_cast< uint64_t* >( data + i->bias + BASE_SIZE ), i->childNodes.size() ) );
					break;
				case SensorDesc::Data::Type::Float:
					item->child( itemChildIndex )->setValue( getVector( reinterpret_cast< float* >( data + i->bias + BASE_SIZE ), i->childNodes.size() ) );
					break;
				case SensorDesc::Data::Type::Double:
					item->child( itemChildIndex )->setValue( getVector( reinterpret_cast< double* >( data + i->bias + BASE_SIZE ), i->childNodes.size() ) );
					break;
				default:
					break;
				}
				set( item->child( itemChildIndex ), i );
				break;
			}
			case SensorDesc::Data::Type::Char:
				item->child( itemChildIndex )->setValue( *reinterpret_cast< char* >( data + i->bias + BASE_SIZE ) );
				break;
			case SensorDesc::Data::Type::Int8:
				item->child( itemChildIndex )->setValue( *reinterpret_cast< int8_t* >( data + i->bias + BASE_SIZE ) );
				break;
			case SensorDesc::Data::Type::Uint8:
				item->child( itemChildIndex )->setValue( *reinterpret_cast< uint8_t* >( data + i->bias + BASE_SIZE ) );
				break;
			case SensorDesc::Data::Type::Int16:
				item->child( itemChildIndex )->setValue( *reinterpret_cast< int16_t* >( data + i->bias + BASE_SIZE ) );
				break;
			case SensorDesc::Data::Type::Uint16:
				item->child( itemChildIndex )->setValue( *reinterpret_cast< uint16_t* >( data + i->bias + BASE_SIZE ) );
				break;
			case SensorDesc::Data::Type::Int32:
				item->child( itemChildIndex )->setValue( *reinterpret_cast< int32_t* >( data + i->bias + BASE_SIZE ) );
				break;
			case SensorDesc::Data::Type::Uint32:
				item->child( itemChildIndex )->setValue( *reinterpret_cast< uint32_t* >( data + i->bias + BASE_SIZE ) );
				break;
			case SensorDesc::Data::Type::Int64:
				item->child( itemChildIndex )->setValue( *reinterpret_cast< int64_t* >( data + i->bias + BASE_SIZE ) );
				break;
			case SensorDesc::Data::Type::Uint64:
				item->child( itemChildIndex )->setValue( *reinterpret_cast< uint64_t* >( data + i->bias + BASE_SIZE ) );
				break;
			case SensorDesc::Data::Type::Float:
				item->child( itemChildIndex )->setValue( *reinterpret_cast< float* >( data + i->bias + BASE_SIZE ) );
				break;
			case SensorDesc::Data::Type::Double:
				item->child( itemChildIndex )->setValue( *reinterpret_cast< double* >( data + i->bias + BASE_SIZE ) );
				break;
			default:
				break;
			}
			++itemChildIndex;
		}
	};
	set( item, desc.data.root.data() );
	monitoringDataModel.topLevelItemDataUpdated( item->childIndex() );
}

void MarvieController::attachSensorRelatedMonitoringDataItems( MonitoringDataItem* sensorItem, QString sensorName )
{
	if( !sensorDescMap.contains( sensorName ) )
		return;
	auto desc = sensorDescMap[sensorName];
	if( !desc.data.root )
		return;

	static std::function< void( MonitoringDataItem* item, SensorDesc::Data::Node* node ) > add = []( MonitoringDataItem* item, SensorDesc::Data::Node* node )
	{
		for( auto i : node->childNodes )
		{
			switch( i->type )
			{
			case SensorDesc::Data::Type::Group:
			{
				MonitoringDataItem* child = new MonitoringDataItem( i->name );
				add( child, i );
				item->appendChild( child );
				break;
			}
			case SensorDesc::Data::Type::Array:
			{
				MonitoringDataItem* child = new MonitoringDataItem( i->name );
				for( int i2 = 0; i2 < i->childNodes.size(); ++i2 )
					child->appendChild( new MonitoringDataItem( QString( "[%1]" ).arg( i2 ) ) );
				item->appendChild( child );
				break;
			}
			default:
			{
				MonitoringDataItem* child = new MonitoringDataItem( i->name );
				item->appendChild( child );
				break;
			}
			}
		}
	};
	add( sensorItem, desc.data.root.data() );
}

void MarvieController::updateAnalogData( uint id, float* data, uint count )
{
	QString itemName = QString( "AI[%1]" ).arg( id );
	MonitoringDataItem* item = monitoringDataModel.findItem( itemName );
	if( !item )
	{
		item = new MonitoringDataItem( itemName );
		attachADRelatedMonitoringDataItems( item, count );
		insertTopLevelMonitoringDataItem( item );
	}

	item->setValue( getVector( data, count ) );
	for( int i = 0; i < item->childCount(); ++i )
		item->child( i )->setValue( data[i] );
	monitoringDataModel.topLevelItemDataUpdated( item->childIndex() );
}

void MarvieController::updateDiscreteData( uint id, uint64_t data, uint count )
{
	QString itemName = QString( "DI[%1]" ).arg( id );
	MonitoringDataItem* item = monitoringDataModel.findItem( itemName );
	if( !item )
	{
		item = new MonitoringDataItem( itemName );
		attachADRelatedMonitoringDataItems( item, count );
		insertTopLevelMonitoringDataItem( item );
	}

	if( count <= 8 )
		item->setValue( ( uint8_t )data );
	else if( count <= 16 )
		item->setValue( ( uint16_t )data );
	else if( count <= 32 )
		item->setValue( ( uint32_t )data );
	else
		item->setValue( ( uint64_t )data );
	for( int i = 0; i < item->childCount(); ++i )
		item->child( i )->setValue( bool( data & ( 1 << i ) ) );
	monitoringDataModel.topLevelItemDataUpdated( item->childIndex() );
}

void MarvieController::attachADRelatedMonitoringDataItems( MonitoringDataItem* item, uint count )
{
	for( int i = 0; i < count; ++i )
		item->appendChild( new MonitoringDataItem( QString( "[%1]" ).arg( i ) ) );
}

void MarvieController::insertTopLevelMonitoringDataItem( MonitoringDataItem* item )
{
	enum ItemType { DI, AI, Sensor };
	auto itemType = []( QString itemName, int& index )
	{
		if( itemName.indexOf( "AI" ) == 0 )
		{
			index = itemName.midRef( 3 ).split( ']' )[0].toInt();
			return ItemType::AI;
		}
		if( itemName.indexOf( "DI" ) == 0 )
		{
			index = itemName.midRef( 3 ).split( ']' )[0].toInt();
			return ItemType::DI;
		}
		index = itemName.split( '.' )[0].toInt();
		return ItemType::Sensor;
	};

	int indexA;
	ItemType typeA = itemType( item->name(), indexA );
	int index = 0;
	while( index < monitoringDataModel.rootItem()->childCount() )
	{
		int indexB;
		ItemType typeB = itemType( monitoringDataModel.rootItem()->child( index )->name(), indexB );
		if( typeA == typeB )
		{
			if( indexA < indexB )
				break;
		}
		else
		{
			if( typeA < typeB )
				break;
		}
		++index;
	}

	monitoringDataModel.insertTopLevelItem( index, item );
}

void MarvieController::XmlMessageHandler::handleMessage( QtMsgType type, const QString &description, const QUrl &identifier, const QSourceLocation &sourceLocation )
{
	Q_UNUSED( type );
	Q_UNUSED( identifier );

	this->description = description;
	this->sourceLocation = sourceLocation;
}

int MarvieController::SensorDesc::Data::typeSize( Type type )
{
	switch( type )
	{
	case Type::Char:
	case Type::Int8:
	case Type::Uint8:
		return 1;
	case Type::Int16:
	case Type::Uint16:
		return 2;
	case Type::Int32:
	case Type::Uint32:
	case Type::Float:
		return 4;
	case Type::Int64:
	case Type::Uint64:
	case Type::Double:
		return 8;
	default:
		assert( true );
		return -1;
	}
}

MarvieController::SensorDesc::Data::Type MarvieController::SensorDesc::Data::toType( QString typeName )
{
	static const QMap< QString, SensorDesc::Data::Type > map = []() -> QMap< QString, SensorDesc::Data::Type >
	{
		QMap< QString, SensorDesc::Data::Type > map;
		map.insert( "char", SensorDesc::Data::Type::Char );
		map.insert( "int8", SensorDesc::Data::Type::Int8 );
		map.insert( "uint8", SensorDesc::Data::Type::Uint8 );
		map.insert( "int16", SensorDesc::Data::Type::Int16 );
		map.insert( "uint16", SensorDesc::Data::Type::Uint16 );
		map.insert( "int32", SensorDesc::Data::Type::Int32 );
		map.insert( "uint32", SensorDesc::Data::Type::Uint32 );
		map.insert( "int64", SensorDesc::Data::Type::Int64 );
		map.insert( "uint64", SensorDesc::Data::Type::Uint64 );
		map.insert( "float", SensorDesc::Data::Type::Float );
		map.insert( "double", SensorDesc::Data::Type::Double );
		map.insert( "unused", SensorDesc::Data::Type::Unused );
		map.insert( "group", SensorDesc::Data::Type::Group );
		map.insert( "array", SensorDesc::Data::Type::Array );
		map.insert( "groupArray", SensorDesc::Data::Type::GroupArray );
		return map;
	}( );
	assert( map.contains( typeName ) );
	return map[typeName];
}

bool MarvieController::VPortIdComboBoxEventFilter::eventFilter( QObject *obj, QEvent *event )
{
	QComboBox* comboBox = static_cast< QComboBox* >( obj );
	if( event->type() == QEvent::Type::MouseButtonPress || event->type() == QEvent::Type::KeyPress )
	{
		auto current = comboBox->currentText();
		QStringList names;
		for( int i = 0; i < vPorts.size(); ++i )
			names.append( QString( "%1 : %2" ).arg( i ).arg( vPorts[i] ) );
		comboBox->clear();
		comboBox->addItems( names );
		if( names.contains( current ) )
			comboBox->setCurrentText( current );
		else
			comboBox->setCurrentIndex( -1 );
	}

	return false;
}
