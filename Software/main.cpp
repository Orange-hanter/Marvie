#include "MarvieController.h"
#include <QtWidgets/QApplication>
#include <time.h>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	a.setStyle( "Fusion" );
	QLocale::setDefault( QLocale( QLocale::English, QLocale::UnitedKingdom ) );
	qsrand( time( nullptr ) );
	MarvieController w;
	w.show();
	return a.exec();
}
