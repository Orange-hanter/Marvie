#include "MarvieController.h"
#include <QtWidgets/QApplication>
#include <time.h>
#include "MonitoringLog.h"

int main( int argc, char *argv[] )
{
 	QApplication a( argc, argv );
 	a.setStyle( "Fusion" );
 	QLocale::setDefault( QLocale( QLocale::English, QLocale::UnitedKingdom ) );
 	qsrand( time( nullptr ) );

	/*MonitoringLog monitoringLog;
	QList< MonitoringLog::SensorDesc > list;
	list.append( MonitoringLog::SensorDesc{ "TestSensorB", 4 } );
	list.append( MonitoringLog::SensorDesc{ "TestSRSensorA", 4 } );
	monitoringLog.setAvailableSensors( list );
	monitoringLog.open( "C:/Users/User/Desktop/Log/BinaryLog" );
	auto dayGroup = monitoringLog.nameGroup( "AI" ).dayGroup( QDate( 2018, 9, 21 ) );
	monitoringLog.nameGroup( "DI" ).dayGroup( QDate( 2018, 9, 21 ) );
	monitoringLog.nameGroup( "3.TestSensorB" ).dayGroup( QDate( 2018, 9, 21 ) );
	monitoringLog.nameGroup( "AI" ).dayGroup( QDate( 2018, 9, 21 ) ).nearestEntry( QTime::fromMSecsSinceStartOfDay( 46454279 + 1 ) );
	monitoringLog.nameGroup( "DI" ).dayGroup( QDate( 2018, 9, 21 ) ).timestamps();*/

 	MarvieController w;
 	w.show();
 	return a.exec();

	/*QStandardItemModel model( 3, 1 ); // 3 rows, 1 col
	for( int r = 0; r < 3; ++r )
	{
		QStandardItem* item = new QStandardItem( QString( "Item %0" ).arg( r ) );

		item->setFlags( Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable );
		item->setData( Qt::Unchecked, Qt::CheckStateRole );

		model.setItem( r, 0, item );
	}

	QComboBox* combo = new QComboBox();
	combo->setModel( &model );

	QListView* list = new QListView();
	list->setModel( &model );

	QTableView* table = new QTableView();
	table->setModel( &model );

	QWidget container;
	QVBoxLayout* containerLayout = new QVBoxLayout();
	container.setLayout( containerLayout );
	containerLayout->addWidget( combo );
	containerLayout->addWidget( list );
	containerLayout->addWidget( table );

	container.show();

	return a.exec();*/
}