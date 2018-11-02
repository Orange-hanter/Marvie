#include "MarvieController.h"
#include <QSerialPortInfo>
#include <QSerialPort>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QNetworkDatagram>
#include <QXmlSchema>
#include <QXmlStreamWriter>
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
#include <DataTransferProgressWindow.h>
#include <QDebug>
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

	setMinimumSize( QSize( 544, 680 ) );
	QRect mainWindowRect( 0, 0, 540, 680 );
	mainWindowRect.moveCenter( qApp->desktop()->rect().center() );
	setGeometry( mainWindowRect );

	mlinkIODevice = nullptr;

	accountWindow = new AccountWindow;
	accountWindow->setPalette( centralWindget->palette() );

	sensorsInit();

	sensorNameTimer.setInterval( 300 );
	sensorNameTimer.setSingleShot( true );

	popupSensorsListWidget = new QListWidget;
	popupSensorsListWidget->setWindowFlags( Qt::Popup );
	popupSensorsListWidget->setFocusPolicy( Qt::NoFocus );
	popupSensorsListWidget->setFocusProxy( ui.sensorNameEdit );
	popupSensorsListWidget->setMouseTracking( true );
	popupSensorsListWidget->setEditTriggers( QTreeWidget::NoEditTriggers );
	popupSensorsListWidget->setSelectionBehavior( QTreeWidget::SelectRows );
	popupSensorsListWidget->setFrameStyle( QFrame::StyledPanel );
	popupSensorsListWidget->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	popupSensorsListWidget->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	popupSensorsListWidget->installEventFilter( this );

	ui.sensorSettingsTreeWidget->setColumnWidth( 0, 250 );
	ui.sensorSettingsTreeWidget->header()->setSectionResizeMode( 0, QHeaderView::ResizeMode::Stretch );
	ui.sensorSettingsTreeWidget->header()->setSectionResizeMode( 1, QHeaderView::ResizeMode::Fixed );
	ui.sensorSettingsTreeWidget->header()->resizeSection( 1, 150 );
	ui.mainStackedWidget->setCurrentIndex( 0 );
	ui.settingsStackedWidget->setCurrentIndex( 0 );

	ui.vPortsOverIpTableView->setModel( &vPortsOverEthernetModel );
	ui.vPortsOverIpTableView->setItemDelegate( &vPortsOverIpDelegate );
	ui.vPortsOverIpTableView->horizontalHeader()->setSectionResizeMode( QHeaderView::ResizeMode::Fixed );
	ui.vPortsOverIpTableView->horizontalHeader()->resizeSection( 0, 100 );
	ui.vPortsOverIpTableView->horizontalHeader()->resizeSection( 1, 50 );
	ui.vPortsOverIpTableView->verticalHeader()->setSectionResizeMode( QHeaderView::ResizeMode::Fixed );

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

	memStat = new MemStatistics;
	class MemoryLoadChartEventFilter : public QObject
	{
	public:
		MemStatistics * memStat;
		MemoryLoadChartEventFilter( QObject* parent, MemStatistics* stat ) : QObject( parent ), memStat( stat ) {}
		bool eventFilter( QObject* obj, QEvent *event )
		{
			if( event->type() == QEvent::MouseButtonPress && static_cast< QMouseEvent* >( event )->modifiers() && Qt::KeyboardModifier::ControlModifier )
			{
				QWidget* w = static_cast< QWidget* >( obj );
				memStat->show( w->mapToGlobal( w->geometry().center() + QPoint( 0, w->geometry().height() / 2 ) ) );
			}
			return false;
		}
	};
	ui.memoryLoadLabel->installEventFilter( new MemoryLoadChartEventFilter( ui.memoryLoadLabel, memStat ) );
	sdStat = new SdStatistics;
	class SdLoadChartEventFilter : public QObject
	{
	public:
		SdStatistics * sdStat;
		SdLoadChartEventFilter( QObject* parent, SdStatistics* stat ) : QObject( parent ), sdStat( stat ) {}
		bool eventFilter( QObject* obj, QEvent *event )
		{
			if( event->type() == QEvent::MouseButtonPress && static_cast< QMouseEvent* >( event )->modifiers() && Qt::KeyboardModifier::ControlModifier )
			{
				QWidget* w = static_cast< QWidget* >( obj );
				sdStat->show( w->mapToGlobal( w->geometry().center() + QPoint( 0, w->geometry().height() / 2 ) ) );
			}
			return false;
		}
	};
	ui.sdLoadLabel->installEventFilter( new SdLoadChartEventFilter( ui.sdLoadLabel, sdStat ) );
	resetDeviceLoad();

	sdCardMenu = new QMenu( this );
	sdCardMenu->addAction( QIcon( ":/MarvieController/icons/icons8-eject-48.png" ), "Eject" );
	sdCardMenu->addAction( QIcon( ":/MarvieController/icons/icons8-eraser-60.png" ), "Format" );
	sdCardMenu->setFixedWidth( 100 );

	logMenu = new QMenu( this );
	logMenu->addAction( QIcon( ":/MarvieController/icons/icons8-eraser-60.png" ), "Clean monitoring log" );
	logMenu->addAction( QIcon( ":/MarvieController/icons/icons8-eraser-60.png" ), "Clean system log" );
	logMenu->setFixedWidth( 150 );

	ui.monitoringDataTreeView->setSensorDescriptionMap( &sensorDescMap );
	ui.monitoringLogTreeWidget->setSensorDescriptionMap( &sensorDescMap );

	sensorFieldAddressMapModel.setSensorUnfoldedDescMap( &sensorUnfoldedDescMap );
	ui.modbusRegMapTreeView->setModel( &sensorFieldAddressMapModel );
	ui.modbusRegMapTreeView->header()->resizeSection( 0, 228 );
	ui.modbusRegMapTreeView->header()->resizeSection( 1, 85 );
	ui.modbusRegMapTreeView->header()->resizeSection( 2, 75 );

	monitoringDataViewMenu = new QMenu( this );
	monitoringDataViewMenu->addAction( "Update" );
	monitoringDataViewMenu->addSeparator();
	monitoringDataViewMenu->addAction( "Copy value" );
	monitoringDataViewMenu->addAction( "Copy row" );
	monitoringDataViewMenu->addSeparator();
	monitoringDataViewMenu->addAction( "Expand all" );
	monitoringDataViewMenu->addAction( "Collapse all" );
	monitoringDataViewMenu->addSeparator();
	monitoringDataViewMenu->addAction( "Hexadecimal output" )->setCheckable( true );

	QList< MonitoringLog::SensorDesc > sList;
	for( auto i = sensorDescMap.begin(); i != sensorDescMap.end(); ++i )
	{
		if( !i.value().data.root || i.value().data.size == !- 1 )
			continue;
		sList.append( MonitoringLog::SensorDesc{ i.key(), ( quint32 )i.value().data.size } );
	}
	monitoringLog.setAvailableSensors( sList );

	monitoringLogViewMenu = new QMenu( this );
	monitoringLogViewMenu->addAction( "Copy value" );
	monitoringLogViewMenu->addAction( "Copy row" );
	monitoringLogViewMenu->addSeparator();
	monitoringLogViewMenu->addAction( "Expand all" );
	monitoringLogViewMenu->addAction( "Collapse all" );
	monitoringLogViewMenu->addSeparator();
	monitoringLogViewMenu->addAction( "Hexadecimal output" )->setCheckable( true );

	sensorSettingsMenu = new QMenu( this );
	sensorSettingsMenu->addAction( "Copy" );
	sensorSettingsMenu->addSeparator();
	sensorSettingsMenu->addAction( "Move up" );
	sensorSettingsMenu->addAction( "Move down" );
	sensorSettingsMenu->addSeparator();
	sensorSettingsMenu->addAction( "Remove" );
	sensorSettingsMenu->addSeparator();
	sensorSettingsMenu->addAction( "Expand all" );
	sensorSettingsMenu->addAction( "Collapse all" );
	sensorSettingsMenu->addSeparator();
	sensorSettingsMenu->addAction( "Create an address map" );

	modbusRegMapMenu = new QMenu( this );
	modbusRegMapMenu->addAction( "Expand all" );
	modbusRegMapMenu->addAction( "Collapse all" );
	modbusRegMapMenu->addSeparator()->setText( "Offset units" );
	QActionGroup* offsetUnitsGroup = new QActionGroup( this );
	offsetUnitsGroup->setExclusive( true );
	QAction* byteOffsetAction = offsetUnitsGroup->addAction( "Bytes" );
	QAction* wordOffsetAction = offsetUnitsGroup->addAction( "Words" );
	QAction* dWordOffsetAction = offsetUnitsGroup->addAction( "DWords" );
	byteOffsetAction->setCheckable( true );
	byteOffsetAction->setChecked( true );
	wordOffsetAction->setCheckable( true );
	dWordOffsetAction->setCheckable( true );
	modbusRegMapMenu->addAction( byteOffsetAction );
	modbusRegMapMenu->addAction( wordOffsetAction );
	modbusRegMapMenu->addAction( dWordOffsetAction );
	modbusRegMapMenu->addSeparator();
	modbusRegMapMenu->addAction( "Hexadecimal output" )->setCheckable( true );
	modbusRegMapMenu->addAction( "Relative offset" )->setCheckable( true );

	syncWindow = new SynchronizationWindow( this );
	deviceState = DeviceState::Unknown;

	QRegExpValidator* validator = new QRegExpValidator( ui.ipEdit );
	validator->setRegExp( QRegExp( "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$" ) );
	ui.ipEdit->setValidator( validator );

	QSettings settings( "settings.ini", QSettings::Format::IniFormat );
	ui.ipEdit->setText( settings.value( "remoteIP", "" ).toString() );
	QString interfaceName = settings.value( "interface", "ethernet" ).toString();
	if( interfaceName == "rs232" )
		ui.interfaceStackedWidget->setCurrentIndex( 0 );
	else if( interfaceName == "ethernet" )
		ui.interfaceStackedWidget->setCurrentIndex( 1 );
	else if( interfaceName == "bluetooth" )
		ui.interfaceStackedWidget->setCurrentIndex( 2 );

	validator = new QRegExpValidator( ui.staticIpLineEdit );
	validator->setRegExp( QRegExp( "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$" ) );
	ui.staticIpLineEdit->setValidator( validator );

	validator = new QRegExpValidator( ui.netmaskLineEdit );
	validator->setRegExp( QRegExp( "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$" ) );
	ui.netmaskLineEdit->setValidator( validator );

	validator = new QRegExpValidator( ui.gatewayLineEdit );
	validator->setRegExp( QRegExp( "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$" ) );
	ui.gatewayLineEdit->setValidator( validator );

	resetDeviceInfo();

	QObject::connect( ui.controlButton, &QToolButton::released, this, &MarvieController::mainMenuButtonClicked );
	QObject::connect( ui.monitoringButton, &QToolButton::released, this, &MarvieController::mainMenuButtonClicked );
	QObject::connect( ui.logButton, &QToolButton::released, this, &MarvieController::mainMenuButtonClicked );
	QObject::connect( ui.settingsButton, &QToolButton::released, this, &MarvieController::mainMenuButtonClicked );
	QObject::connect( ui.mainSettingsButton, &QToolButton::released, this, &MarvieController::settingsMenuButtonClicked );
	QObject::connect( ui.sensorsSettingsButton, &QToolButton::released, this, &MarvieController::settingsMenuButtonClicked );

	QObject::connect( ui.nextInterfaceButton, &QToolButton::released, this, &MarvieController::nextInterfaceButtonClicked );
	QObject::connect( ui.rs232ConnectButton, &QToolButton::released, this, &MarvieController::connectButtonClicked );
	QObject::connect( ui.ethernetConnectButton, &QToolButton::released, this, &MarvieController::connectButtonClicked );
	QObject::connect( ui.bluetoothConnectButton, &QToolButton::released, this, &MarvieController::connectButtonClicked );
	QObject::connect( accountWindow, &AccountWindow::logIn, this, &MarvieController::logInButtonClicked );
	QObject::connect( accountWindow, &AccountWindow::logOut, this, &MarvieController::logOutButtonClicked );
	QObject::connect( &mlink, &MLinkClient::stateChanged, this, &MarvieController::mlinkStateChanged );
	QObject::connect( &mlink, static_cast< void( MLinkClient::* )( MLinkClient::Error ) >( &MLinkClient::error ), this, &MarvieController::mlinkError );
	QObject::connect( &mlink, &MLinkClient::newPacketAvailable, this, &MarvieController::mlinkNewPacketAvailable );
	QObject::connect( &mlink, &MLinkClient::newComplexPacketAvailable, this, &MarvieController::mlinkNewComplexPacketAvailable );
	QObject::connect( &mlink, &MLinkClient::complexDataSendingProgress, this, &MarvieController::mlinkComplexDataSendingProgress );
	QObject::connect( &mlink, &MLinkClient::complexDataReceivingProgress, this, &MarvieController::mlinkComplexDataReceivingProgress );

	QObject::connect( ui.deviceRestartButton, &QToolButton::released, this, &MarvieController::deviceRestartButtonClicked );
	QObject::connect( ui.startVPortsButton, &QToolButton::released, this, &MarvieController::startVPortsButtonClicked );
	QObject::connect( ui.stopVPortsButton, &QToolButton::released, this, &MarvieController::stopVPortsButtonClicked );
	QObject::connect( ui.updateAllSensorsButton, &QToolButton::released, this, &MarvieController::updateAllSensorsButtonClicked );
	QObject::connect( ui.updateSensorButton, &QToolButton::released, this, &MarvieController::updateSensorButtonClicked );
	QObject::connect( ui.syncDateTimeButton, &QToolButton::released, this, &MarvieController::syncDateTimeButtonClicked );
	QObject::connect( ui.sdCardMenuButton, &QToolButton::released, this, &MarvieController::sdCardMenuButtonClicked );
	QObject::connect( ui.logMenuButton, &QToolButton::released, this, &MarvieController::logMenuButtonClicked );

	QObject::connect( ui.monitoringLogOpenButton, &QPushButton::released, this, &MarvieController::monitoringLogOpenButtonClicked );
	QObject::connect( ui.monitoringLogDateEdit, &QDateEdit::userDateChanged, this, &MarvieController::monitoringLogDateChanged );
	QObject::connect( ui.monitoringLogTimeEdit, &QTimeEdit::userTimeChanged, this, &MarvieController::monitoringLogTimeChanged );
	QObject::connect( ui.monitoringLogPrevEntryButton, &QToolButton::released, this, &MarvieController::monitoringLogMoveEntryButtonClicked );
	QObject::connect( ui.monitoringLogNextEntryButton, &QToolButton::released, this, &MarvieController::monitoringLogMoveEntryButtonClicked );
	QObject::connect( ui.monitoringLogTimeSlider, &QSlider::valueChanged, this, &MarvieController::monitoringLogSliderChanged );

	QObject::connect( ui.addVPortOverIpButton, &QToolButton::released, this, &MarvieController::addVPortOverIpButtonClicked );
	QObject::connect( ui.removeVPortOverIpButton, &QToolButton::released, this, &MarvieController::removeVPortOverIpButtonClicked );
	QObject::connect( ui.digitalInputsLogModeComboBox, static_cast< void( QComboBox::* )( int ) >( &QComboBox::currentIndexChanged ), [this]( int index )
	{
		if( index == 0 )
			ui.digitalInputsLogPeriodSpinBox->hide(), ui.digitInputsLogLabel->hide();
		else
			ui.digitalInputsLogPeriodSpinBox->show(), ui.digitInputsLogLabel->show();
	} );
	QObject::connect( ui.analogInputsLogModeComboBox, static_cast< void( QComboBox::* )( int ) >( &QComboBox::currentIndexChanged ), [this]( int index )
	{
		if( index == 0 )
			ui.analogInputsLogPeriodSpinBox->hide(), ui.analogInputsLogLabel->hide();
		else
			ui.analogInputsLogPeriodSpinBox->show(), ui.analogInputsLogLabel->show();
	} );

	QObject::connect( ui.comPortsConfigWidget, &ComPortsConfigWidget::assignmentChanged, this, &MarvieController::comPortAssignmentChanged );
	QObject::connect( &vPortsOverEthernetModel, &VPortOverIpModel::dataChanged, this, &MarvieController::updateVPortsList );
	QObject::connect( &vPortsOverEthernetModel, &VPortOverIpModel::rowsInserted, this, &MarvieController::updateVPortsList );
	QObject::connect( &vPortsOverEthernetModel, &VPortOverIpModel::rowsRemoved, this, &MarvieController::updateVPortsList );

	QObject::connect( ui.sensorNameEdit, &QLineEdit::textChanged, &sensorNameTimer, static_cast< void( QTimer::* )( ) >( &QTimer::start ) );
	QObject::connect( ui.sensorNameEdit, &QLineEdit::returnPressed, this, &MarvieController::sensorNameEditReturnPressed );
	QObject::connect( &sensorNameTimer, &QTimer::timeout, this, &MarvieController::sensorNameTimerTimeout );

	QObject::connect( popupSensorsListWidget, &QListWidget::itemClicked, this, &MarvieController::sensorNameSearchCompleted );

	QObject::connect( ui.sensorAddButton, &QToolButton::released, this, &MarvieController::sensorAddButtonClicked );
	QObject::connect( ui.sensorRemoveButton, &QToolButton::released, this, &MarvieController::sensorRemoveButtonClicked );
	QObject::connect( ui.sensorMoveUpButton, &QToolButton::released, this, &MarvieController::sensorMoveUpButtonClicked );
	QObject::connect( ui.sensorMoveDownButton, &QToolButton::released, this, &MarvieController::sensorMoveDownButtonClicked );
	QObject::connect( ui.sensorCopyButton, &QToolButton::released, this, &MarvieController::sensorCopyButtonClicked );
	QObject::connect( ui.sensorsListClearButton, &QToolButton::released, this, &MarvieController::sensorsClearButtonClicked );

	QObject::connect( ui.backToSensorsLisstButton, &QToolButton::released, this, [this]() { ui.settingsStackedWidget->setCurrentWidget( ui.sensorSettingsPage ); } );
	QObject::connect( ui.exportModbusRegMapToCsvButton, &QToolButton::released, this, &MarvieController::exportModbusRegMapToCsvButtonClicked );

	QObject::connect( ui.targetDeviceComboBox, &QComboBox::currentTextChanged, this, &MarvieController::targetDeviceChanged );
	QObject::connect( ui.newConfigButton, &QToolButton::released, this, &MarvieController::newConfigButtonClicked );
	QObject::connect( ui.importConfigButton, &QToolButton::released, this, &MarvieController::importConfigButtonClicked );
	QObject::connect( ui.exportConfigButton, &QToolButton::released, this, &MarvieController::exportConfigButtonClicked );
	QObject::connect( ui.uploadConfigButton, &QToolButton::released, this, &MarvieController::uploadConfigButtonClicked );
	QObject::connect( ui.downloadConfigButton, &QToolButton::released, this, &MarvieController::downloadConfigButtonClicked );

	QObject::connect( sdCardMenu, &QMenu::triggered, this, &MarvieController::sdCardMenuActionTriggered );
	QObject::connect( logMenu, &QMenu::triggered, this, &MarvieController::logMenuActionTriggered );

	QObject::connect( ui.monitoringDataTreeView, &QTreeView::customContextMenuRequested, this, &MarvieController::monitoringDataViewMenuRequested );
	QObject::connect( monitoringDataViewMenu, &QMenu::triggered, this, &MarvieController::monitoringDataViewMenuActionTriggered );

	QObject::connect( ui.monitoringLogTreeWidget, &QTreeView::customContextMenuRequested, this, &MarvieController::monitoringLogViewMenuRequested );
	QObject::connect( monitoringLogViewMenu, &QMenu::triggered, this, &MarvieController::monitoringLogViewMenuActionTriggered );

	QObject::connect( ui.sensorSettingsTreeWidget, &QTreeView::customContextMenuRequested, this, &MarvieController::sensorSettingsMenuRequested );
	QObject::connect( sensorSettingsMenu, &QMenu::triggered, this, &MarvieController::sensorSettingsMenuActionTriggered );

	QObject::connect( ui.modbusRegMapTreeView, &QTreeView::customContextMenuRequested, this, &MarvieController::modbusRegMapMenuRequested );
	QObject::connect( modbusRegMapMenu, &QMenu::triggered, this, &MarvieController::modbusRegMapMenuActionTriggered );

	ui.rs232ComboBox->installEventFilter( this );

	newConfigButtonClicked();

	//////////////////////////////////////////////////////////////////////////
	//struct Data
	//{
	//	int a;
	//	int _reseved;
	//	float b[3];
	//	int _reseved2;
	//	double d;
	//	struct Ch 
	//	{
	//		uint8_t c;
	//		char str[3];
	//		uint16_t m[2];
	//		uint32_t _reseved;
	//	} ch[2];
	//} data;
	//data.a = 1;
	//data.b[0] = 3.14f; data.b[1] = 0.0000042f; data.b[2] = 4200000000000.0f;
	//data.d = 42.12345678900123456789;
	//data.ch[0].c = 2;
	//data.ch[0].str[0] = 'a'; data.ch[0].str[1] = 'b'; data.ch[0].str[2] = 'c';
	//data.ch[0].m[0] = 3; data.ch[0].m[1] = 4;
	//data.ch[1].c = 5;
	//data.ch[1].str[0] = 'd'; data.ch[1].str[1] = 'e'; data.ch[1].str[2] = 'f';
	//data.ch[1].m[0] = 6; data.ch[1].m[1] = 7;
	////for( int i  = 0; i < 32; ++i )
	//ui.monitoringDataTreeView->updateSensorData( "1. SimpleSensor", "SimpleSensor", reinterpret_cast< uint8_t* >( &data ), QDateTime::currentDateTime() );

	//float ai[8];
	//for( int i = 0; i < ARRAYSIZE( ai ); ++i )
	//	ai[i] = 0.1 * i;
	//ui.monitoringDataTreeView->updateAnalogData( 0, ai, ARRAYSIZE( ai ) );
	//uint16_t di = 0x4288;
	//ui.monitoringDataTreeView->updateDiscreteData( 0, di, 16 );

	//ui.monitoringDataTreeView->updateSensorData( "b.SimpleSensor", "SimpleSensor", reinterpret_cast< uint8_t* >( &data ), QDateTime::currentDateTime() );
	//ui.monitoringDataTreeView->updateSensorData( "16. SimpleSensor", "SimpleSensor", reinterpret_cast< uint8_t* >( &data ), QDateTime::currentDateTime() );

	//ui.monitoringDataTreeView->updateAnalogData( 10, ai, ARRAYSIZE( ai ) );
	//ui.monitoringDataTreeView->updateDiscreteData( 13, di, 16 );
	//ui.monitoringDataTreeView->updateAnalogData( 15, ai, ARRAYSIZE( ai ) );
	//ui.monitoringDataTreeView->updateDiscreteData( 15, di, 16 );
	//ui.monitoringDataTreeView->updateAnalogData( 0, ai, ARRAYSIZE( ai ) );
	//ui.monitoringDataTreeView->updateDiscreteData( 5, di, 16 );

	//ui.monitoringDataTreeView->updateSensorData( "10. SimpleSensor", "SimpleSensor", reinterpret_cast< uint8_t* >( &data ), QDateTime::currentDateTime() );
	//ui.monitoringDataTreeView->updateSensorData( "4. SimpleSensor", "SimpleSensor", reinterpret_cast< uint8_t* >( &data ), QDateTime::currentDateTime() );
	//ui.monitoringDataTreeView->updateSensorData( "a.SimpleSensor", "SimpleSensor", reinterpret_cast< uint8_t* >( &data ), QDateTime::currentDateTime() );

	//ui.monitoringDataTreeView->removeAnalogData();
	//ui.monitoringDataTreeView->removeDiscreteData();
	//ui.monitoringDataTreeView->removeSensorData( "a.SimpleSensor" );

	/*ui.vPortTileListWidget->setTilesCount( 8 );
	ui.vPortTileListWidget->tile( 0 )->setNextSensorRead( 0, "CE301", 61 );
	ui.vPortTileListWidget->tile( 0 )->setNextSensorRead( 0, "CE301", 4 );
	ui.vPortTileListWidget->tile( 1 )->setNextSensorRead( 1, "CE301", 0 );
	ui.vPortTileListWidget->tile( 2 )->setNextSensorRead( 2, "CE301", 5 );
	ui.vPortTileListWidget->tile( 2 )->resetNextSensorRead();
	ui.vPortTileListWidget->tile( 1 )->addSensorReadError( 0, "CE301", VPortTileWidget::SensorError::NoResponseError, 255, QDateTime::currentDateTime() );*/

	//ui.vPortTileListWidget->removeAllTiles();
	//ui.vPortTileListWidget->setTilesCount( 5 );
	//ui.vPortTileListWidget->tile( 0 )->setNextSensorRead( 0, "CE301", 61 );
	//ui.vPortTileListWidget->tile( 0 )->setNextSensorRead( 0, "CE301", 4 );
	//ui.vPortTileListWidget->tile( 1 )->setNextSensorRead( 1, "CE301", 0 );
	//ui.vPortTileListWidget->tile( 2 )->setNextSensorRead( 2, "CE301", 5 );
	//ui.vPortTileListWidget->tile( 2 )->resetNextSensorRead();


	/*QTimer* timer = new QTimer;
	timer->setInterval( 1000 / DEVICE_LOAD_FREQ );
	timer->setSingleShot( false );
	QObject::connect( timer, &QTimer::timeout, [this]() 
	{
		DeviceMemoryLoad load;
		load.cpuLoad = qrand() % 100;
		load.totalMemory = 128 * 1024;
		load.allocatedCoreMemory = 64 * 1024;
		load.allocatedHeapMemory = 32 * 1024;
		load.sdCapacity = 4 * 1024 * 1024 * 1024ULL;
		load.freeSdSpace = 1 * 1024 * 1024 * 1024ULL;
		updateDeviceMemoryLoad( &load );
	} );
	timer->start();*/

	//syncWindow->show();
}

MarvieController::~MarvieController()
{
	accountWindow->deleteLater();
	popupSensorsListWidget->deleteLater();

	QSettings settings( "settings.ini", QSettings::Format::IniFormat );
	if( ui.ipEdit->hasAcceptableInput() )
		settings.setValue( "remoteIP", ui.ipEdit->text() );
	QString interfaceName;
	switch( ui.interfaceStackedWidget->currentIndex() )
	{
	case 0:
		interfaceName = "rs232";
		break;
	case 1:
		interfaceName = "ethernet";
		break;
	case 2:
		interfaceName = "bluetooth";
		break;
	default:
		break;
	}
	settings.setValue( "interface", interfaceName );
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
	QToolButton* buttons[] = { ui.controlButton, ui.monitoringButton, ui.logButton, ui.settingsButton };
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
			if( ui.settingsStackedWidget->currentIndex() != i )
			{
				if( s == ui.sensorsSettingsButton )
					fixSensorVPortIds( false );
				ui.settingsStackedWidget->setCurrentIndex( i );
			}
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
	if( mlink.state() == MLinkClient::State::Connecting )
	{
		mlink.disconnectFromHost();
		return;
	}
	else if( mlink.state() == MLinkClient::State::Disconnecting )
		return;
	int pointX = mapToGlobal( QPoint( width(), 0 ) ).x() - accountWindow->width() - 1;
	int pointY = ui.mainMenuEndLine->mapToGlobal( QPoint( 0, 0 ) ).y();
	accountWindow->setGeometry( pointX, pointY, accountWindow->width(), accountWindow->height() );
	accountWindow->show();	
}

void MarvieController::logInButtonClicked( QString accountName, QString accountPassword )
{
	mlink.setAuthorizationData( accountName, accountPassword );
	if( ui.interfaceStackedWidget->currentWidget() == ui.rs232Page )
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
	else if( ui.interfaceStackedWidget->currentWidget() == ui.ethernetPage )
	{
		if( !ui.ipEdit->hasAcceptableInput() )
			return;

		/*class Device : public QIODevice
		{
		public:
			QUdpSocket * socket;
			QHostAddress host;
			QByteArray buffer;

			Device( QString ip ) : host( ip )
			{
				socket = new QUdpSocket;
				socket->bind( QHostAddress::AnyIPv4, 16032 );
				QObject::connect( socket, &QUdpSocket::readyRead, [this]
				{
					while( socket->hasPendingDatagrams() )
					{
						QNetworkDatagram datagram = socket->receiveDatagram();
						buffer.append( datagram.data() );
					}
					readyRead();
				} );
				QObject::connect( socket, &QUdpSocket::bytesWritten, [this]( qint64 n ) { bytesWritten( n ); } );
			}
			~Device()
			{
				delete socket;
			}
			qint64 writeData( const char *data, qint64 len ) override
			{
				return socket->writeDatagram( data, len, host, 16021 );
			}
			qint64 readData( char *data, qint64 maxlen ) override
			{
				if( buffer.size() < maxlen )
					maxlen = buffer.size();
				for( qint64 i = 0; i < maxlen; ++i )
					data[i] = buffer.at( i );
				buffer = buffer.right( buffer.size() - maxlen );

				return maxlen;
			}
			qint64 bytesAvailable() const override
			{
				return QIODevice::bytesAvailable() + buffer.size();
			}
			qint64 bytesToWrite() const override
			{
				return socket->bytesToWrite();
			}
			bool isSequential() const override
			{
				return true;
			}
			void close() override
			{
				socket->close();
				QIODevice::close();
			}
		};
		Device* device = new Device( ui.ipEdit->text() );
		device->open( QIODevice::ReadWrite );*/

		QTcpSocket* device = new QTcpSocket;
		device->connectToHost( ui.ipEdit->text(), 16021 );

		mlinkIODevice = device;
		mlink.setIODevice( mlinkIODevice );
		mlink.connectToHost();
		ui.ipEdit->setEnabled( false );
	}
	else if( ui.interfaceStackedWidget->currentWidget() == ui.bluetoothPage )
	{
		return;
	}
	ui.nextInterfaceButton->setEnabled( false );
}

void MarvieController::logOutButtonClicked( QString accountName )
{
	mlink.disconnectFromHost();
	return;
}

void MarvieController::deviceRestartButtonClicked()
{
	/*int ret = QMessageBox::question( nullptr, "Restart device",
									 "Do you want to restart?",
									 QMessageBox::Yes, QMessageBox::Discard );
	if( ret == QMessageBox::Discard )
		return;*/
	mlink.sendPacket( MarviePackets::Type::RestartDeviceType, QByteArray() );
}

void MarvieController::startVPortsButtonClicked()
{
	mlink.sendPacket( MarviePackets::Type::StartVPortsType, QByteArray() );
}

void MarvieController::stopVPortsButtonClicked()
{
	mlink.sendPacket( MarviePackets::Type::StopVPortsType, QByteArray() );
}

void MarvieController::updateAllSensorsButtonClicked()
{
	mlink.sendPacket( MarviePackets::Type::UpdateAllSensorsType, QByteArray() );
}

void MarvieController::updateSensorButtonClicked()
{
	if( ui.deviceSensorsComboBox->currentIndex() != -1 )
	{
		uint16_t id = ( uint16_t )ui.deviceSensorsComboBox->currentIndex();
		mlink.sendPacket( MarviePackets::Type::UpdateOneSensorType, QByteArray( ( const char* )&id, sizeof( id ) ) );
	}
}

void MarvieController::syncDateTimeButtonClicked()
{
	DateTime dateTime = toDeviceDateTime( QDateTime::currentDateTime() );
	mlink.sendPacket( MarviePackets::Type::SetDateTimeType, QByteArray( ( const char* )&dateTime, sizeof( dateTime ) ) );
}

void MarvieController::sdCardMenuButtonClicked()
{
	sdCardMenu->actions()[0]->setEnabled( deviceSdCardStatus != SdCardStatus::NotInserted );
	sdCardMenu->actions()[1]->setEnabled( deviceSdCardStatus == SdCardStatus::BadFileSystem || deviceSdCardStatus == SdCardStatus::Working );
	sdCardMenu->popup( ui.sdCardMenuButton->mapToGlobal( ui.sdCardMenuButton->rect().bottomLeft() ) );
}

void MarvieController::logMenuButtonClicked()
{
	logMenu->actions()[0]->setEnabled( deviceSdCardStatus == SdCardStatus::Working );
	logMenu->actions()[1]->setEnabled( deviceSdCardStatus == SdCardStatus::Working );
	logMenu->popup( ui.logMenuButton->mapToGlobal( ui.logMenuButton->rect().bottomLeft() ) );
}

void MarvieController::sdCardMenuActionTriggered( QAction* action )
{
	if( action->text() == "Eject" )
		mlink.sendPacket( MarviePackets::Type::EjectSdCardType, QByteArray() );
	else if( action->text() == "Format" )
	{
		if( mlink.state() != MLinkClient::State::Connected || ui.sdCardStatusLabel->text() == "SD card: formatting" )
			return;
		int ret = QMessageBox::question( nullptr, "Format SD card",
										 "This operation cannot be interrupted. Do you want to continue?",
										 QMessageBox::Yes, QMessageBox::Discard );
		if( ret == QMessageBox::Discard )
			return;
		mlink.sendPacket( MarviePackets::Type::FormatSdCardType, QByteArray() );
	}
}

void MarvieController::logMenuActionTriggered( QAction* action )
{
	if( action->text() == "Clean monitoring log" )
	{
		int ret = QMessageBox::question( nullptr, "Clean monitoring log",
										 "This operation cannot be interrupted. Do you want to continue?",
										 QMessageBox::Yes, QMessageBox::Discard );
		if( ret == QMessageBox::Discard )
			return;
		mlink.sendPacket( MarviePackets::Type::CleanMonitoringLogType, QByteArray() );
	}
	else if( action->text() == "Clean system log" )
	{
		int ret = QMessageBox::question( nullptr, "Clean system log",
										 "This operation cannot be interrupted. Do you want to continue?",
										 QMessageBox::Yes, QMessageBox::Discard );
		if( ret == QMessageBox::Discard )
			return;
		mlink.sendPacket( MarviePackets::Type::CleanSystemLogType, QByteArray() );
	}
}

void MarvieController::monitoringDataViewMenuRequested( const QPoint& point )
{
	auto updateAction = monitoringDataViewMenu->actions()[0];
	QModelIndex index = ui.monitoringDataTreeView->currentIndex();
	if( index.isValid() )
	{
		index = index.sibling( index.row(), 0 );
		while( index.parent().isValid() )
			index = index.parent();
		auto list = index.data( Qt::DisplayRole ).toString().split( '.' );
		if( list.size() >= 2 )
		{
			bool ok;
			int num = list[0].toInt( &ok );
			if( ok )
			{
				updateAction->setVisible( true );
				updateAction->setProperty( "sensorId", num - 1 );
			}
			else
				updateAction->setVisible( false );
		}
		else
			updateAction->setVisible( false );
	}
	else
		updateAction->setVisible( false );

	monitoringDataViewMenu->popup( ui.monitoringDataTreeView->viewport()->mapToGlobal( point ) );
}

void MarvieController::monitoringDataViewMenuActionTriggered( QAction* action )
{
	if( action->text() == "Update" )
	{
		ui.deviceSensorsComboBox->setCurrentIndex( action->property( "sensorId" ).toInt() );
		updateSensorButtonClicked();
	}
	else if( action->text() == "Copy value" )
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
		ui.monitoringDataTreeView->setHexadecimalOutput( action->isChecked() );
}

void MarvieController::monitoringLogOpenButtonClicked()
{
	QSettings setting( "settings.ini", QSettings::Format::IniFormat );
	QString path = QFileDialog::getExistingDirectory( this, "Open the monitoring log", setting.value( "monitoringLogDir", QDir::currentPath() ).toString() );
	if( path.isEmpty() )
		return;
	setting.setValue( "monitoringLogDir", QDir::current().relativeFilePath( path ) );

	if( monitoringLog.open( path ) )
	{
		setMonitoringLogWidgetGroupEnabled( true );
		ui.monitoringLogDateEdit->blockSignals( true );
		ui.monitoringLogDateEdit->setDateRange( monitoringLog.minimumDate(), monitoringLog.maximumDate() );
		ui.monitoringLogPathEdit->setText( path );
		ui.monitoringLogDateEdit->setDate( monitoringLog.maximumDate() );
		ui.monitoringLogDateEdit->blockSignals( false );
		monitoringLogDateChanged();
	}
	else
	{
		setMonitoringLogWidgetGroupEnabled( false );
		ui.monitoringLogPathEdit->setText( "" );
		ui.monitoringLogTreeWidget->clear();
	}
}

void MarvieController::monitoringLogDateChanged()
{
	ui.monitoringLogTreeWidget->clear();
	QVector< QTime > timestamps;
	ui.monitoringLogTreeWidget->clear();
	auto names = monitoringLog.availableGroupNames();
	for( auto& name : names )
	{
		auto nameGroup = monitoringLog.nameGroup( name );
		auto dayGroup = nameGroup.dayGroup( ui.monitoringLogDateEdit->date() );
		timestamps.append( dayGroup.timestamps() );
	}
	qSort( timestamps );
	monitoringLogTimestamps.clear();
	ui.monitoringLogTimeSlider->blockSignals( true );
	if( !timestamps.isEmpty() )
	{
		monitoringLogTimestamps.reserve( timestamps.size() );
		auto v = timestamps.first();
		monitoringLogTimestamps.append( v );
		for( auto i = timestamps.begin() + 1; i != timestamps.end(); ++i )
		{
			if( *i != v )
			{
				monitoringLogTimestamps.append( *i );
				v = *i;
			}
		}
		ui.monitoringLogTimeSlider->setRange( 0, monitoringLogTimestamps.size() - 1 );
		ui.monitoringLogTimeSlider->setValue( monitoringLogTimestamps.size() - 1 );
		ui.monitoringLogTimeEdit->blockSignals( true );
		ui.monitoringLogTimeEdit->setTime( monitoringLogTimestamps.back() );
		ui.monitoringLogTimeEdit->blockSignals( false );
		monitoringLogTimeChanged();
	}
	else
	{
		ui.monitoringLogTimeSlider->setRange( 0, 0 );
		ui.monitoringLogTimeSlider->setValue( 0 );
		ui.monitoringLogTimeEdit->blockSignals( true );
		ui.monitoringLogTimeEdit->setTime( QTime( 0, 0, 0, 0 ) );
		ui.monitoringLogTimeEdit->blockSignals( false );
	}
	ui.monitoringLogTimeSlider->blockSignals( false );
}

void MarvieController::monitoringLogTimeChanged()
{
	if( monitoringLogTimestamps.isEmpty() )
	{
		ui.monitoringLogTimeEdit->blockSignals( true );
		ui.monitoringLogTimeEdit->setTime( QTime( 0, 0, 0, 0 ) );
		ui.monitoringLogTimeEdit->blockSignals( false );
		return;
	}

	QTime t = ui.monitoringLogTimeEdit->time();
	QVector< QTime >::iterator i = qLowerBound( monitoringLogTimestamps.begin(), monitoringLogTimestamps.end(), t );
	if( i != monitoringLogTimestamps.begin() && qAbs( t.msecsSinceStartOfDay() - ( *i ).msecsSinceStartOfDay() ) > qAbs( t.msecsSinceStartOfDay() - ( *( i - 1 ) ).msecsSinceStartOfDay() ) )
		--i;
	ui.monitoringLogTimeSlider->blockSignals( true );
	ui.monitoringLogTimeSlider->setValue( i - monitoringLogTimestamps.begin() );
	ui.monitoringLogTimeSlider->blockSignals( false );
	auto names = monitoringLog.availableGroupNames();
	for( auto& name : names )
	{
		auto nameGroup = monitoringLog.nameGroup( name );
		auto dayGroup = nameGroup.dayGroup( ui.monitoringLogDateEdit->date() );
		auto entryInfo = dayGroup.nearestEntry( *i );
		if( !entryInfo.isValid() )
			continue;
		switch( entryInfo.entry()->type() )
		{
		case MonitoringLog::Entry::Type::DigitInputsEntry:
		{
			auto entry = entryInfo.entry().staticCast< MonitoringLog::DigitInputsEntry >();
			for( quint32 i = 0; i < 8; ++i )
			{
				if( entry->isBlockPresent( i ) )
					ui.monitoringLogTreeWidget->updateDiscreteData( i, entry->data( i ), 32, entryInfo.dateTime() );
			}
			break;
		}
		case MonitoringLog::Entry::Type::AnalogInputsEntry:
		{
			auto entry = entryInfo.entry().staticCast< MonitoringLog::AnalogInputsEntry >();
			for( quint32 i = 0; i < 8; ++i )
			{
				const float* data = entry->data( i );
				if( data )
					ui.monitoringLogTreeWidget->updateAnalogData( i, data, entry->channelsCount( i ), entryInfo.dateTime() );
			}
			break;
		}
		case MonitoringLog::Entry::Type::SensorEntry:
		{
			auto entry = entryInfo.entry().staticCast< MonitoringLog::SensorEntry >();
			QString sensorName = name.right( name.size() - name.lastIndexOf( '.' ) - 1 );
			ui.monitoringLogTreeWidget->updateSensorData( name, sensorName, ( const uint8_t* )entry->data(), entryInfo.dateTime() );
			break;
		}
		default:
			break;
		}
	}
	MonitoringDataItem* root = ui.monitoringLogTreeWidget->dataModel()->rootItem();
	for( int i = 0; i < root->childCount(); )
	{
		MonitoringDataItem* child = root->child( i );
		if( child->type() == MonitoringDataItem::ValueType::DateTime && child->value().toDateTime().time() > t )
			ui.monitoringLogTreeWidget->dataModel()->removeRows( i, 1 );
		else
			++i;
	}
}

void MarvieController::monitoringLogSliderChanged( int v )
{
	ui.monitoringLogTimeEdit->setTime( monitoringLogTimestamps[v] );
}

void MarvieController::monitoringLogMoveEntryButtonClicked()
{
	if( sender() == ui.monitoringLogPrevEntryButton )
		ui.monitoringLogTimeSlider->setValue( ui.monitoringLogTimeSlider->value() - 1 );
	else
		ui.monitoringLogTimeSlider->setValue( ui.monitoringLogTimeSlider->value() + 1 );
}

void MarvieController::monitoringLogViewMenuRequested( const QPoint& point )
{
	monitoringLogViewMenu->popup( ui.monitoringLogTreeWidget->viewport()->mapToGlobal( point ) );
}

void MarvieController::monitoringLogViewMenuActionTriggered( QAction* action )
{
	if( action->text() == "Copy value" )
	{
		QModelIndex index = ui.monitoringLogTreeWidget->currentIndex();
		if( !index.isValid() )
			return;
		QApplication::clipboard()->setText( index.sibling( index.row(), 1 ).data( Qt::DisplayRole ).toString() );
	}
	else if( action->text() == "Copy row" )
	{
		QModelIndex index = ui.monitoringLogTreeWidget->currentIndex();
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
		ui.monitoringLogTreeWidget->expandAll();
	else if( action->text() == "Collapse all" )
		ui.monitoringLogTreeWidget->collapseAll();
	else if( action->text() == "Hexadecimal output" )
		ui.monitoringLogTreeWidget->setHexadecimalOutput( action->isChecked() );
}

void MarvieController::setMonitoringLogWidgetGroupEnabled( bool enabled )
{
	ui.monitoringLogDateLabel->setEnabled( enabled );
	ui.monitoringLogDateEdit->setEnabled( enabled );
	ui.monitoringLogTimeLabel->setEnabled( enabled );
	ui.monitoringLogTimeEdit->setEnabled( enabled );
	ui.monitoringLogPrevEntryButton->setEnabled( enabled );
	ui.monitoringLogNextEntryButton->setEnabled( enabled );
	ui.monitoringLogTimeSlider->setEnabled( enabled );
	ui.monitoringLogTreeWidget->setEnabled( enabled );
	if( !enabled )
	{
		ui.monitoringLogTimeSlider->setRange( 0, 100 );
		ui.monitoringLogTimeSlider->setValue( 0 );
	}
}

void MarvieController::sensorSettingsMenuRequested( const QPoint& point )
{
	sensorSettingsMenu->popup( ui.sensorSettingsTreeWidget->viewport()->mapToGlobal( point ) );
}

void MarvieController::sensorSettingsMenuActionTriggered( QAction* action )
{
	if( action->text() == "Copy" )
		sensorCopyButtonClicked();
	else if( action->text() == "Move up" )
		sensorMoveUpButtonClicked();
	else if( action->text() == "Move down" )
		sensorMoveDownButtonClicked();
	else if( action->text() == "Remove" )
		sensorRemoveButtonClicked();
	else if( action->text() == "Expand all" )
		ui.sensorSettingsTreeWidget->expandAll();
	else if( action->text() == "Collapse all" )
		ui.sensorSettingsTreeWidget->collapseAll();
	else if( action->text() == "Create an address map" )
	{
		sensorFieldAddressMapModel.resetData();
		quint32 offset = 1200;
		for( int i = 0; i < ui.sensorSettingsTreeWidget->topLevelItemCount(); ++i )
		{
			QString name = ui.sensorSettingsTreeWidget->topLevelItem( i )->data( 0, Qt::DisplayRole ).toString();
			QString typeName = name.split( ". " )[1];
			sensorFieldAddressMapModel.appendSensor( name, typeName, offset );
			offset += sensorDescMap[typeName].data.size + sizeof( DateTime );
		}
		ui.settingsStackedWidget->setCurrentWidget( ui.modbusRegMapPage );
	}
}

void MarvieController::modbusRegMapMenuRequested( const QPoint& point )
{
	modbusRegMapMenu->popup( ui.modbusRegMapTreeView->viewport()->mapToGlobal( point ) );
}

void MarvieController::modbusRegMapMenuActionTriggered( QAction* action )
{
	if( action->text() == "Expand all" )
		ui.modbusRegMapTreeView->expandAll();
	else if( action->text() == "Collapse all" )
		ui.modbusRegMapTreeView->collapseAll();
	if( action->text() == "Bytes" )
		sensorFieldAddressMapModel.setDisplayOffsetUnits( SensorFieldAddressMapModel::OffsetUnits::Bytes );
	if( action->text() == "Words" )
		sensorFieldAddressMapModel.setDisplayOffsetUnits( SensorFieldAddressMapModel::OffsetUnits::Words );
	if( action->text() == "DWords" )
		sensorFieldAddressMapModel.setDisplayOffsetUnits( SensorFieldAddressMapModel::OffsetUnits::DWords );
	else if( action->text() == "Hexadecimal output" )
		sensorFieldAddressMapModel.setHexadecimalOutput( action->isChecked() );
	else if( action->text() == "Relative offset" )
		sensorFieldAddressMapModel.setRelativeOffset( action->isChecked() );
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
		com0.append( ComPortsConfigWidget::Assignment::ModbusAsciiSlave );
		portAssignments.append( com0 );

		QVector< ComPortsConfigWidget::Assignment > com1;
		com1.append( ComPortsConfigWidget::Assignment::VPort );
		com1.append( ComPortsConfigWidget::Assignment::ModbusRtuSlave );
		com1.append( ComPortsConfigWidget::Assignment::ModbusAsciiSlave );
		portAssignments.append( com1 );

		QVector< ComPortsConfigWidget::Assignment > com2;
		com2.append( ComPortsConfigWidget::Assignment::VPort );
		com2.append( ComPortsConfigWidget::Assignment::ModbusRtuSlave );
		com2.append( ComPortsConfigWidget::Assignment::ModbusAsciiSlave );
		com2.append( ComPortsConfigWidget::Assignment::Multiplexer );
		portAssignments.append( com2 );

		return portAssignments;
	}();
	static QVector< QVector< ComPortsConfigWidget::Assignment > > vxPortAssignments = []()
	{
		QVector< QVector< ComPortsConfigWidget::Assignment > > portAssignments;
		QVector< ComPortsConfigWidget::Assignment > com0;
		com0.append( ComPortsConfigWidget::Assignment::GsmModem );
		com0.append( ComPortsConfigWidget::Assignment::ModbusRtuSlave );
		com0.append( ComPortsConfigWidget::Assignment::ModbusAsciiSlave );
		com0.append( ComPortsConfigWidget::Assignment::VPort );
		portAssignments.append( com0 );

		QVector< ComPortsConfigWidget::Assignment > com1to4;
		com1to4.append( ComPortsConfigWidget::Assignment::VPort );
		portAssignments.append( com1to4 );
		portAssignments.append( com1to4 );
		portAssignments.append( com1to4 );
		portAssignments.append( com1to4 );

		QVector< ComPortsConfigWidget::Assignment > com5;
		com5.append( ComPortsConfigWidget::Assignment::VPort );
		com5.append( ComPortsConfigWidget::Assignment::ModbusRtuSlave );
		com5.append( ComPortsConfigWidget::Assignment::ModbusAsciiSlave );
		portAssignments.append( com5 );

		QVector< ComPortsConfigWidget::Assignment > com6;
		com6.append( ComPortsConfigWidget::Assignment::VPort );
		com6.append( ComPortsConfigWidget::Assignment::ModbusRtuSlave );
		com6.append( ComPortsConfigWidget::Assignment::ModbusAsciiSlave );
		portAssignments.append( com6 );

		return portAssignments;
	}();
	static QVector< QVector< ComPortsConfigWidget::Assignment > > emptyPortAssignments = []()
	{
		QVector< QVector< ComPortsConfigWidget::Assignment > > portAssignments;

		return portAssignments;
	}();

	if( text == "QX" )
		ui.comPortsConfigWidget->init( qxPortAssignments );
	else if( text == "VX" )
		ui.comPortsConfigWidget->init( vxPortAssignments );
	else
	{
		ui.comPortsConfigWidget->init( emptyPortAssignments );
		vPortsOverEthernetModel.removeRows( 0, vPortsOverEthernetModel.rowCount() );
	}
}

void MarvieController::newConfigButtonClicked()
{
	targetDeviceChanged( ui.targetDeviceComboBox->currentText() );

	ui.dhcpRadioButton->setChecked( true );
	ui.staticIpLineEdit->setText( "192.168.1.10" );
	ui.netmaskLineEdit->setText( "255.255.255.0" );
	ui.gatewayLineEdit->setText( "192.168.1.1" );
	ui.modbusTcpCheckBox->setCheckState( Qt::Unchecked );
	ui.modbusTcpSpinBox->setValue( 502 );
	ui.modbusRtuCheckBox->setCheckState( Qt::Unchecked );
	ui.modbusRtuSpinBox->setValue( 503 );
	ui.modbusAsciiCheckBox->setCheckState( Qt::Unchecked );
	ui.modbusAsciiSpinBox->setValue( 504 );

	vPortsOverEthernetModel.removeRows( 0, vPortsOverEthernetModel.rowCount() );

	ui.rs485MinIntervalSpinBox->setValue( 0 );

	ui.logMaxSizeSpinBox->setValue( 1024 );
	ui.logOverwritingCheckBox->setChecked( true );
	ui.digitalInputsLogModeComboBox->setCurrentIndex( 0 );
	ui.digitalInputsLogPeriodSpinBox->setValue( 1 );
	ui.analogInputsLogModeComboBox->setCurrentIndex( 0 );
	ui.analogInputsLogPeriodSpinBox->setValue( 10 );
	ui.sensorsLogModeComboBox->setCurrentIndex( 0 );

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
	if( mlink.state() != MLinkClient::State::Connected )
		return;

	QByteArray data = saveConfigToXml();
	if( data.isEmpty() )
	{
		// ADD // FIX
		return;
	}

	mlink.sendComplexData( MarviePackets::ComplexChannel::XmlConfigChannel, data );
	DataTransferProgressWindow window( &mlink, MarviePackets::ComplexChannel::XmlConfigChannel, DataTransferProgressWindow::TransferDir::Sending, this );
	window.setTitleText( "Uploading config" );
	window.exec();
}

void MarvieController::downloadConfigButtonClicked()
{
	mlink.sendPacket( MarviePackets::Type::GetConfigXmlType, QByteArray() );
	DataTransferProgressWindow window( &mlink, MarviePackets::ComplexChannel::XmlConfigChannel, DataTransferProgressWindow::TransferDir::Receiving, this, MarviePackets::Type::ConfigXmlMissingType );
	window.setTitleText( "Downloading config" );
	window.exec();
}

void MarvieController::addVPortOverIpButtonClicked()
{
	vPortsOverEthernetModel.insertRows( vPortsOverEthernetModel.rowCount(), 1 );
}

void MarvieController::removeVPortOverIpButtonClicked()
{
	vPortsOverEthernetModel.removeRows( ui.vPortsOverIpTableView->currentIndex().row(), 1 );
}

void MarvieController::comPortAssignmentChanged( unsigned int id, ComPortsConfigWidget::Assignment previous, ComPortsConfigWidget::Assignment current )
{
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

	auto vPortsOverIp = vPortsOverEthernetModel.modelData();
	for( const auto& i : vPortsOverIp )
		vPorts.append( QString( "Network[%1]" ).arg( i ) );
}

void MarvieController::sensorNameEditReturnPressed()
{
	sensorNameTimer.stop();
	sensorNameTimerTimeout();
}

void MarvieController::sensorNameTimerTimeout()
{
	QStringList matches;
	QStringList supportedSensorsList = sensorDescMap.keys();
	if( !ui.sensorNameEdit->text().isEmpty() )
	{
		QRegExp reg( ui.sensorNameEdit->text() );
		reg.setCaseSensitivity( Qt::CaseInsensitive );
		if( !reg.isValid() )
			return;

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
	}
	else
		matches = supportedSensorsList;

	const QPalette &pal = ui.sensorNameEdit->palette();
	QColor color = pal.color( QPalette::Disabled, QPalette::WindowText );

	popupSensorsListWidget->setUpdatesEnabled( false );
	popupSensorsListWidget->clear();

	for( const auto &i : matches )
	{
		auto item = new QListWidgetItem( popupSensorsListWidget );
		item->setText( i );
		item->setTextColor( color );
	}

	popupSensorsListWidget->setCurrentRow( 0 );
	popupSensorsListWidget->setUpdatesEnabled( true );

	popupSensorsListWidget->move( ui.sensorNameEdit->mapToGlobal( QPoint( 0, ui.sensorNameEdit->height() - 1 ) ) );
	popupSensorsListWidget->setFocus();
	popupSensorsListWidget->show();
}

void MarvieController::sensorNameSearchCompleted()
{
	sensorNameTimer.stop();
	popupSensorsListWidget->hide();
	ui.sensorNameEdit->setFocus();
	QListWidgetItem *item = popupSensorsListWidget->currentItem();
	if( item )
	{
		ui.sensorNameEdit->blockSignals( true );
		ui.sensorNameEdit->setText( item->text() );
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

void MarvieController::sensorMoveUpButtonClicked()
{
	auto item = ui.sensorSettingsTreeWidget->currentItem();
	if( !item || ui.sensorSettingsTreeWidget->topLevelItemCount() == 1 )
		return;
	if( item->parent() )
		item = item->parent();
	int index = ui.sensorSettingsTreeWidget->indexOfTopLevelItem( item );

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

void MarvieController::sensorMoveDownButtonClicked()
{
	auto item = ui.sensorSettingsTreeWidget->currentItem();
	if( !item || ui.sensorSettingsTreeWidget->topLevelItemCount() == 1 )
		return;
	if( item->parent() )
		item = item->parent();
	int index = ui.sensorSettingsTreeWidget->indexOfTopLevelItem( item );

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

void MarvieController::sensorCopyButtonClicked()
{
	auto index = ui.sensorSettingsTreeWidget->currentIndex();
	if( !index.isValid() )
		return;
	if( index.parent().isValid() )
		index = index.parent();
	auto settings = sensorSettingsValues( index.row() );
	QString sensorName = index.data().toString().split( ". " )[1];
	insertSensorSettings( index.row() + 1, sensorName, settings, true );
}

void MarvieController::sensorsClearButtonClicked()
{
	ui.sensorSettingsTreeWidget->clear();
}

void MarvieController::exportModbusRegMapToCsvButtonClicked()
{
	QSettings setting( "settings.ini", QSettings::Format::IniFormat );
	QString name = QFileDialog::getSaveFileName( this, "Export a modbus map to csv", setting.value( "modbusMapDir", QDir::currentPath() ).toString(), "CSV files (*.csv)" );
	if( name.isEmpty() )
		return;
	setting.setValue( "modbusMapDir", QDir( name ).absolutePath().remove( QDir( name ).dirName() ) );

	QFile file( name );
	file.open( QIODevice::WriteOnly );
	file.write( "Name;Offset;Type\n\n" );
	int sensorCount = sensorFieldAddressMapModel.rowCount();
	for( int iSensor = 0; iSensor < sensorCount; ++iSensor )
	{
		QString name = sensorFieldAddressMapModel.index( iSensor, 0 ).data( Qt::DisplayRole ).toString();
		file.write( name.toUtf8() );
		file.write( ";" );
		file.write( sensorFieldAddressMapModel.index( iSensor, 1 ).data( Qt::DisplayRole ).toString().toUtf8() );
		file.write( ";\n" );

		QModelIndex parent = sensorFieldAddressMapModel.index( iSensor, 0 );
		QModelIndex child = sensorFieldAddressMapModel.index( 0, 0, parent );
		int childCount = sensorFieldAddressMapModel.rowCount( parent );
		for( int iChild = 0; iChild < childCount; ++iChild )
		{
			file.write( ( name + "." + child.sibling( iChild, 0 ).data( Qt::DisplayRole ).toString() ).toUtf8() );
			file.write( ( ";" + child.sibling( iChild, 1 ).data( Qt::DisplayRole ).toString() ).toUtf8() );
			file.write( ( ";" + child.sibling( iChild, 2 ).data( Qt::DisplayRole ).toString() + "\n" ).toUtf8() );
		}
		file.write( "\n" );
	}
}

void MarvieController::mlinkStateChanged( MLinkClient::State s )
{
	if( ui.interfaceStackedWidget->currentWidget() == ui.rs232Page )
	{
		switch( s )
		{
		case MLinkClient::State::Disconnected:
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
		switch( s )
		{
		case MLinkClient::State::Disconnected:
			ui.ipEdit->setEnabled( true );
			ui.ethernetConnectButton->setIcon( QIcon( ":/MarvieController/icons/icons8-ethernet-on-filled-50.png" ) );
			break;
		case MLinkClient::State::Connecting:
			ui.ethernetConnectButton->setIcon( QIcon( ":/MarvieController/icons/icons8-ethernet-on-filled-50-orange.png" ) );
			break;
		case MLinkClient::State::Connected:
			ui.ethernetConnectButton->setIcon( QIcon( ":/MarvieController/icons/icons8-ethernet-on-filled-50-green.png" ) );
			break;
		case MLinkClient::State::Disconnecting:
			ui.ethernetConnectButton->setIcon( QIcon( ":/MarvieController/icons/icons8-ethernet-on-filled-50-orange.png" ) );
			break;
		default:
			break;
		}
	}
	else if( ui.interfaceStackedWidget->currentWidget() == ui.bluetoothPage )
	{

	}

	switch( s )
	{
	case MLinkClient::State::Disconnected:
		accountWindow->logOutConfirmed();
		mlink.setIODevice( nullptr );
		mlinkIODevice->waitForBytesWritten( 100 );
		mlinkIODevice->deleteLater();
		mlinkIODevice = nullptr;

		ui.nextInterfaceButton->setEnabled( true );
		resetDeviceLoad();
		ui.vPortTileListWidget->removeAllTiles();
		ui.targetDeviceComboBox->setEnabled( true );
		deviceVPorts.clear();
		deviceSensors.clear();
		ui.deviceSensorsComboBox->clear();
		deviceSupportedSensors.clear();
		resetDeviceInfo();
		deviceState = DeviceState::Unknown;
		syncWindow->hide();
		break;
	case MLinkClient::State::Connecting:
		ui.targetDeviceComboBox->setEnabled( false );
		ui.syncDateTimeButton->setEnabled( true );
		break;
	case MLinkClient::State::Connected:
		accountWindow->logInConfirmed();
		ui.syncDateTimeButton->show();
		ui.sdCardMenuButton->show();
		ui.logMenuButton->show();

		ui.monitoringDataTreeView->clear();
		break;
	case MLinkClient::State::Disconnecting:
		resetDeviceInfo();
		ui.syncDateTimeButton->setEnabled( false );
		deviceState = DeviceState::Unknown;
		syncWindow->hide();
		break;
	default:
		break;
	}
}

void MarvieController::mlinkError( MLinkClient::Error err )
{
	if( err == MLinkClient::Error::AuthorizationError )
		accountWindow->passwordIncorrect();
}

void MarvieController::mlinkNewPacketAvailable( uint8_t type, QByteArray data )
{
	switch( type )
	{
	case MarviePackets::Type::CpuLoadType:
	{
		updateDeviceCpuLoad( reinterpret_cast< const MarviePackets::CpuLoad* >( data.constData() )->load );
		break;
	}
	case MarviePackets::Type::MemoryLoadType:
	{
		const MarviePackets::MemoryLoad* load = reinterpret_cast< const MarviePackets::MemoryLoad* >( data.constData() );
		updateDeviceMemoryLoad( load );
		break;
	}
	case MarviePackets::Type::VPortStatusType:
	{
		const MarviePackets::VPortStatus* status = reinterpret_cast< const MarviePackets::VPortStatus* >( data.constData() );
		auto tile = ui.vPortTileListWidget->tile( status->vPortId );
		tile->show();
		switch( status->state )
		{
		case MarviePackets::VPortStatus::State::Stopped:
			tile->setState( VPortTileWidget::State::Stopped );
			break;
		case MarviePackets::VPortStatus::State::Working:
			tile->setState( VPortTileWidget::State::Working );
			break;
		case MarviePackets::VPortStatus::State::Stopping:
			tile->setState( VPortTileWidget::State::Stopping );
			break;
		default:
			break;
		}
		if( status->sensorId == -1 )
			tile->resetNextSensorRead();
		else
		{
			if( deviceSensors.isEmpty() )
				tile->setNextSensorRead( status->sensorId, "", status->timeLeft );
			else
				tile->setNextSensorRead( status->sensorId, deviceSensors[status->sensorId], status->timeLeft );
		}
		break;
	}
	case MarviePackets::Type::DeviceStatusType:
	{
		updateDeviceStatus( reinterpret_cast< const MarviePackets::DeviceStatus* >( data.constData() ) );
		break;
	}
	case MarviePackets::Type::ServiceStatisticsType:
	{
		updateServiceStatistics( reinterpret_cast< const MarviePackets::ServiceStatistics* >( data.constData() ) );
		break;
	}
	case MarviePackets::Type::EthernetStatusType:
	{
		updateEthernetStatus( reinterpret_cast< const MarviePackets::EthernetStatus* >( data.constData() ) );
		break;
	}
	case MarviePackets::Type::GsmStatusType:
	{
		updateGsmStatus( reinterpret_cast< const MarviePackets::GsmStatus* >( data.constData() ) );
		break;
	}
	case MarviePackets::Type::AnalogInputsDataType:
	{
		const uint8_t* pData = ( const uint8_t* )data.constData();
		uint32_t size = data.size();
		while( size )
		{
			struct BlockDesc
			{
				uint16_t id;
				uint16_t count;
			}* block = ( BlockDesc* )pData;
			ui.monitoringDataTreeView->updateAnalogData( block->id, ( float* )( pData + sizeof( uint16_t ) * 2 ), block->count );
			size -= sizeof( uint16_t ) * 2 + sizeof( float ) * block->count;
			pData += sizeof( uint16_t ) * 2 + sizeof( float ) * block->count;
		}
		break;
	}
	case MarviePackets::Type::DigitInputsDataType:
	{
		const uint8_t* pData = ( const uint8_t* )data.constData();
		uint32_t size = data.size();
		while( size )
		{
			struct BlockDesc
			{
				uint16_t id;
				uint16_t count;
				uint32_t data;
			}* block = ( BlockDesc* )pData;
			ui.monitoringDataTreeView->updateDiscreteData( block->id, block->data, block->count );
			size -= sizeof( BlockDesc );
			pData += sizeof( BlockDesc );
		}
		break;
	}
	case MarviePackets::Type::SensorErrorReportType:
	{
		const MarviePackets::SensorErrorReport* report = reinterpret_cast< const MarviePackets::SensorErrorReport* >( data.constData() );
		VPortTileWidget::SensorError err;
		switch( report->error )
		{
		case MarviePackets::SensorErrorReport::Error::CrcError:
			err = VPortTileWidget::SensorError::CrcError;
			break;
		case MarviePackets::SensorErrorReport::Error::NoResponseError:
			err = VPortTileWidget::SensorError::NoResponseError;
			break;
		default:
			break;
		}
		ui.vPortTileListWidget->tile( report->vPortId )->addSensorReadError( report->sensorId,
																			 deviceSensors.isEmpty() ? "" : deviceSensors[report->sensorId],
																			 err, report->errorCode, toQtDateTime( report->dateTime ) );
		break;
	}
	case MarviePackets::Type::VPortCountType:
	{
		ui.vPortTileListWidget->setTilesCount( *reinterpret_cast< const uint32_t* >( data.constData() ) );
		for( uint i = 0; i < ui.vPortTileListWidget->tilesCount(); ++i )
		{
			auto tile = ui.vPortTileListWidget->tile( i );
			tile->hide();
			if( deviceVPorts.size() )
				tile->setBindInfo( deviceVPorts[i] );
		}
		break;
	}
	case MarviePackets::Type::SyncStartType:
	{
		ui.vPortTileListWidget->removeAllTiles();
		ui.monitoringDataTreeView->clear();
		deviceVPorts.clear();
		deviceSensors.clear();
		ui.deviceSensorsComboBox->clear();
		syncWindow->show();
		break;
	}
	case MarviePackets::Type::FirmwareDescType:
	{
		const MarviePackets::FirmwareDesc* desc = reinterpret_cast< const MarviePackets::FirmwareDesc* >( data.constData() );
		QString targetName = QString( desc->targetName );
		ui.targetDeviceComboBox->setCurrentText( targetName );
		if( ui.targetDeviceComboBox->currentText() != targetName )
			ui.targetDeviceComboBox->setCurrentIndex( -1 );
		break;
	}
	case MarviePackets::Type::SyncEndType:
	{
		syncWindow->hide();
		break;
	}
	case MarviePackets::Type::ConfigResetType:
	{
		ui.vPortTileListWidget->removeAllTiles();
		ui.monitoringDataTreeView->clear();
		deviceVPorts.clear();
		deviceSensors.clear();
		ui.deviceSensorsComboBox->clear();
		break;
	}
	default:
		break;
	}
}

void MarvieController::mlinkNewComplexPacketAvailable( uint8_t channelId, QString name, QByteArray data )
{
	if( channelId == MarviePackets::ComplexChannel::XmlConfigSyncChannel )
	{
		if( !loadConfigFromXml( data ) )
		{
			deviceVPorts.clear();
			deviceSensors.clear();
			// ADD // FIX
		}
		else
		{
			deviceVPorts = vPorts;
			deviceSensors = loadedXmlSensors;
			QStringList list = deviceSensors.toList();
			int n = 1;
			for( auto i = list.begin(); i != list.end(); ++i )
				( *i ).insert( 0, QString( "%1. " ).arg( n++ ) );
			ui.deviceSensorsComboBox->addItems( list );
		}
	}
	else if( channelId == MarviePackets::ComplexChannel::XmlConfigChannel )
	{
		if( !loadConfigFromXml( data ) )
		{
			// ADD // FIX
		}
	}
	else if( channelId == MarviePackets::ComplexChannel::SupportedSensorsListChannel )
		deviceSupportedSensors = QString( data ).split( ',' ).toVector();
	else if( channelId == MarviePackets::ComplexChannel::SensorDataChannel )
	{
		uint8_t sensorId = ( uint8_t )data.constData()[0];
		uint8_t vPortId = ( uint8_t )data.constData()[1];
		if( vPortId < ui.vPortTileListWidget->tilesCount() )
			ui.vPortTileListWidget->tile( vPortId )->removeSensorReadError( sensorId );
		if( deviceSensors.isEmpty() )
			ui.monitoringDataTreeView->updateSensorData( QString( "%1." ).arg( sensorId + 1 ), "",
														 reinterpret_cast< const uint8_t* >( data.constData() + sizeof( uint8_t ) * 2 + sizeof( DateTime ) ),
														 toQtDateTime( *reinterpret_cast< const DateTime* >( data.constData() + sizeof( uint8_t ) * 2 ) ) );
		else
			ui.monitoringDataTreeView->updateSensorData( QString( "%1. %2" ).arg( sensorId + 1 ).arg( deviceSensors[sensorId] ), deviceSensors[sensorId],
														 reinterpret_cast< const uint8_t* >( data.constData() + sizeof( uint8_t ) * 2 + sizeof( DateTime ) ),
														 toQtDateTime( *reinterpret_cast< const DateTime* >( data.constData() + sizeof( uint8_t ) * 2 ) ) );
	}
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
				desc.data.size = c0.attribute( "size", "-1" ).toInt();
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
						case SensorDesc::Data::Type::Array:
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
						case SensorDesc::Data::Type::Group:
						{
							auto newNode = new SensorDesc::Data::Node( type, bias, c.attribute( "name" ) );
							bias = parse( newNode, c.firstChildElement(), bias );
							node->childNodes.append( newNode );
							break;
						}
						case SensorDesc::Data::Type::GroupArray:
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
						case SensorDesc::Data::Type::Unused:
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
				int bias = parse( root, c1, 0 );
				if( desc.data.size == -1 )
					desc.data.size = bias;
				desc.data.root = DataPointer( root );
			}

			auto& modbusDesc = sensorUnfoldedDescMap[i];
			static const std::function< void( QString name, const SensorDesc::Data::Node* node, SensorUnfoldedDesc* desc ) > unroll = []( QString name, const SensorDesc::Data::Node* node, SensorUnfoldedDesc* desc )
			{
				int type = -1;
				switch( node->type )
				{
				case SensorDesc::Data::Type::Char:
					type = ( int )SensorUnfoldedDesc::Type::Char;
					break;
				case SensorDesc::Data::Type::Int8:
					type = ( int )SensorUnfoldedDesc::Type::Int8;
					break;
				case SensorDesc::Data::Type::Uint8:
					type = ( int )SensorUnfoldedDesc::Type::Uint8;
					break;
				case SensorDesc::Data::Type::Int16:
					type = ( int )SensorUnfoldedDesc::Type::Int16;
					break;
				case SensorDesc::Data::Type::Uint16:
					type = ( int )SensorUnfoldedDesc::Type::Uint16;
					break;
				case SensorDesc::Data::Type::Int32:
					type = ( int )SensorUnfoldedDesc::Type::Int32;
					break;
				case SensorDesc::Data::Type::Uint32:
					type = ( int )SensorUnfoldedDesc::Type::Uint32;
					break;
				case SensorDesc::Data::Type::Int64:
					type = ( int )SensorUnfoldedDesc::Type::Int64;
					break;
				case SensorDesc::Data::Type::Uint64:
					type = ( int )SensorUnfoldedDesc::Type::Uint64;
					break;
				case SensorDesc::Data::Type::Float:
					type = ( int )SensorUnfoldedDesc::Type::Float;
					break;
				case SensorDesc::Data::Type::Double:
					type = ( int )SensorUnfoldedDesc::Type::Double;
					break;
				default:
					break;
				}
				if( type != -1 )
				{
					desc->addElement( name + ( name.size() && node->name.front() != '[' ? "." : "" ) + node->name, node->bias + sizeof( DateTime ), ( SensorUnfoldedDesc::Type )type );
					return;
				}

				for( auto child : node->childNodes )
				{
					switch( child->type )
					{
					case SensorDesc::Data::Type::Array:
					case SensorDesc::Data::Type::GroupArray:
					{
						for( int i = 0; i < child->childNodes.size(); ++i )
							unroll( name + ( name.size() ? "." : "" ) + child->name, child->childNodes.at( i ), desc );
						break;
					}
					case SensorDesc::Data::Type::Group:
						unroll( name + ( name.size() && child->name.front() != '[' ? "." : "" ) + child->name, child, desc );
						break;
					default:
						unroll( name, child, desc );
						break;
					}
				}
			};
			modbusDesc.addElement( "dateTime", 0, SensorUnfoldedDesc::Type::DateTime );
			unroll( "", desc.data.root.data(), &modbusDesc );
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

	return saveCanonicalXML( doc ).toLocal8Bit();
}

class PeriodEdit : public QWidget
{
public:
	PeriodEdit()
	{
		secSpinBox = new QSpinBox( this );
		minSpinBox = new QSpinBox( this );
		QObject::connect( secSpinBox, &QSpinBox::editingFinished, [this]()
		{
			auto v = secSpinBox->value();
			secSpinBox->setValue( v % 60 );
			minSpinBox->setValue( minSpinBox->value() + v / 60 );
		} );

		QHBoxLayout* layout = new QHBoxLayout( this );
		layout->setContentsMargins( 0, 0, 0, 0 );
		layout->setSpacing( 4 );
		layout->addWidget( minSpinBox );
		layout->addWidget( new QLabel( "min" ) );
		layout->addWidget( secSpinBox );
		layout->addWidget( new QLabel( "sec" ) );
	}

	void setRange( uint minSec, uint maxSec )
	{
		secSpinBox->setMinimum( minSec );
		secSpinBox->setMaximum( maxSec );
		minSpinBox->setMinimum( minSec / 60 );
		minSpinBox->setMaximum( maxSec / 60 );
	}

	uint value()
	{
		return secSpinBox->value() + minSpinBox->value() * 60;
	}
	void setValue( uint sec )
	{
		secSpinBox->setValue( sec % 60 );
		minSpinBox->setValue( sec / 60 );
	}

private:
	QSpinBox* secSpinBox;
	QSpinBox* minSpinBox;
};

class MsPeriodEdit : public QWidget
{
public:
	MsPeriodEdit( int decimals = 1 )
	{
		secSpinBox = new QDoubleSpinBox( this );
		secSpinBox->setDecimals( decimals );
		minSpinBox = new QSpinBox( this );
		QObject::connect( secSpinBox, &QSpinBox::editingFinished, [this]()
		{
			auto v = secSpinBox->value();
			secSpinBox->setValue( ( int( v * 1000 ) % 60000 ) / 1000.0 );
			minSpinBox->setValue( minSpinBox->value() + ( int )v / 60 );
		} );

		QHBoxLayout* layout = new QHBoxLayout( this );
		layout->setContentsMargins( 0, 0, 0, 0 );
		layout->setSpacing( 4 );
		layout->addWidget( minSpinBox );
		layout->addWidget( new QLabel( "min" ) );
		layout->addWidget( secSpinBox );
		layout->addWidget( new QLabel( "sec" ) );
	}

	void setRange( uint minMSec, uint maxMSec )
	{
		secSpinBox->setMinimum( minMSec / 1000 );
		secSpinBox->setMaximum( maxMSec / 1000 );
		minSpinBox->setMinimum( minMSec / 1000 / 60 );
		minSpinBox->setMaximum( maxMSec / 1000 / 60 );
	}

	uint value()
	{
		return int( secSpinBox->value() * 1000 ) + minSpinBox->value() * 60;
	}
	void setValue( uint msec )
	{
		secSpinBox->setValue( ( msec % 60000 ) / 1000.0 );
		minSpinBox->setValue( msec / 1000 / 60 );
	}

private:
	QDoubleSpinBox * secSpinBox;
	QSpinBox* minSpinBox;
};

QTreeWidgetItem* MarvieController::insertSensorSettings( int position, QString sensorName, QMap< QString, QString > sensorSettingsValues, bool needUpdateName /*= false*/ )
{
	assert( sensorDescMap.contains( sensorName ) );
	auto& desc = sensorDescMap[sensorName].settings.prmList;
	if( needUpdateName )
	{
		for( int i = ui.sensorSettingsTreeWidget->topLevelItemCount() - 1; i >= position; --i )
		{
			auto item = ui.sensorSettingsTreeWidget->topLevelItem( i );
			QString sensorName = item->text( 0 ).split( ". " )[1];
			item->setText( 0, QString( "%1. %2" ).arg( i + 1 + 1 ).arg( sensorName ) );
			auto objects = ui.sensorSettingsTreeWidget->findChildren< QObject* >( QRegExp( QString( "^%1\\*" ).arg( i + 1 ) ) );
			for( auto& object : objects )
			{
				QString settingName = object->objectName().split( '*' )[1];
				object->setObjectName( QString( "%1*%2" ).arg( i + 1 + 1 ).arg( settingName ) );
			}
		}
	}
	QTreeWidgetItem* topItem = new QTreeWidgetItem;
	topItem->setSizeHint( 1, QSize( 1, 26 ) );
	topItem->setText( 0, QString( "%1. %2" ).arg( position + 1 ).arg( sensorName ) );
	ui.sensorSettingsTreeWidget->insertTopLevelItem( position, topItem );

	auto addContent = [this, topItem, position]( QString name, QWidget* content )
	{
		QTreeWidgetItem* item = new QTreeWidgetItem( topItem );
		item->setText( 0, name );

		QWidget* widget = new QWidget;
		QHBoxLayout* layout = new QHBoxLayout( widget );
		layout->setContentsMargins( QMargins( 0, 0, 2, 0 ) );
		layout->addWidget( content );
		content->setFixedHeight( 24 );
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

		PeriodEdit* periodEdit = new PeriodEdit;
		periodEdit->setRange( 0, 86400 );
		if( sensorSettingsValues.contains( "normalPeriod" ) )
			periodEdit->setValue( sensorSettingsValues["normalPeriod"].toInt() );
		addContent( "normalPeriod", periodEdit );

		periodEdit = new PeriodEdit;
		periodEdit->setRange( 0, 86400 );
		if( sensorSettingsValues.contains( "emergencyPeriod" ) )
			periodEdit->setValue( sensorSettingsValues["emergencyPeriod"].toInt() );
		addContent( "emergencyPeriod", periodEdit );

		QLineEdit* lineEdit = new QLineEdit;
		lineEdit->setMaxLength( 150 );
		if( sensorSettingsValues.contains( "name" ) )
			lineEdit->setText( sensorSettingsValues["name"] );
		addContent( "name", lineEdit );
	}
	else /*if( sensorDescMap[sensorName].settings.target == SensorDesc::Settings::Target::SR )*/
	{
		QSpinBox* spinBox = new QSpinBox;
		spinBox->setMaximum( 7 );
		spinBox->setMinimum( 0 );
		if( sensorSettingsValues.contains( "blockID" ) )
			spinBox->setValue( sensorSettingsValues["blockID"].toInt() );
		addContent( "blockID", spinBox );

		MsPeriodEdit* periodEdit = new MsPeriodEdit;
		periodEdit->setRange( 100, 86400000 );
		if( sensorSettingsValues.contains( "logPeriod" ) )
			periodEdit->setValue( sensorSettingsValues["logPeriod"].toInt() );
		else
			periodEdit->setValue( 1000 );
		addContent( "logPeriod", periodEdit );

		QLineEdit* lineEdit = new QLineEdit;
		lineEdit->setMaxLength( 150 );
		if( sensorSettingsValues.contains( "name" ) )
			lineEdit->setText( sensorSettingsValues["name"] );
		addContent( "name", lineEdit );
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
				if( !prm->regExp.pattern().isEmpty() )
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
	auto list = ui.sensorSettingsTreeWidget->findChildren< QObject* >( QRegExp( QString( "^" ) + mlist[0] + "\\*" ) );
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
		map["normalPeriod"] = QString( "%1" ).arg( ui.sensorSettingsTreeWidget->findChild< PeriodEdit* >( ENAME( "normalPeriod" ) )->value() );
		map["emergencyPeriod"] = QString( "%1" ).arg( ui.sensorSettingsTreeWidget->findChild< PeriodEdit* >( ENAME( "emergencyPeriod" ) )->value() );
		map["name"] = QString( "%1" ).arg( ui.sensorSettingsTreeWidget->findChild< QLineEdit* >( ENAME( "name" ) )->text() );
	}
	else /*if( sensorDescMap[sensorName].settings.target == SensorDesc::Settings::Target::SR )*/
	{
		map["blockID"] = QString( "%1" ).arg( ui.sensorSettingsTreeWidget->findChild< QSpinBox* >( ENAME( "blockID" ) )->value() );
		map["logPeriod"] = QString( "%1" ).arg( ui.sensorSettingsTreeWidget->findChild< MsPeriodEdit* >( ENAME( "logPeriod" ) )->value() );
		map["name"] = QString( "%1" ).arg( ui.sensorSettingsTreeWidget->findChild< QLineEdit* >( ENAME( "name" ) )->text() );
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

void MarvieController::fixSensorVPortIds( bool needUpdate )
{
	auto vPortNames = vPortFullNames();
	auto list = ui.sensorSettingsTreeWidget->findChildren< QComboBox* >( QRegExp( "\\*vPortID" ) );
	for( auto& i : list )
	{
		if( !i->currentText().isEmpty() && !vPortNames.contains( i->currentText() ) )
		{
			QString name = i->currentText().split( " : " )[1];
			auto id = vPorts.indexOf( name );
			if( id != -1 )
			{
				i->clear();
				i->addItems( vPortNames );
				i->setCurrentIndex( id );
			}
			else if( needUpdate )
				i->clear();
		}
	}
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
	loadedXmlSensors.clear();
	if( !c0.isNull() )
	{
		auto c1 = c0.firstChildElement();
		while( !c1.isNull() )
		{
			loadedXmlSensors.append( c1.tagName() );
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

	if( ui.targetDeviceComboBox->isEnabled() )
	{
		ui.targetDeviceComboBox->blockSignals( true );
		if( target == Target::QX )
			ui.targetDeviceComboBox->setCurrentText( "QX" );
		else
			ui.targetDeviceComboBox->setCurrentText( "VX" );
		ui.targetDeviceComboBox->blockSignals( false );
	}
	else
	{
		if( ui.targetDeviceComboBox->currentText() == "QX" && target != Target::QX ||
			ui.targetDeviceComboBox->currentText() == "VX" && target != Target::VX ||
			ui.targetDeviceComboBox->currentIndex() == -1 )
		{
			xmlMessageHandler.description = "Configuration not recognized.";
			return false;
		}
	}
	newConfigButtonClicked();

	auto c1 = configRoot.firstChildElement( "comPortsConfig" );
	auto c2 = c1.firstChildElement();
	for( int i = 0; i < ui.comPortsConfigWidget->comPortsCount(); ++i )
	{
		auto c3 = c2.firstChildElement();		
		if( c3.tagName() == "vPort" )
		{
			ui.comPortsConfigWidget->setAssignment( i, ComPortsConfigWidget::Assignment::VPort );
			QMap< QString, QVariant > prms;
			prms.insert( "format", c3.attribute( "frameFormat", "B8N" ) );
			prms.insert( "baudrate", c3.attribute( "baudrate", "9600" ).toInt() );
			ui.comPortsConfigWidget->setRelatedParameters( i, prms );
		}
		else if( c3.tagName() == "modbusRtuSlave" )
		{
			ui.comPortsConfigWidget->setAssignment( i, ComPortsConfigWidget::Assignment::ModbusRtuSlave );
			QMap< QString, QVariant > prms;
			prms.insert( "format", c3.attribute( "frameFormat", "B8N" ) );
			prms.insert( "baudrate", c3.attribute( "baudrate", "9600" ).toInt() );
			prms.insert( "address", c3.attribute( "address", "0" ).toInt() );
			ui.comPortsConfigWidget->setRelatedParameters( i, prms );
		}
		else if( c3.tagName() == "modbusAsciiSlave" )
		{
			ui.comPortsConfigWidget->setAssignment( i, ComPortsConfigWidget::Assignment::ModbusAsciiSlave );
			QMap< QString, QVariant > prms;
			prms.insert( "format", c3.attribute( "frameFormat", "B8N" ) );
			prms.insert( "baudrate", c3.attribute( "baudrate", "9600" ).toInt() );
			prms.insert( "address", c3.attribute( "address", "0" ).toInt() );
			ui.comPortsConfigWidget->setRelatedParameters( i, prms );
		}
		else if( c3.tagName() == "gsmModem" )
		{
			ui.comPortsConfigWidget->setAssignment( i, ComPortsConfigWidget::Assignment::GsmModem );
			QMap< QString, QVariant > prms;
			prms.insert( "pinCode", c3.attribute( "pinCode", "" ) );
			prms.insert( "apn", c3.attribute( "apn", "" ) );
			ui.comPortsConfigWidget->setRelatedParameters( i, prms );
		}
		else if( c3.tagName() == "multiplexer" )
		{
			ui.comPortsConfigWidget->setAssignment( i, ComPortsConfigWidget::Assignment::Multiplexer );
			QMap< QString, QVariant > prms;
			int counter = 0;
			auto c4 = c3.firstChildElement();
			while( !c4.isNull() )
			{
				prms.insert( QString( "%1.format" ).arg( counter ), c4.attribute( "frameFormat", "B8N" ) );
				prms.insert( QString( "%1.baudrate" ).arg( counter++ ), c4.attribute( "baudrate", "9600" ).toInt() );

				c4 = c4.nextSiblingElement();
			}
			ui.comPortsConfigWidget->setRelatedParameters( i, prms );
		}

		c2 = c2.nextSiblingElement();
	}

	c1 = configRoot.firstChildElement( "networkConfig" );
	c2 = c1.firstChildElement( "ethernet" );
	if( c2.firstChildElement( "dhcp" ).text() == "enable" )
		ui.dhcpRadioButton->setChecked( true );
	else
		ui.staticIpRadioButton->setChecked( true );
	ui.staticIpLineEdit->setText( c2.firstChildElement( "ip" ).text() );
	ui.netmaskLineEdit->setText( c2.firstChildElement( "netmask" ).text() );
	ui.gatewayLineEdit->setText( c2.firstChildElement( "gateway" ).text() );
	c2 = c1.firstChildElement( "modbusRtuServer" );
	if( !c2.isNull() )
	{
		ui.modbusRtuCheckBox->setCheckState( Qt::Checked );
		ui.modbusRtuSpinBox->setValue( c2.attribute( "port" ).toInt() );
	}
	c2 = c1.firstChildElement( "modbusTcpServer" );
	if( !c2.isNull() )
	{
		ui.modbusTcpCheckBox->setCheckState( Qt::Checked );
		ui.modbusTcpSpinBox->setValue( c2.attribute( "port" ).toInt() );
	}
	c2 = c1.firstChildElement( "modbusAsciiServer" );
	if( !c2.isNull() )
	{
		ui.modbusAsciiCheckBox->setCheckState( Qt::Checked );
		ui.modbusAsciiSpinBox->setValue( c2.attribute( "port" ).toInt() );
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

	c1 = configRoot.firstChildElement( "sensorReadingConfig" );
	if( !c1.isNull() )
	{
		c2 = c1.firstChildElement( "rs485MinInterval" );
		ui.rs485MinIntervalSpinBox->setValue( c2.attribute( "value" ).toInt() );
	}

	c1 = configRoot.firstChildElement( "logConfig" );
	if( !c1.isNull() )
	{
		ui.logMaxSizeSpinBox->setValue( c1.attribute( "maxSize" ).toInt() );
		ui.logOverwritingCheckBox->setChecked( c1.attribute( "overwriting" ).replace( "true", "1" ).replace( "false", "0" ).toInt() );

		c2 = c1.firstChildElement( "digitInputs" );
		QString value = c2.attribute( "mode" );
		if( value == "byTime" )
		{
			ui.digitalInputsLogModeComboBox->setCurrentText( "ByTime" );
			ui.digitalInputsLogPeriodSpinBox->setValue( c2.attribute( "period" ).toInt() / 1000.0 );
		}
		else if( value == "byChange" )
		{
			ui.digitalInputsLogModeComboBox->setCurrentText( "ByChange" );
			ui.digitalInputsLogPeriodSpinBox->setValue( c2.attribute( "period" ).toInt() / 1000.0 );
		}

		c2 = c1.firstChildElement( "analogInputs" );
		if( c2.attribute( "mode" ) == "byTime" )
		{
			ui.analogInputsLogModeComboBox->setCurrentText( "ByTime" );
			ui.analogInputsLogPeriodSpinBox->setValue( c2.attribute( "period" ).toInt() / 1000 );
		}

		if( c1.firstChildElement( "sensors" ).attribute( "mode" ) == "enabled" )
			ui.sensorsLogModeComboBox->setCurrentText( "Enabled" );
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
	fixSensorVPortIds( true );

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
	if( ui.staticIpLineEdit->text().isEmpty() )
		ui.staticIpLineEdit->setText( "192.168.1.2" );
	if( ui.netmaskLineEdit->text().isEmpty() )
		ui.netmaskLineEdit->setText( "255.255.255.0" );
	if( ui.gatewayLineEdit->text().isEmpty() )
		ui.gatewayLineEdit->setText( "192.168.1.1" );
	if( !ui.staticIpLineEdit->hasAcceptableInput() || !ui.netmaskLineEdit->hasAcceptableInput() || !ui.gatewayLineEdit->hasAcceptableInput() )
		xmlMessageHandler.description.append( "Invalid network config" );
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
			c2.setAttribute( "frameFormat", prms["format"].toString() );
			c2.setAttribute( "baudrate", prms["baudrate"].toInt() );
			break;
		}
		case ComPortsConfigWidget::Assignment::ModbusRtuSlave:
		{
			auto c2 = doc.createElement( "modbusRtuSlave" );
			c1.appendChild( c2 );
			auto prms = ui.comPortsConfigWidget->relatedParameters( ( unsigned int )comCounter );
			c2.setAttribute( "frameFormat", prms["format"].toString() );
			c2.setAttribute( "baudrate", prms["baudrate"].toInt() );
			c2.setAttribute( "address", prms["address"].toInt() );
			break;
		}
		case ComPortsConfigWidget::Assignment::ModbusAsciiSlave:
		{
			auto c2 = doc.createElement( "modbusAsciiSlave" );
			c1.appendChild( c2 );
			auto prms = ui.comPortsConfigWidget->relatedParameters( ( unsigned int )comCounter );
			c2.setAttribute( "frameFormat", prms["format"].toString() );
			c2.setAttribute( "baudrate", prms["baudrate"].toInt() );
			c2.setAttribute( "address", prms["address"].toInt() );
			break;
		}
		case ComPortsConfigWidget::Assignment::GsmModem:
		{
			auto c2 = doc.createElement( "gsmModem" );
			c1.appendChild( c2 );
			auto prms = ui.comPortsConfigWidget->relatedParameters( ( unsigned int )comCounter );
			c2.setAttribute( "apn", prms["apn"].toString() );
			if( !prms["pinCode"].toString().isEmpty() )
				c2.setAttribute( "pinCode", prms["pinCode"].toString() );
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
				c3.setAttribute( "frameFormat", prms[QString( "%1.format" ).arg( i )].toString() );
				c3.setAttribute( "baudrate", prms[QString( "%1.baudrate" ).arg( i )].toInt() );
			}
			break;
		}
		default:
			break;
		}
		++comCounter;
	}
	root.appendChild( doc.createComment( "==================================================================" ) );

	{
		auto c1 = doc.createElement( "networkConfig" );
		root.appendChild( c1 );
		auto c2 = doc.createElement( "ethernet" );
		c1.appendChild( c2 );

		auto c3 = doc.createElement( "dhcp" );
		c2.appendChild( c3 );
		c3.appendChild( doc.createTextNode( ui.dhcpRadioButton->isChecked() ? "enable" : "disable" ) );

		c3 = doc.createElement( "ip" );
		c2.appendChild( c3 );
		c3.appendChild( doc.createTextNode( ui.staticIpLineEdit->text() ) );

		c3 = doc.createElement( "netmask" );
		c2.appendChild( c3 );
		c3.appendChild( doc.createTextNode( ui.netmaskLineEdit->text() ) );

		c3 = doc.createElement( "gateway" );
		c2.appendChild( c3 );
		c3.appendChild( doc.createTextNode( ui.gatewayLineEdit->text() ) );

		if( ui.modbusRtuCheckBox->checkState() == Qt::Checked )
		{
			auto c2 = doc.createElement( "modbusRtuServer" );
			c1.appendChild( c2 );
			c2.setAttribute( "port", ui.modbusRtuSpinBox->value() );
		}
		if( ui.modbusTcpCheckBox->checkState() == Qt::Checked )
		{
			auto c2 = doc.createElement( "modbusTcpServer" );
			c1.appendChild( c2 );
			c2.setAttribute( "port", ui.modbusTcpSpinBox->value() );
		}
		if( ui.modbusAsciiCheckBox->checkState() == Qt::Checked )
		{
			auto c2 = doc.createElement( "modbusAsciiServer" );
			c1.appendChild( c2 );
			c2.setAttribute( "port", ui.modbusAsciiSpinBox->value() );
		}
		for( int i = 0; i < vPortsOverEthernetModel.rowCount(); ++i )
		{
			auto c2 = doc.createElement( "vPortOverIp" );
			c1.appendChild( c2 );
			c2.setAttribute( "port", vPortsOverEthernetModel.data( vPortsOverEthernetModel.index( i, 1 ), Qt::DisplayRole ).toInt() );
			c2.setAttribute( "ip", vPortsOverEthernetModel.data( vPortsOverEthernetModel.index( i, 0 ), Qt::DisplayRole ).toString() );
		}
	}
	root.appendChild( doc.createComment( "==================================================================" ) );

	{
		auto c1 = doc.createElement( "sensorReadingConfig" );
		root.appendChild( c1 );
		auto c2 = doc.createElement( "rs485MinInterval" );
		c1.appendChild( c2 );
		c2.setAttribute( "value", ui.rs485MinIntervalSpinBox->value() );
	}
	root.appendChild( doc.createComment( "==================================================================" ) );

	{
		auto c1 = doc.createElement( "logConfig" );
		root.appendChild( c1 );
		c1.setAttribute( "maxSize", ui.logMaxSizeSpinBox->value() );
		c1.setAttribute( "overwriting", ui.logOverwritingCheckBox->isChecked() ? "true" : "false" );

		auto c2 = doc.createElement( "digitInputs" );
		c1.appendChild( c2 );
		if( ui.digitalInputsLogModeComboBox->currentText() == "Disabled" )
			c2.setAttribute( "mode", "disabled" );
		else
		{
			c2.setAttribute( "period", int( ui.digitalInputsLogPeriodSpinBox->value() * 1000 ) );
			if( ui.digitalInputsLogModeComboBox->currentText() == "ByTime" )
				c2.setAttribute( "mode", "byTime" );
			else if( ui.digitalInputsLogModeComboBox->currentText() == "ByChange" )
				c2.setAttribute( "mode", "byChange" );
		}

		c2 = doc.createElement( "analogInputs" );
		c1.appendChild( c2 );
		if( ui.analogInputsLogModeComboBox->currentText() == "Disabled" )
			c2.setAttribute( "mode", "disabled" );
		else if( ui.analogInputsLogModeComboBox->currentText() == "ByTime" )
		{
			c2.setAttribute( "mode", "byTime" );
			c2.setAttribute( "period", ui.analogInputsLogPeriodSpinBox->value() * 1000 );
		}

		c2 = doc.createElement( "sensors" );
		c1.appendChild( c2 );
		if( ui.sensorsLogModeComboBox->currentText() == "Disabled" )
			c2.setAttribute( "mode", "disabled" );
		else if( ui.sensorsLogModeComboBox->currentText() == "Enabled" )
			c2.setAttribute( "mode", "enabled" );
	}
	root.appendChild( doc.createComment( "==================================================================" ) );

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

				c3 = doc.createElement( "name" );
				c2.appendChild( c3 );
				c3.appendChild( doc.createTextNode( settingsValues["name"] ) );
			}
			else if( settings.target == SensorDesc::Settings::Target::SR )
			{
				auto c3 = doc.createElement( "blockID" );
				c2.appendChild( c3 );
				c3.appendChild( doc.createTextNode( settingsValues["blockID"] ) );

				c3 = doc.createElement( "logPeriod" );
				c2.appendChild( c3 );
				c3.appendChild( doc.createTextNode( settingsValues["logPeriod"] ) );

				c3 = doc.createElement( "name" );
				c2.appendChild( c3 );
				c3.appendChild( doc.createTextNode( settingsValues["name"] ) );
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

	return /*QByteArray( "<?xml version=\"1.0\"?>\n" ) +*/ saveCanonicalXML( doc ).toLocal8Bit();
}

void MarvieController::updateDeviceCpuLoad( float cpuLoad )
{
	static_cast< QPieSeries* >( ui.cpuLoadChartView->chart()->series()[0] )->slices()[0]->setValue( cpuLoad );
	static_cast< QPieSeries* >( ui.cpuLoadChartView->chart()->series()[0] )->slices()[1]->setValue( 100.0 - cpuLoad );
	QSplineSeries* splineSeries = static_cast< QSplineSeries* >( ui.cpuLoadSeriesChartView->chart()->series()[0] );
	if( splineSeries->count() == DEVICE_LOAD_FREQ * CPU_LOAD_SERIES_WINDOW )
		splineSeries->remove( 0 );
	if( splineSeries->points().size() )
		splineSeries->append( QPointF( splineSeries->points().last().x() + 1.0 / DEVICE_LOAD_FREQ, cpuLoad ) );
	else
		splineSeries->append( QPointF( 0, cpuLoad ) );
	QScatterSeries* scatterSeries = static_cast< QScatterSeries* >( ui.cpuLoadSeriesChartView->chart()->series()[1] );
	scatterSeries->clear();
	auto last = splineSeries->points().last();
	scatterSeries->append( last );
	ui.cpuLoadSeriesChartView->chart()->axisX()->setRange( last.x() - CPU_LOAD_SERIES_WINDOW, last.x() + 0.5 );
	ui.cpuLoadLabel->setText( QString( "CPU\n%1%" ).arg( cpuLoad ) );
}

void MarvieController::updateDeviceMemoryLoad( const MarviePackets::MemoryLoad* load )
{
	uint32_t totalRam = load->totalGRam + load->totalCcRam;
	uint32_t allocatedRam = totalRam - load->freeGRam - load->freeCcRam;
	uint32_t heapRam = allocatedRam - load->gRamHeapSize - load->ccRamHeapSize;

	static_cast< QPieSeries* >( ui.memoryLoadChartView->chart()->series()[0] )->slices()[0]->setValue( heapRam );
	static_cast< QPieSeries* >( ui.memoryLoadChartView->chart()->series()[0] )->slices()[1]->setValue( allocatedRam - heapRam );
	static_cast< QPieSeries* >( ui.memoryLoadChartView->chart()->series()[0] )->slices()[2]->setValue( totalRam - allocatedRam );
	ui.memoryLoadLabel->setText( QString( "Memory\n%1KB" ).arg( ( heapRam + 512 ) / 1024 ) );
	memStat->setStatistics( load->totalGRam - load->freeGRam, load->gRamHeapSize, load->gRamHeapFragments, load->gRamHeapLargestFragmentSize,
							load->totalCcRam - load->freeCcRam, load->ccRamHeapSize, load->ccRamHeapFragments, load->ccRamHeapLargestFragmentSize );

	if( load->sdCardCapacity != 0 )
	{
		static_cast< QPieSeries* >( ui.sdLoadChartView->chart()->series()[0] )->slices()[0]->setValue( load->sdCardCapacity - load->sdCardFreeSpace );
		static_cast< QPieSeries* >( ui.sdLoadChartView->chart()->series()[0] )->slices()[1]->setValue( load->sdCardFreeSpace );
		ui.sdLoadLabel->setText( QString( "SD\n%1MB" ).arg( ( load->sdCardCapacity - load->sdCardFreeSpace + 1024 * 1024 / 2 ) / 1024 / 1024 ) );
	}
	else
	{
		static_cast< QPieSeries* >( ui.sdLoadChartView->chart()->series()[0] )->slices()[0]->setValue( 0.0 );
		static_cast< QPieSeries* >( ui.sdLoadChartView->chart()->series()[0] )->slices()[1]->setValue( 100.0 );
		ui.sdLoadLabel->setText( "SD\nN/A" );
	}
	sdStat->setStatistics( load->sdCardCapacity, load->sdCardFreeSpace, load->logSize );
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
	memStat->setStatistics( 0, 0, 0, 0, 0, 0, 0, 0 );

	static_cast< QPieSeries* >( ui.sdLoadChartView->chart()->series()[0] )->slices()[0]->setValue( 0.0 );
	static_cast< QPieSeries* >( ui.sdLoadChartView->chart()->series()[0] )->slices()[1]->setValue( 100.0 );
	ui.sdLoadLabel->setText( "SD\n0MB" );
	sdStat->setStatistics( 0, 0, 0 );
}

void MarvieController::updateDeviceStatus( const MarviePackets::DeviceStatus* status )
{
	ui.deviceDateTimeLabel->setText( QString( "Time: " ) + printDateTime( toQtDateTime( status->dateTime ) ) );
	ui.deviceStateLabel->setToolTip( "" );
	switch( status->state )
	{
	case MarviePackets::DeviceStatus::DeviceState::Reconfiguration:
		deviceState = DeviceState::Reconfiguration;
		ui.deviceStateLabel->setText( "State: reconfiguration" );
		break;
	case MarviePackets::DeviceStatus::DeviceState::Working:
		deviceState = DeviceState::Working;
		ui.deviceStateLabel->setText( "State: working" );
		break;
	case MarviePackets::DeviceStatus::DeviceState::IncorrectConfiguration:
		deviceState = DeviceState::IncorrectConfiguration;
		ui.deviceStateLabel->setText( "State: incorrect configuration" );
		switch( status->configError )
		{
		case MarviePackets::DeviceStatus::ConfigError::NoConfiglFile:
			ui.deviceStateLabel->setToolTip( "No config file" );
			break;
		case MarviePackets::DeviceStatus::ConfigError::XmlStructureError:
			ui.deviceStateLabel->setToolTip( "Xml structure error" );
			break;
		case MarviePackets::DeviceStatus::ConfigError::ComPortsConfigError:
			ui.deviceStateLabel->setToolTip( "Com-ports configuration error" );
			break;
		case MarviePackets::DeviceStatus::ConfigError::NetworkConfigError:
			ui.deviceStateLabel->setToolTip( "Network configuration error" );
			break;
		case MarviePackets::DeviceStatus::ConfigError::SensorReadingConfigError:
			ui.deviceStateLabel->setToolTip( "Sensor reading configuration error" );
			break;
		case MarviePackets::DeviceStatus::ConfigError::LogConfigError:
			ui.deviceStateLabel->setToolTip( "Log configuration error" );
			break;
		case MarviePackets::DeviceStatus::ConfigError::SensorsConfigError:
			ui.deviceStateLabel->setToolTip( "Sensors configuration error" );
			break;
		default:
			break;
		}
		break;
	default:
		deviceState = DeviceState::Unknown;
		break;
	}

	switch( status->sdCardStatus )
	{
	case MarviePackets::DeviceStatus::SdCardStatus::NotInserted:
		deviceSdCardStatus = SdCardStatus::NotInserted;
		ui.sdCardStatusLabel->setText( "SD card: not inserted" );
		break;
	case MarviePackets::DeviceStatus::SdCardStatus::Initialization:
		deviceSdCardStatus = SdCardStatus::Initialization;
		ui.sdCardStatusLabel->setText( "SD card: initialization" );
		break;
	case MarviePackets::DeviceStatus::SdCardStatus::InitFailed:
		deviceSdCardStatus = SdCardStatus::InitFailed;
		ui.sdCardStatusLabel->setText( "SD card: initialization failed" );
		break;
	case MarviePackets::DeviceStatus::SdCardStatus::BadFileSystem:
		deviceSdCardStatus = SdCardStatus::BadFileSystem;
		ui.sdCardStatusLabel->setText( "SD card: bad file system" );
		break;
	case MarviePackets::DeviceStatus::SdCardStatus::Formatting:
		deviceSdCardStatus = SdCardStatus::Formatting;
		ui.sdCardStatusLabel->setText( "SD card: formatting" );
		break;
	case MarviePackets::DeviceStatus::SdCardStatus::Working:
		deviceSdCardStatus = SdCardStatus::Working;
		ui.sdCardStatusLabel->setText( "SD card: working" );
		break;
	default:
		deviceSdCardStatus = SdCardStatus::Unknown;
		break;
	}

	switch( status->logState )
	{
	case MarviePackets::DeviceStatus::LogState::Off:
		deviceLogState = LogState::Off;
		ui.logStateLabel->setText( "Log: off" );
		break;
	case MarviePackets::DeviceStatus::LogState::Stopped:
		deviceLogState = LogState::Off;
		ui.logStateLabel->setText( "Log: stopped" );
		break;
	case MarviePackets::DeviceStatus::LogState::Working:
		deviceLogState = LogState::Off;
		ui.logStateLabel->setText( "Log: working" );
		break;
	case MarviePackets::DeviceStatus::LogState::Archiving:
		deviceLogState = LogState::Off;
		ui.logStateLabel->setText( "Log: archiving" );
		break;
	case MarviePackets::DeviceStatus::LogState::Stopping:
		deviceLogState = LogState::Stopping;
		ui.logStateLabel->setText( "Log: stopping" );
		break;
	default:
		deviceLogState = LogState::Unknown;
		break;
	}
}

QString printIp( uint32_t ip )
{
	return QString( "%1.%2.%3.%4" ).arg( ip >> 24 ).arg( ( ip >> 16 ) & 0xFF ).
		arg( ( ip >> 8 ) & 0xFF ).arg( ip & 0xFF );
}

void MarvieController::updateEthernetStatus( const MarviePackets::EthernetStatus* status )
{
	ui.lanIpLabel->setText( QString( "IP: " ) + printIp( status->ip ) );
	ui.lanNetmaskLabel->setText( QString( "Netmask: " ) + printIp( status->netmask ) );
	ui.lanGatewayLabel->setText( QString( "Gateway: " ) + printIp( status->gateway ) );
}

void MarvieController::updateGsmStatus( const MarviePackets::GsmStatus* status )
{
	switch( status->state )
	{
	case MarviePackets::GsmStatus::State::Stopped:
		if( status->error == MarviePackets::GsmStatus::Error::AuthenticationError )
			ui.gsmStateLabel->setText( "State: incorrect PIN" );
		else
			ui.gsmStateLabel->setText( "State: stopped" );
		break;
	case MarviePackets::GsmStatus::State::Initializing:
		ui.gsmStateLabel->setText( "State: initializing" );
		break;
	case MarviePackets::GsmStatus::State::Working:
		ui.gsmStateLabel->setText( "State: working" );
		break;
	case MarviePackets::GsmStatus::State::Stopping:
		ui.gsmStateLabel->setText( "State: stopped" );
		break;
	case MarviePackets::GsmStatus::State::Off:
		ui.gsmStateLabel->setText( "State: off" );
		break;
	default:
		break;
	}
	/*switch( status->error )
	{
	case MarviePackets::GsmStatus::Error::NoError:
		break;
	case MarviePackets::GsmStatus::Error::AuthenticationError:
		break;
	case MarviePackets::GsmStatus::Error::TimeoutError:
		break;
	case MarviePackets::GsmStatus::Error::UnknownError:
		break;
	default:
		break;
	}*/
	ui.gsmIpLabel->setText( QString( "IP: " ) + printIp( status->ip ) );
}

void MarvieController::updateServiceStatistics( const MarviePackets::ServiceStatistics* stat )
{
	if( stat->tcpModbusRtuClientsCount == -1 )
		ui.modbusRtuStatusLabel->setText( "ModbusRTU: off" );
	else
		ui.modbusRtuStatusLabel->setText( QString( "ModbusRTU: %1" ).arg( stat->tcpModbusRtuClientsCount ) );
	if( stat->tcpModbusAsciiClientsCount == -1 )
		ui.modbusAsciiStatusLabel->setText( "ModbusAscii: off" );
	else
		ui.modbusAsciiStatusLabel->setText( QString( "ModbusAscii: %1" ).arg( stat->tcpModbusAsciiClientsCount ) );
	if( stat->tcpModbusIpClientsCount == -1 )
		ui.modbusTcpStatusLabel->setText( "ModbusTcp: off" );
	else
		ui.modbusTcpStatusLabel->setText( QString( "ModbusTcp: %1" ).arg( stat->tcpModbusIpClientsCount ) );
}

void MarvieController::resetDeviceInfo()
{
	// Device status
	deviceState = DeviceState::Unknown;
	deviceSdCardStatus = SdCardStatus::Unknown;
	deviceLogState = LogState::Unknown;

	ui.deviceDateTimeLabel->setText( "Time: unknown" );
	ui.deviceStateLabel->setText( "State: unknown" );
	ui.sdCardStatusLabel->setText( "SD card: unknown" );	

	ui.syncDateTimeButton->hide();
	ui.sdCardMenuButton->hide();
	ui.logMenuButton->hide();

	// Ethernet status
	ui.lanIpLabel->setText( "IP: 0.0.0.0" );
	ui.lanNetmaskLabel->setText( "Netmask: 0.0.0.0" );
	ui.lanGatewayLabel->setText( "Gateway: 0.0.0.0" );

	// Gsm status
	ui.gsmStateLabel->setText( "State: unknown" );
	ui.gsmIpLabel->setText( "IP: 0.0.0.0" );

	// Service statistics
	ui.modbusRtuStatusLabel->setText( "ModbusRTU: unknown" );
	ui.modbusTcpStatusLabel->setText( "ModbusTCP: unknown" );
	ui.modbusAsciiStatusLabel->setText( "ModbusASCII: unknown" );
}

DateTime MarvieController::toDeviceDateTime( const QDateTime& dateTime )
{
	return DateTime( Date( dateTime.date().year(), dateTime.date().month(), dateTime.date().day(), dateTime.date().dayOfWeek() ),
					 Time( dateTime.time().hour(), dateTime.time().minute(), dateTime.time().second(), dateTime.time().msec() ) );
}

QDateTime MarvieController::toQtDateTime( const DateTime& dateTime )
{
	return QDateTime( QDate( dateTime.date().year(), dateTime.date().month(), dateTime.date().day() ),
					  QTime( dateTime.time().hour(), dateTime.time().min(), dateTime.time().sec(), dateTime.time().msec() ) );
}

QString MarvieController::printDateTime( const QDateTime& dateTime )
{
	return QString( "%1.%2.%3/%4:%5:%6" ).arg( dateTime.date().day(), 2, 10, QChar( '0' ) )
										 .arg( dateTime.date().month(), 2, 10, QChar( '0' ) )
										 .arg( dateTime.date().year(), 2, 10, QChar( '0' ) )
										 .arg( dateTime.time().hour(), 2, 10, QChar( '0' ) )
										 .arg( dateTime.time().minute(), 2, 10, QChar( '0' ) )
										 .arg( dateTime.time().second(), 2, 10, QChar( '0' ) );
}

QString MarvieController::saveCanonicalXML( const QDomDocument& doc, int indent ) const
{
	QString xmlData;
	QXmlStreamWriter stream( &xmlData );
	stream.setAutoFormatting( true );
	stream.setAutoFormattingIndent( indent );
	stream.writeStartDocument();

	QDomNode root = doc.documentElement();
	while( !root.isNull() )
	{
		writeDomNodeCanonically( stream, root );
		if( stream.hasError() )
			break;
		root = root.nextSibling();
	}

	stream.writeEndDocument();
	if( stream.hasError() )
		return "";

	return xmlData;
}

void MarvieController::writeDomNodeCanonically( QXmlStreamWriter &stream, const QDomNode &domNode ) const
{
	if( stream.hasError() )
		return;

	if( domNode.isElement() )
	{
		const QDomElement domElement = domNode.toElement();
		if( !domElement.isNull() )
		{
			stream.writeStartElement( domElement.tagName() );

			if( domElement.hasAttributes() )
			{
				QMap<QString, QString> attributes;
				const QDomNamedNodeMap attributeMap = domElement.attributes();
				for( int i = 0; i < attributeMap.count(); ++i )
				{
					const QDomNode attribute = attributeMap.item( i );
					attributes.insert( attribute.nodeName(), attribute.nodeValue() );
				}

				QMap<QString, QString>::const_iterator i = attributes.constBegin();
				while( i != attributes.constEnd() )
				{
					stream.writeAttribute( i.key(), i.value() );
					++i;
				}
			}

			if( domElement.hasChildNodes() )
			{
				QDomNode elementChild = domElement.firstChild();
				while( !elementChild.isNull() )
				{
					writeDomNodeCanonically( stream, elementChild );
					elementChild = elementChild.nextSibling();
				}
			}

			stream.writeEndElement();
		}
	}
	else if( domNode.isComment() )
		stream.writeComment( domNode.nodeValue() );
	else if( domNode.isText() )
		stream.writeCharacters( domNode.nodeValue() );
}

void MarvieController::XmlMessageHandler::handleMessage( QtMsgType type, const QString &description, const QUrl &identifier, const QSourceLocation &sourceLocation )
{
	Q_UNUSED( type );
	Q_UNUSED( identifier );

	this->description = description;
	this->sourceLocation = sourceLocation;
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

MarvieController::MemStatistics::MemStatistics()
{
	QVBoxLayout *verticalLayout_3;
	QLabel *label;
	QVBoxLayout *verticalLayout;
	QHBoxLayout *horizontalLayout;
	QLabel *label_2;
	QSpacerItem *horizontalSpacer;	
	QHBoxLayout *horizontalLayout_2;
	QLabel *label_3;
	QSpacerItem *horizontalSpacer_2;
	QHBoxLayout *horizontalLayout_3;
	QLabel *label_4;
	QSpacerItem *horizontalSpacer_3;
	QHBoxLayout *horizontalLayout_4;
	QLabel *label_5;
	QSpacerItem *horizontalSpacer_4;
	QLabel *label_10;
	QVBoxLayout *verticalLayout_2;
	QHBoxLayout *horizontalLayout_5;
	QLabel *label_6;
	QSpacerItem *horizontalSpacer_5;
	QHBoxLayout *horizontalLayout_6;
	QLabel *label_7;
	QSpacerItem *horizontalSpacer_6;
	QHBoxLayout *horizontalLayout_7;
	QLabel *label_8;
	QSpacerItem *horizontalSpacer_7;
	QHBoxLayout *horizontalLayout_8;
	QLabel *label_9;
	QSpacerItem *horizontalSpacer_8;

	resize( 135, 260 );
	setWindowFlag( Qt::WindowFlags::enum_type::Popup );
	setFrameShape( QFrame::Shape::StyledPanel );
	setFrameShadow( QFrame::Shadow::Raised );

	verticalLayout_3 = new QVBoxLayout( this );
	verticalLayout_3->setObjectName( QStringLiteral( "verticalLayout_3" ) );
	verticalLayout_3->setContentsMargins( 4, 4, 4, 4 );
	label = new QLabel( this );
	label->setObjectName( QStringLiteral( "label" ) );
	verticalLayout_3->addWidget( label );

	verticalLayout = new QVBoxLayout();
	verticalLayout->setObjectName( QStringLiteral( "verticalLayout" ) );
	verticalLayout->setContentsMargins( 20, -1, -1, -1 );
	horizontalLayout = new QHBoxLayout();
	horizontalLayout->setSpacing( 0 );
	horizontalLayout->setObjectName( QStringLiteral( "horizontalLayout" ) );

	label_2 = new QLabel( this );
	label_2->setObjectName( QStringLiteral( "label_2" ) );
	horizontalLayout->addWidget( label_2 );

	horizontalSpacer = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
	horizontalLayout->addItem( horizontalSpacer );

	gMemMaxUsedLabel = new QLabel( this );
	gMemMaxUsedLabel->setObjectName( QStringLiteral( "gMemMaxUsedLabel" ) );
	horizontalLayout->addWidget( gMemMaxUsedLabel );

	verticalLayout->addLayout( horizontalLayout );

	horizontalLayout_2 = new QHBoxLayout();
	horizontalLayout_2->setSpacing( 0 );
	horizontalLayout_2->setObjectName( QStringLiteral( "horizontalLayout_2" ) );

	label_3 = new QLabel( this );
	label_3->setObjectName( QStringLiteral( "label_3" ) );
	horizontalLayout_2->addWidget( label_3 );

	horizontalSpacer_2 = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
	horizontalLayout_2->addItem( horizontalSpacer_2 );

	gMemHeapLabel = new QLabel( this );
	gMemHeapLabel->setObjectName( QStringLiteral( "gMemHeapLabel" ) );
	horizontalLayout_2->addWidget( gMemHeapLabel );

	verticalLayout->addLayout( horizontalLayout_2 );

	horizontalLayout_3 = new QHBoxLayout();
	horizontalLayout_3->setSpacing( 0 );
	horizontalLayout_3->setObjectName( QStringLiteral( "horizontalLayout_3" ) );

	label_4 = new QLabel( this );
	label_4->setObjectName( QStringLiteral( "label_4" ) );
	horizontalLayout_3->addWidget( label_4 );

	horizontalSpacer_3 = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
	horizontalLayout_3->addItem( horizontalSpacer_3 );

	gMemFragmentsLabel = new QLabel( this );
	gMemFragmentsLabel->setObjectName( QStringLiteral( "gMemFragmentsLabel" ) );
	horizontalLayout_3->addWidget( gMemFragmentsLabel );

	verticalLayout->addLayout( horizontalLayout_3 );

	horizontalLayout_4 = new QHBoxLayout();
	horizontalLayout_4->setSpacing( 0 );
	horizontalLayout_4->setObjectName( QStringLiteral( "horizontalLayout_4" ) );

	label_5 = new QLabel( this );
	label_5->setObjectName( QStringLiteral( "label_5" ) );
	horizontalLayout_4->addWidget( label_5 );

	horizontalSpacer_4 = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
	horizontalLayout_4->addItem( horizontalSpacer_4 );

	gMemLargestLabel = new QLabel( this );
	gMemLargestLabel->setObjectName( QStringLiteral( "gMemLargestLabel" ) );
	horizontalLayout_4->addWidget( gMemLargestLabel );

	verticalLayout->addLayout( horizontalLayout_4 );
	verticalLayout_3->addLayout( verticalLayout );

	label_10 = new QLabel( this );
	label_10->setObjectName( QStringLiteral( "label_10" ) );
	verticalLayout_3->addWidget( label_10 );

	verticalLayout_2 = new QVBoxLayout();
	verticalLayout_2->setObjectName( QStringLiteral( "verticalLayout_2" ) );
	verticalLayout_2->setContentsMargins( 20, -1, -1, -1 );
	horizontalLayout_5 = new QHBoxLayout();
	horizontalLayout_5->setSpacing( 0 );
	horizontalLayout_5->setObjectName( QStringLiteral( "horizontalLayout_5" ) );

	label_6 = new QLabel( this );
	label_6->setObjectName( QStringLiteral( "label_6" ) );
	horizontalLayout_5->addWidget( label_6 );

	horizontalSpacer_5 = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
	horizontalLayout_5->addItem( horizontalSpacer_5 );

	ccMemMaxUsedLabel = new QLabel( this );
	ccMemMaxUsedLabel->setObjectName( QStringLiteral( "ccMemMaxUsedLabel" ) );
	horizontalLayout_5->addWidget( ccMemMaxUsedLabel );

	verticalLayout_2->addLayout( horizontalLayout_5 );

	horizontalLayout_6 = new QHBoxLayout();
	horizontalLayout_6->setSpacing( 0 );
	horizontalLayout_6->setObjectName( QStringLiteral( "horizontalLayout_6" ) );

	label_7 = new QLabel( this );
	label_7->setObjectName( QStringLiteral( "label_7" ) );
	horizontalLayout_6->addWidget( label_7 );

	horizontalSpacer_6 = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
	horizontalLayout_6->addItem( horizontalSpacer_6 );

	ccMemHeapLabel = new QLabel( this );
	ccMemHeapLabel->setObjectName( QStringLiteral( "ccMemHeapLabel" ) );
	horizontalLayout_6->addWidget( ccMemHeapLabel );

	verticalLayout_2->addLayout( horizontalLayout_6 );

	horizontalLayout_7 = new QHBoxLayout();
	horizontalLayout_7->setSpacing( 0 );
	horizontalLayout_7->setObjectName( QStringLiteral( "horizontalLayout_7" ) );

	label_8 = new QLabel( this );
	label_8->setObjectName( QStringLiteral( "label_8" ) );
	horizontalLayout_7->addWidget( label_8 );

	horizontalSpacer_7 = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
	horizontalLayout_7->addItem( horizontalSpacer_7 );

	ccMemFragmentsLabel = new QLabel( this );
	ccMemFragmentsLabel->setObjectName( QStringLiteral( "ccMemFragmentsLabel" ) );
	horizontalLayout_7->addWidget( ccMemFragmentsLabel );

	verticalLayout_2->addLayout( horizontalLayout_7 );

	horizontalLayout_8 = new QHBoxLayout();
	horizontalLayout_8->setSpacing( 0 );
	horizontalLayout_8->setObjectName( QStringLiteral( "horizontalLayout_8" ) );

	label_9 = new QLabel( this );
	label_9->setObjectName( QStringLiteral( "label_9" ) );
	horizontalLayout_8->addWidget( label_9 );

	horizontalSpacer_8 = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
	horizontalLayout_8->addItem( horizontalSpacer_8 );

	ccMemLargestLabel = new QLabel( this );
	ccMemLargestLabel->setObjectName( QStringLiteral( "ccMemLargestLabel" ) );
	horizontalLayout_8->addWidget( ccMemLargestLabel );

	verticalLayout_2->addLayout( horizontalLayout_8 );
	verticalLayout_3->addLayout( verticalLayout_2 );

	label->setText( "General memory:" );
	label_2->setText( "Max used - " );
	gMemMaxUsedLabel->setText( "0B" );
	label_3->setText( "Heap - " );
	gMemHeapLabel->setText( "0B" );
	label_4->setText( "Fragments - " );
	gMemFragmentsLabel->setText( "0" );
	label_5->setText( "Largest - " );
	gMemLargestLabel->setText( "0B" );
	label_10->setText( "Core coupled memory:" );
	label_6->setText( "Max used - " );
	ccMemMaxUsedLabel->setText( "0B" );
	label_7->setText( "Heap - " );
	ccMemHeapLabel->setText( "0B" );
	label_8->setText( "Fragments - " );
	ccMemFragmentsLabel->setText( "0" );
	label_9->setText( "Largest - " );
	ccMemLargestLabel->setText( "0B" );
}

void MarvieController::MemStatistics::setStatistics( uint32_t gmemMaxUsed, uint32_t gmemHeap, uint32_t gmemFragments, uint32_t gmemLargestFragment, uint32_t ccmemMaxUsed, uint32_t ccmemHeap, uint32_t ccmemFragments, uint32_t ccmemLargestFragment )
{
	gMemMaxUsedLabel->setText( QString( "%1" ).arg( gmemMaxUsed ) );
	gMemHeapLabel->setText( QString( "%1" ).arg( gmemHeap ) );
	gMemFragmentsLabel->setText( QString( "%1" ).arg( gmemFragments ) );
	gMemLargestLabel->setText( QString( "%1" ).arg( gmemLargestFragment ) );

	ccMemMaxUsedLabel->setText( QString( "%1" ).arg( ccmemMaxUsed ) );
	ccMemHeapLabel->setText( QString( "%1" ).arg( ccmemHeap ) );
	ccMemFragmentsLabel->setText( QString( "%1" ).arg( ccmemFragments ) );
	ccMemLargestLabel->setText( QString( "%1" ).arg( ccmemLargestFragment ) );
}

void MarvieController::MemStatistics::show( QPoint point )
{
	auto rect = geometry();
	rect.moveCenter( point + QPoint( 0, height() / 2 ) );
	setGeometry( rect );
	QFrame::show();
	setFocus();
}

void MarvieController::MemStatistics::focusOutEvent( QFocusEvent *event )
{
	hide();
}

MarvieController::SdStatistics::SdStatistics()
{
	ui.setupUi( this );

	resize( 100, 75 );
	setWindowFlag( Qt::WindowFlags::enum_type::Popup );
	setFrameShape( QFrame::Shape::StyledPanel );
	setFrameShadow( QFrame::Shadow::Raised );

	setStatistics( 0, 0, 0 );
}

void MarvieController::SdStatistics::setStatistics( uint64_t totalSize, uint64_t freeSize, uint64_t logSize )
{
	ui.totalSizeLabel->setText( printSize( totalSize ) );
	ui.freeSizeLabel->setText( printSize( freeSize ) );
	ui.logSizeLabel->setText( printSize( logSize ) );
}

void MarvieController::SdStatistics::show( QPoint point )
{
	auto rect = geometry();
	rect.moveCenter( point + QPoint( 0, height() / 2 ) );
	setGeometry( rect );
	QFrame::show();
	setFocus();
}

void MarvieController::SdStatistics::focusOutEvent( QFocusEvent *event )
{
	hide();
}

QString MarvieController::SdStatistics::printSize( uint64_t size )
{
	if( size < 1024 )
		return QString( "%1 B" ).arg( size );
	if( size < 1024 * 1024 )
		return QString( "%1.%2 KB" ).arg( size / 1024 ).arg( ( ( size + 1 ) % 1024 ) * 100 / 1024, 2, 10, QChar( '0' ) );
	if( size < 1024 * 1024 * 1024 )
		return QString( "%1.%2 MB" ).arg( size / 1024 / 1024 ).arg( ( ( size + 1 ) % ( 1024 * 1024 ) ) * 100 / ( 1024 * 1024 ), 2, 10, QChar( '0' ) );
	return QString( "%1.%2 GB" ).arg( size / 1024 / 1024 / 1024 ).arg( ( ( size + 1 ) % ( 1024 * 1024 * 1024 ) ) * 100 / ( 1024 * 1024 * 1024 ), 2, 10, QChar( '0' ) );
}