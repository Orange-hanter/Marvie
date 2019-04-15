#include "DeviceFirmwareInfoWidget.h"

#ifdef USE_FRAMELESS_WINDOW 
DeviceFirmwareInfoWidget::DeviceFirmwareInfoWidget( QWidget *parent ) : FramelessWidget( parent )
{
	QWidget* centralWindget = new QWidget;
	ui.setupUi( centralWindget );
	setPalette( centralWindget->palette() );
	setCentralWidget( centralWindget );
	windowButtons()->setButtonColor( ButtonType::Minimize | ButtonType::Maximize | ButtonType::Close, QColor( 100, 100, 100 ) );
	setTitleText( "Device info" );
#else
DeviceFirmwareInfoWidget::DeviceFirmwareInfoWidget( QWidget *parent ) : QWidget( parent )
{
	ui.setupUi( this );
	ui.lineForFramelessWindow->hide();
	setWindowTitle( "Device info" );
#endif
	setMinimumSize( QSize( 400, 300 ) );
	QRect mainWindowRect( 0, 0, 400, 300 );
	setGeometry( mainWindowRect );

	ui.treeWidget->header()->resizeSection( 0, 250 );

	ui.treeWidget->insertTopLevelItem( 0, new QTreeWidgetItem() );
	ui.treeWidget->topLevelItem( 0 )->setData( 0, Qt::DisplayRole, "Sensors" );

	ui.treeWidget->insertTopLevelItem( 0, new QTreeWidgetItem() );
	ui.treeWidget->topLevelItem( 0 )->setData( 0, Qt::DisplayRole, "Model" );

	ui.treeWidget->insertTopLevelItem( 0, new QTreeWidgetItem() );
	ui.treeWidget->topLevelItem( 0 )->setData( 0, Qt::DisplayRole, "Bootloader" );

	ui.treeWidget->insertTopLevelItem( 0, new QTreeWidgetItem() );
	ui.treeWidget->topLevelItem( 0 )->setData( 0, Qt::DisplayRole, "Firmware" );
}

DeviceFirmwareInfoWidget::~DeviceFirmwareInfoWidget()
{

}

void DeviceFirmwareInfoWidget::setFirmwareVersion( QString version )
{
	ui.treeWidget->topLevelItem( FirmwareRow )->setData( 1, Qt::DisplayRole, version );
}

void DeviceFirmwareInfoWidget::setBootloaderVersion( QString version )
{
	ui.treeWidget->topLevelItem( BootloaderRow )->setData( 1, Qt::DisplayRole, version );
}

void DeviceFirmwareInfoWidget::setModelName( QString name )
{
	ui.treeWidget->topLevelItem( ModelRow )->setData( 1, Qt::DisplayRole, name );
}

void DeviceFirmwareInfoWidget::setSupportedSensorList( QStringList list )
{
	qDeleteAll( ui.treeWidget->topLevelItem( SensorsRow )->takeChildren() );
	for( auto& sensorName : list )
	{
		auto item = new QTreeWidgetItem();
		item->setData( 0, Qt::DisplayRole, sensorName );
		ui.treeWidget->topLevelItem( SensorsRow )->addChild( item );
	}
}

void DeviceFirmwareInfoWidget::clear()
{
	ui.treeWidget->topLevelItem( FirmwareRow )->setData( 1, Qt::DisplayRole, "" );
	ui.treeWidget->topLevelItem( BootloaderRow )->setData( 1, Qt::DisplayRole, "" );
	ui.treeWidget->topLevelItem( ModelRow )->setData( 1, Qt::DisplayRole, "" );
	qDeleteAll( ui.treeWidget->topLevelItem( SensorsRow )->takeChildren() );
}
