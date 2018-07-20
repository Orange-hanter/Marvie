#include "VPortTileWidget.h"
#include <QDesktopWidget>
#include <QApplication>
#include <QHeaderView>
#include <QEvent>

VPortTileWidget::VPortTileWidget( QWidget* parent /*= nullptr */ ) : QWidget( parent )
{
	QVBoxLayout* verticalLayout = new QVBoxLayout( this );
	verticalLayout->setContentsMargins( 6, 6, 6, 6 );
	QHBoxLayout* horizontalLayout = new QHBoxLayout();
	horizontalLayout->setSpacing( 2 );

	vPortLabel = new QLabel( "VPort0", this );
	QFont font;
	font.setPointSize( 12 );
	font.setWeight( 50 );
	vPortLabel->setFont( font );
	horizontalLayout->addWidget( vPortLabel );

	QSpacerItem* horizontalSpacer = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
	horizontalLayout->addItem( horizontalSpacer );

	statusLabel = new QLabel( this );
	statusLabel->setMinimumSize( QSize( 25, 25 ) );
	statusLabel->setMaximumSize( QSize( 25, 25 ) );
	statusLabel->setPixmap( QPixmap( ":/MarvieController/icons/icons8-checkmark-64.png" ) );
	statusLabel->setScaledContents( true );
	horizontalLayout->addWidget( statusLabel );

	QToolButton* button = new QToolButton( this );
	button->setMinimumSize( QSize( 25, 25 ) );
	QIcon icon;
	icon.addFile( ":/MarvieController/icons/icons8-chevron-right-filled-50.png", QSize(), QIcon::Normal, QIcon::Off );
	button->setIcon( icon );
	button->setAutoRaise( true );
	horizontalLayout->addWidget( button );
	verticalLayout->addLayout( horizontalLayout );

	sensorNameLabel = new QLabel( "Unknown", this );
	sensorNameLabel->setAlignment( Qt::AlignCenter );
	verticalLayout->addWidget( sensorNameLabel );

	progressBar = new QProgressBar( this );
	progressBar->setMaximum( 1000 );
	progressBar->setValue( 0 );
	progressBar->setAlignment( Qt::AlignCenter );
	progressBar->setFormat( "" );
	verticalLayout->addWidget( progressBar );

	QVBoxLayout* tLayout = new QVBoxLayout( progressBar );
	tLayout->setContentsMargins( QMargins( 0, 0, 0, 0 ) );
	timeLeftLabel = new QLabel( this );
	timeLeftLabel->setAlignment( Qt::AlignCenter );
	tLayout->addWidget( timeLeftLabel );

	popupWindow = new QFrame;
	popupWindow->setWindowFlag( Qt::WindowFlags::enum_type::Popup );
	popupWindow->setFrameShape( QFrame::Shape::StyledPanel );
	popupWindow->setFrameShadow( QFrame::Shadow::Raised );
	popupWindow->installEventFilter( this );

	verticalLayout = new QVBoxLayout( popupWindow );
	verticalLayout->setContentsMargins( 4, 4, 4, 4 );
	bindLabel = new QLabel( "Bind to UNKNOWN", popupWindow );
	verticalLayout->addWidget( bindLabel );

	stateLabel = new QLabel( "State: Stopped", popupWindow );
	verticalLayout->addWidget( stateLabel );

	sensorErrorsListTableView = new QTableView( popupWindow );
	sensorErrorsListTableView->setFocusPolicy( Qt::FocusPolicy::NoFocus );
	sensorErrorsListTableView->setEditTriggers( QAbstractItemView::NoEditTriggers );
	sensorErrorsListTableView->setProperty( "showDropIndicator", QVariant( false ) );
	sensorErrorsListTableView->setDragDropOverwriteMode( false );
	sensorErrorsListTableView->setSelectionMode( QAbstractItemView::SingleSelection );
	sensorErrorsListTableView->setSelectionBehavior( QAbstractItemView::SelectRows );
	sensorErrorsListTableView->verticalHeader()->setStretchLastSection( true );
	sensorErrorsListTableView->setModel( &model );
	sensorErrorsListTableView->horizontalHeader()->setSectionResizeMode( QHeaderView::ResizeMode::Interactive );
	sensorErrorsListTableView->horizontalHeader()->resizeSection( 0, 26 );
	sensorErrorsListTableView->horizontalHeader()->resizeSection( 1, 90 );
	sensorErrorsListTableView->horizontalHeader()->resizeSection( 2, 75 );
	sensorErrorsListTableView->horizontalHeader()->resizeSection( 3, 118 );
	sensorErrorsListTableView->horizontalHeader()->setStretchLastSection( true );
	sensorErrorsListTableView->verticalHeader()->setStretchLastSection( false );
	sensorErrorsListTableView->verticalHeader()->hide();
	verticalLayout->addWidget( sensorErrorsListTableView );

	timeLeft = 0;
	nextSensorId = -1;

	setFixedSize( QSize( 135, 85 ) );

	QObject::connect( button, &QToolButton::released, this, &VPortTileWidget::buttonClicked );

	/*QColor baseColor( 165, 218, 238 );
	QLinearGradient linearGrad( QPointF( 0, 0 ), QPointF( 0, 85 ) );
	linearGrad.setColorAt( 0, baseColor.darker( 95 ) );
	linearGrad.setColorAt( 1, baseColor );
	QPalette p = palette();
	p.setBrush( QPalette::Window, QBrush( linearGrad ) );
	setPalette( p );
	setAutoFillBackground( true );*/
}

VPortTileWidget::VPortTileWidget( uint index, QWidget* parent /*= nullptr */ ) : VPortTileWidget( parent )
{
	setVPortIndex( index );
}

VPortTileWidget::~VPortTileWidget()
{
	popupWindow->deleteLater();
}

void VPortTileWidget::setVPortIndex( uint index )
{
	vPortLabel->setText( QString( "VPort%1" ).arg( index ) );
}

void VPortTileWidget::setBindInfo( QString bindInfo )
{
	bindLabel->setText( QString( "Bind to " ) + bindInfo );
}

void VPortTileWidget::setState( State state )
{
	stateLabel->setText( toText( state ) );
}

void VPortTileWidget::setNextSensorRead( uint sensorId, QString sensorName, uint timeLeft )
{
	sensorNameLabel->setText( QString( "%1. %2" ).arg( sensorId + 1 ).arg( sensorName ) );
	if( timeLeft == 0 )
	{
		this->timeLeft = 0;
		nextSensorId = ( int )sensorId;
		timeLeftLabel->setText( "" );
		progressBar->setMaximum( 0 );
		return;
	}
	timeLeftLabel->setText( QString( "t - %1:%2" ).arg( timeLeft / 60, 2, 10, QChar( '0' ) ).arg( timeLeft % 60, 2, 10, QChar( '0' ) ) );
	if( this->timeLeft < timeLeft || ( int )sensorId != nextSensorId )
		progressBar->setMaximum( ( int )timeLeft );
	this->timeLeft = timeLeft;
	nextSensorId = ( int )sensorId;
	progressBar->setValue( progressBar->maximum() - timeLeft );
}

void VPortTileWidget::resetNextSensorRead()
{
	sensorNameLabel->setText( "Unknown" );
	progressBar->setMaximum( 1000 );
	progressBar->setValue( 0 );
	timeLeftLabel->setText( "" );
	timeLeft = 0;
	nextSensorId = -1;
}

void VPortTileWidget::addSensorReadError( uint sensorId, QString sensorName, SensorError error, uint8_t errorCode, QDateTime date )
{
	if( model.rowCount() == 0 )
		statusLabel->setPixmap( QPixmap( ":/MarvieController/icons/icons8-box-important-42.png" ) );
	model.addSensorReadError( sensorId, sensorName, error, errorCode, date );
}

void VPortTileWidget::removeSensorReadError( uint sensorId )
{
	model.removeSensorReadError( sensorId );
	if( model.rowCount() == 0 )
		statusLabel->setPixmap( QPixmap( ":/MarvieController/icons/icons8-checkmark-64.png" ) );
}

void VPortTileWidget::clearSensorErrorsList()
{
	model.clear();
	statusLabel->setPixmap( QPixmap( ":/MarvieController/icons/icons8-checkmark-64.png" ) );
}

void VPortTileWidget::buttonClicked()
{
	auto desktopRect = QApplication::desktop()->rect();
	QRect rect = QRect( mapToGlobal( QPoint( width(), 0 ) ), QSize( 337, 200 ) );
	if( desktopRect.bottom() < rect.bottom() )
		rect.translate( 0, desktopRect.bottom() - rect.bottom() );
	if( desktopRect.right() < rect.right() )
		rect.translate( desktopRect.right() - rect.right(), 0 );
	popupWindow->setGeometry( rect );
	popupWindow->show();
	popupWindow->setFocus();
}

bool VPortTileWidget::eventFilter( QObject* obj, QEvent* e )
{
	if( e->type() == QEvent::FocusOut )
		popupWindow->hide();

	return false;
}

QString VPortTileWidget::toText( State state )
{
	switch( state )
	{
	case VPortTileWidget::State::Stopped:
		return "Stopped";
	case VPortTileWidget::State::Working:
		return "Working";
	case VPortTileWidget::State::Stopping:
		return "Stopping";
	default:
		break;
	}
	return "";
}
