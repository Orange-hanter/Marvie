#include "RemoteTerminalTest.h"
#include <QtWidgets/QApplication>
#include <QFontDatabase>

int main( int argc, char *argv[] )
{
	QApplication a( argc, argv );
	QFontDatabase::addApplicationFont( ":/RemoteTerminalTest/Droid Sans Mono.ttf" );
	a.setStyle( "Fusion" );
	RemoteTerminalTest w;
	w.show();
	return a.exec();
}
