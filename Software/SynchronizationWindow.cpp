#include "SynchronizationWindow.h"
#include <QVBoxLayout>
#include <QCloseEvent>

SynchronizationWindow::SynchronizationWindow( QWidget* mainWindow ) : QDialog( nullptr ), mainWindow( mainWindow )
{
	setWindowFlags( Qt::FramelessWindowHint | Qt::SubWindow );
	QPalette pal = palette();
	pal.setColor( QPalette::Window, Qt::white );
	setPalette( pal );
	movie.setFileName( ":/MarvieController/Animations/loading.gif" );
	QVBoxLayout* layout = new QVBoxLayout( this );
	layout->setContentsMargins( 0, 0, 0, 0 );
	QFrame* frame = new QFrame;
	frame->setFrameShape( QFrame::StyledPanel );
	frame->setFrameShadow( QFrame::Raised );
	layout->addWidget( frame );
	layout = new QVBoxLayout( frame );
	layout->setContentsMargins( 0, 0, 0, 0 );
	label = new QLabel( "" );
	label->setScaledContents( true );
	layout->addWidget( label );

	QObject::connect( &movie, &QMovie::frameChanged, [this]() { frameChanged(); } );
}

SynchronizationWindow::~SynchronizationWindow()
{

}

void SynchronizationWindow::show()
{
	QRect windowRect( 0, 0, 256, 256 );
	windowRect.moveCenter( mainWindow->mapToGlobal( mainWindow->rect().center() ) );
	setGeometry( windowRect );

	movie.start();
	//exec();
	setWindowModality( Qt::ApplicationModal );
	QDialog::show();
}

void SynchronizationWindow::hide()
{
	movie.stop();
	QDialog::hide();
}

void SynchronizationWindow::closeEvent( QCloseEvent * event )
{
	event->ignore();
}

void SynchronizationWindow::frameChanged()
{
	label->setPixmap( movie.currentPixmap() );
}