#include "FirmwareUpdater.h"
#include <QDomDocument>
#include <QHostAddress>
#include <QFileDialog>
#include <QXmlSchema>
#include <QXmlSchemaValidator>
#include <QAbstractMessageHandler>
#include <QSettings>
#include <QMenu>

FirmwareUpdater::FirmwareUpdater( QWidget *parent ) : QWidget( parent )
{
	ui.setupUi( this );

	ui.treeView->setModel( &hostListModel );
	ui.treeView->setColumnWidth( 0, 235 );
	updateCompleted = false;

	menu = new QMenu( this );
	menu->addAction( "Clear" );
	menu->addSeparator();
	menu->addAction( "Expand all" );
	menu->addAction( "Collapse all" );

	QObject::connect( ui.openFolderButton, &QToolButton::released, this, &FirmwareUpdater::openFolderButtonClicked );
	QObject::connect( ui.addDevicesButton, &QToolButton::released, this, &FirmwareUpdater::addDevicesButtonClicked );
	QObject::connect( ui.startButton, &QToolButton::released, this, &FirmwareUpdater::startButtonClicked );

	QObject::connect( ui.treeView, &QTreeView::customContextMenuRequested, this, &FirmwareUpdater::treeViewMenuRequested );
	QObject::connect( menu, &QMenu::triggered, this, &FirmwareUpdater::menuActionTriggered );

	QObject::connect( &groupUpdater, &GroupUpdater::hostStateChanged, this, &FirmwareUpdater::hostStateChanged );
	QObject::connect( &groupUpdater, &GroupUpdater::hostVersions, this, &FirmwareUpdater::hostVersions );
	QObject::connect( &groupUpdater, &GroupUpdater::hostProgress, this, &FirmwareUpdater::hostProgress );
	QObject::connect( &groupUpdater, &GroupUpdater::hostUpdated, this, &FirmwareUpdater::hostUpdated );
	QObject::connect( &groupUpdater, &GroupUpdater::allHostUpdated, this, &FirmwareUpdater::allHostUpdated );
}

void FirmwareUpdater::openFolderButtonClicked()
{
	QSettings setting( "settings.ini", QSettings::Format::IniFormat );
	QString path = QFileDialog::getExistingDirectory( this, "Open folder", setting.value( "firmwareBinariesFolder", QDir::currentPath() ).toString() );
	if( path.isEmpty() )
		return;
	setting.setValue( "firmwareBinariesFolder", QDir::current().relativeFilePath( path ) );

	auto fileNameList = QDir( path ).entryList( QDir::Filter::Files, QDir::SortFlag::Name );
	QSet< QString > versionSet;

	auto filteredFileNameList = fileNameList.filter( QRegExp( "^bootloader.*\\.bin$" ) );
	bootloaderMap.clear();
	for( auto& e : filteredFileNameList )
	{
		QFile file( path + "/" + e );
		file.open( QIODevice::ReadOnly );
		if( file.size() < sizeof( "__MARVIE_BOOTLOADER__" ) - 1 )
			continue;
		if( !file.seek( file.size() - sizeof( "__MARVIE_BOOTLOADER__" ) + 1 ) ||
			file.readAll() != "__MARVIE_BOOTLOADER__" ||
			!file.seek( 0 ) )
			continue;
		auto data = file.readAll();
		int begin = data.lastIndexOf( "__", data.size() - sizeof( "__MARVIE_BOOTLOADER__" ) ) + 2;
		
		BinaryFileInfo info;
		info.version = data.mid( begin, data.indexOf( "__", begin ) - begin );
		info.fullPath = path + "/" + e;
		QString model = modelName( info.version );
		QString baseVersion = info.version.left( info.version.size() - model.size() );
		bootloaderMap[baseVersion].append( info );
		versionSet.insert( baseVersion );
	}
	auto list = versionSet.toList();
	auto sortFunc = []( const QString& a, const QString& b )
	{
		auto listA = a.split( '.' );
		auto listB = b.split( '.' );
		for( int i = 0; i < listA.size(); ++i )
		{
			if( listA[i] == listB[i] )
				continue;
			return listA[i] < listB[i];
		}
		return false;
	};
	qSort( list.begin(), list.end(), sortFunc );
	list.push_front( "Skip" );
	ui.bootloaderComboBox->clear();
	ui.bootloaderComboBox->addItems( list );
	ui.bootloaderComboBox->setCurrentIndex( list.size() - 1 );

	filteredFileNameList = fileNameList.filter( QRegExp( "^firmware.*\\.bin$" ) );
	firmwareMap.clear();
	versionSet.clear();
	for( auto& e : filteredFileNameList )
	{
		QFile file( path + "/" + e );
		file.open( QIODevice::ReadOnly );
		if( file.size() < sizeof( "__MARVIE_FIRMWARE__" ) - 1 )
			continue;
		if( !file.seek( file.size() - sizeof( "__MARVIE_FIRMWARE__" ) + 1 ) ||
			file.readAll() != "__MARVIE_FIRMWARE__" ||
			!file.seek( 0 ) )
			continue;
		auto data = file.readAll();
		int begin = data.lastIndexOf( "__", data.size() - sizeof( "__MARVIE_FIRMWARE__" ) ) + 2;

		BinaryFileInfo info;
		info.version = data.mid( begin, data.indexOf( "__", begin ) - begin );
		info.fullPath = path + "/" + e;
		QString model = modelName( info.version );
		QString baseVersion = info.version.left( info.version.size() - model.size() );
		firmwareMap[baseVersion].append( info );
		versionSet.insert( baseVersion );
	}
	list = versionSet.toList();
	qSort( list.begin(), list.end(), sortFunc );
	list.push_front( "Skip" );
	ui.firmwareComboBox->clear();
	ui.firmwareComboBox->addItems( list );
	ui.firmwareComboBox->setCurrentIndex( list.size() - 1 );
}

void FirmwareUpdater::addDevicesButtonClicked()
{
	QSettings setting( "settings.ini", QSettings::Format::IniFormat );
	QStringList list = QFileDialog::getOpenFileNames( this, "Select host list", setting.value( "hostListPath", QDir::currentPath() ).toString() );
	if( list.isEmpty() )
		return;
	setting.setValue( "hostListPath", QDir::current().relativeFilePath( list[0] ) );

	class XmlMessageHandler : public QAbstractMessageHandler
	{
		virtual void handleMessage( QtMsgType type, const QString &description, const QUrl &identifier, const QSourceLocation &sourceLocation )
		{
		}
	} xmlMessageHandler;

	QXmlSchema hostListScheme;
	hostListScheme.setMessageHandler( &xmlMessageHandler );
	hostListScheme.load( QUrl::fromLocalFile( ":/FirmwareUpdater/Xml/schemas/HostList.xsd" ) );
	QXmlSchemaValidator hostListValidator( hostListScheme );

	for( auto& fileName : list )
	{
		QFile file( fileName );
		file.open( QIODevice::ReadOnly );
		QByteArray data = file.readAll();
		file.close();

		if( !hostListValidator.validate( data, QUrl::fromLocalFile( "GG42" ) ) )
			continue;

		QDomDocument doc;
		if( !doc.setContent( data ) )
			continue;

		auto c0 = doc.firstChildElement( "hostList" ).firstChildElement( "group" );
		while( !c0.isNull() )
		{
			HostListModel::HostGroup hostGroup;
			hostGroup.title = c0.attribute( "title", "Untitled" );
			auto c1 = c0.firstChildElement( "host" );
			while( !c1.isNull() )
			{
				HostListModel::Host host;
				host.addr = c1.attribute( "ipAddress" );
				host.password = c1.attribute( "password" );
				host.info = c1.attribute( "info" );
				host.progress = 0.0f;
				hostGroup.hostList.append( host );

				c1 = c1.nextSiblingElement( "host" );
			}
			hostListModel.appendHostGroup( hostGroup );

			c0 = c0.nextSiblingElement( "group" );
		}
	}
}

void FirmwareUpdater::startButtonClicked()
{
	if( groupUpdater.isStarted() )
	{
		groupUpdater.stop();
		hostListModel.setEditMode( true );
		ui.startButton->setIcon( QIcon( ":/FirmwareUpdater/icons/icons8-play-60.png" ) );
		ui.startButton->setToolTip( "Start update" );
		return;
	}
	else if( updateCompleted )
	{
		hostListModel.setEditMode( true );
		ui.startButton->setIcon( QIcon( ":/FirmwareUpdater/icons/icons8-play-60.png" ) );
		ui.startButton->setToolTip( "Start update" );
		updateCompleted = false;
		return;
	}

	hostListModel.resetHostsStatus();
	auto& list = hostListModel.hostGroupList();
	int n = 0, m = 0, h = 0;
	for( auto i = list.begin(); i != list.end(); ++i )
		n += i->hostList.size();
	hostGroupIndexMap = QVector< QPair< int, int > >( n );
	QVector< GroupUpdater::Host > hostList( n );
	n = 0;
	for( auto i = list.begin(); i != list.end(); ++i )
	{
		h = 0;
		for( auto i2 = i->hostList.begin(); i2 != i->hostList.end(); ++i2 )
		{
			GroupUpdater::Host host;
			host.addr = i2->addr;
			host.password = i2->password;
			hostList[n] = host;
			hostGroupIndexMap[n++] = QPair< int, int >( m, h++ );
		}
		++m;
	}
	groupUpdater.clear();
	groupUpdater.setHostList( std::move( hostList ) );
	groupUpdater.setMaxConnections( ui.maxConnectionsSpinBox->value() );
	groupUpdater.setUpdatePolicy( ui.updatePolicyComboBox->currentText() == "IfNewer" ? GroupUpdater::UpdatePolicy::IfNewer : GroupUpdater::UpdatePolicy::Unconditional );
	bool flag = false;
	if( ui.firmwareComboBox->currentText() != "Skip" )
	{
		auto& list = firmwareMap[ui.firmwareComboBox->currentText()];
		for( auto& e : list )
		{
			QFile file( e.fullPath );
			file.open( QIODevice::ReadOnly );
			groupUpdater.addFirmware( e.version, file.readAll() );
		}
		flag = true;
	}
	if( ui.bootloaderComboBox->currentText() != "Skip" )
	{
		auto& list = bootloaderMap[ui.bootloaderComboBox->currentText()];
		for( auto& e : list )
		{
			QFile file( e.fullPath );
			file.open( QIODevice::ReadOnly );
			groupUpdater.addBootloader( e.version, file.readAll() );
		}
		flag = true;
	}
	if( flag )
	{
		hostListModel.setEditMode( false );
		ui.startButton->setIcon( QIcon( ":/FirmwareUpdater/icons/icons8-stop-48.png" ) );
		ui.startButton->setToolTip( "Stop update" );

		groupUpdater.start();
	}
}

void FirmwareUpdater::treeViewMenuRequested( const QPoint& pos )
{
	menu->popup( ui.treeView->viewport()->mapToGlobal( pos ) );
}

void FirmwareUpdater::menuActionTriggered( QAction* action )
{
	if( action->text() == "Clear" )
		hostListModel.clear();
	else if( action->text() == "Expand all" )
		ui.treeView->expandAll();
	else if( action->text() == "Collapse all" )
		ui.treeView->collapseAll();
}

void FirmwareUpdater::hostStateChanged( int index, GroupUpdater::HostState state )
{
	auto pair = hostGroupIndexMap[index];
	hostListModel.setHostState( pair.first, pair.second, toString( state ) );
}

void FirmwareUpdater::hostVersions( int index, QString firmwareVersion, QString bootloaderVersion )
{
	auto pair = hostGroupIndexMap[index];
	hostListModel.setHostVersions( pair.first, pair.second, firmwareVersion, bootloaderVersion );
}

void FirmwareUpdater::hostProgress( int index, float progress )
{
	auto pair = hostGroupIndexMap[index];
	hostListModel.setHostProgress( pair.first, pair.second, progress * 100 );
}

void FirmwareUpdater::hostUpdated( int index )
{

}

void FirmwareUpdater::allHostUpdated()
{
	updateCompleted = true;
}

QString FirmwareUpdater::modelName( QString version )
{
	int i;
	for( i = version.size() - 1; version[i] < '0' || version[i] > '9'; --i )
		;
	return version.right( version.size() - i - 1 );
}

QString FirmwareUpdater::toString( GroupUpdater::HostState state )
{
	switch( state )
	{
	case GroupUpdater::HostState::InQueue:
		return "InQueue";
	case GroupUpdater::HostState::Connecting:
		return "Connecting";
	case GroupUpdater::HostState::Authorization:
		return "Authorization";
	case GroupUpdater::HostState::FirmwareUploading:
		return "FirmwareUploading";
	case GroupUpdater::HostState::BootloaderUploading:
		return "BootloaderUploading";
	case GroupUpdater::HostState::Restarting:
		return "Restarting";
	case GroupUpdater::HostState::Waiting:
		return "Waiting";
	case GroupUpdater::HostState::Checking:
		return "Checking";
	case GroupUpdater::HostState::Complete:
		return "Complete";
	case GroupUpdater::HostState::AuthError:
		return "AuthError";
	case GroupUpdater::HostState::VersionError:
		return "VersionError";
	case GroupUpdater::HostState::ServerVersionError:
		return "ServerVersionError";
	case GroupUpdater::HostState::NetworkError:
		return "NetworkError";
	default:
		return "Unknown";
	}
}
