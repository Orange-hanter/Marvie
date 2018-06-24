#include "FramelessWindow.h"
#define QT_FEATURE_vulkan -1
#include "qwindowswindow.h"

//additional directories:
//Src\qtbase\include\QtGui\5.3.0\QtGui\
//Src\qtbase\src\plugins\platforms\windows

#ifdef _DEBUG
#pragma comment( lib, "Qt5WinExtrasd.lib")
#else
#pragma comment( lib, "Qt5WinExtras.lib")
#endif

FramelessWindowHandle::FramelessWindowHandle( QWidget* mainWidget )
{
	this->mainWidget = mainWidget;

	vLayout = new QVBoxLayout( mainWidget );
	vLayout->setContentsMargins( QMargins( 2, 2, 2, 2 ) );
	vLayout->setSpacing( 0 );

	hLayout = new QHBoxLayout;
	hLayout->setContentsMargins( QMargins( 0, 0, 6, 0 ) );
	hLayout->setSpacing( 0 );

	title = new QLabel( "Main application title" );
	title->setFixedHeight( 30 );
	title->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
	title->setFont( QFont( "Segoe UI", 13 ) );
	title->setContentsMargins( QMargins( 9, 0, 50, 0 ) );

	buttons = new WindowButtonsWidget;

	hLayout->addWidget( title, 1 );
	hLayout->addWidget( buttons, 0 );

	vLayout->addLayout( hLayout );

	borderWidth = 6;

	connect( buttons, SIGNAL( minimizeButtonClicked() ), SLOT( minimizeButtonClicked() ) );
	connect( buttons, SIGNAL( maximizeButtonClicked( WindowState ) ), SLOT( maximizeButtonClicked( WindowState ) ) );
	connect( buttons, SIGNAL( closeButtonClicked() ), SLOT( closeButtonClicked() ) );

	cWidget = nullptr;
	mBar = nullptr;

	QtWin::extendFrameIntoClientArea( mainWidget, 1, 1, 1, 1 );
	QMargins frameMargins = mainWidget->windowHandle()->frameMargins();
#if QT_VERSION < QT_VERSION_CHECK( 5, 7, 0 )
	QWindowsWindow::baseWindowOf( mainWidget->windowHandle() )->setCustomMargins( QMargins( -frameMargins.left(), -frameMargins.top(), -frameMargins.right(), -frameMargins.bottom() ) );
#else
	QWindowsWindow::windowsWindowOf( mainWidget->windowHandle() )->setCustomMargins( QMargins( -frameMargins.left(), -frameMargins.top(), -frameMargins.right(), -frameMargins.bottom() ) );
#endif
}

FramelessWindowHandle::~FramelessWindowHandle()
{
	delete vLayout;
}

void FramelessWindowHandle::setCentralWidget( QWidget* centralWidget )
{
	if( this->cWidget )
		vLayout->removeWidget( this->cWidget );
	vLayout->insertWidget( 1, centralWidget );
	vLayout->setStretch( 1, 1 );
	this->cWidget = centralWidget;
}

void FramelessWindowHandle::setMenuBar( QMenuBar* m )
{
	if( mBar )
		hLayout->removeWidget( this->mBar );
//  	else
// 		setTitleAlignment( Qt::AlignRight | Qt::AlignVCenter );

	mBar = m;
	hLayout->insertWidget( 0, mBar, 0, Qt::AlignLeft | Qt::AlignBottom );
}

void FramelessWindowHandle::setTitleText( QString text )
{
	title->setText( text );
}

void FramelessWindowHandle::setTitleFont( QFont font )
{
	title->setFont( font );
}

void FramelessWindowHandle::setTitleAlignment( Qt::Alignment alignment )
{
	title->setAlignment( alignment );
}

QWidget* FramelessWindowHandle::centralWidget()
{
	return cWidget;
}

QMenuBar* FramelessWindowHandle::menuBar()
{
	return mBar;
}

void FramelessWindowHandle::paintEvent( QPaintEvent* )
{
	QColor dark = mainWidget->palette().dark().color();
	QColor light = mainWidget->palette().light().color();
	QMargins margins = vLayout->contentsMargins() - QMargins( 2, 2, 2, 2 );
	QPainter painter( mainWidget );

	painter.setPen( QPen( dark ) );
	painter.drawLine( QPoint( 0 + margins.left(), 0 + margins.top() ), QPoint( mainWidget->width() - 1 - margins.left(), 0 + margins.top() ) );
	painter.drawLine( QPoint( mainWidget->width() - 1 - margins.left(), 0 + margins.top() ), QPoint( mainWidget->width() - 1 - margins.left(), mainWidget->height() - 1 - margins.bottom() ) );
	painter.drawLine( QPoint( mainWidget->width() - 1 - margins.left(), mainWidget->height() - 1 - margins.bottom() ), QPoint( 0 + margins.left(), mainWidget->height() - 1 - margins.left() ) );
	painter.drawLine( QPoint( 0 + margins.left(), mainWidget->height() - 1 - margins.left() ), QPoint( 0 + margins.left(), 0 + margins.top() ) );

	painter.setPen( QPen( light ) );
	painter.drawLine( QPoint( 1 + margins.left(), 1 + margins.top() ), QPoint( mainWidget->width() - 2 - margins.left(), 1 + margins.top() ) );
	painter.drawLine( QPoint( mainWidget->width() - 2 - margins.left(), 1 + margins.top() ), QPoint( mainWidget->width() - 2 - margins.left(), mainWidget->height() - 2 - margins.bottom() ) );
	painter.drawLine( QPoint( mainWidget->width() - 2 - margins.left(), mainWidget->height() - 2 - margins.bottom() ), QPoint( 1 + margins.left(), mainWidget->height() - 2 - margins.left() ) );
	painter.drawLine( QPoint( 1 + margins.left(), mainWidget->height() - 2 - margins.left() ), QPoint( 1 + margins.left(), 1 + margins.top() ) );

	if( vLayout->contentsMargins().left() == 2 )
	{
		painter.setRenderHint( QPainter::RenderHint::HighQualityAntialiasing );
		uint offset = 4, size = 10;
		QPoint bottomLeft( mainWidget->width() - offset - size, mainWidget->height() - offset + 1 ); // +1 пиксель по Y из-за использования DWM.. наверное)
		QPoint topRight( mainWidget->width() - offset, mainWidget->height() - offset - size + 1 );
		painter.drawLine( bottomLeft, topRight );
		painter.drawLine( bottomLeft + QPoint( 4, 0 ), topRight + QPoint( 0, 4 ) );
		painter.drawLine( bottomLeft + QPoint( 8, 0 ), topRight + QPoint( 0, 8 ) );
	}
}

bool FramelessWindowHandle::nativeEvent( const QByteArray &eventType, void *message, long *result )
{
	MSG* msg = static_cast< MSG* >( message );
	if( msg->message == WM_NCCALCSIZE )
	{
		*result = 0;
		return true;
	}
	else if( msg->message == WM_NCHITTEST )
	{
		int captionHeight = title->height();
		QRect winRect = mainWidget->geometry();
		QPoint point( LOWORD( msg->lParam ), HIWORD( msg->lParam ) );
		QRect leftBorder( winRect.left(), winRect.top(), borderWidth, winRect.height() );
		QRect topBorder( winRect.left(), winRect.top(), winRect.width(), borderWidth );
		QRect rightBorder( winRect.right() - borderWidth, winRect.top(), borderWidth, winRect.height() );
		QRect bottomBorder( winRect.left(), winRect.bottom() - borderWidth, winRect.width(), borderWidth );
		QRect captionRect( winRect.left(), winRect.top(), winRect.width(), captionHeight );

		if( leftBorder.contains( point ) && topBorder.contains( point ) )
			*result = HTTOPLEFT;
		else if( topBorder.contains( point ) && rightBorder.contains( point ) )
			*result = HTTOPRIGHT;
		else if( rightBorder.contains( point ) && bottomBorder.contains( point ) )
			*result = HTBOTTOMRIGHT;
		else if( bottomBorder.contains( point ) && leftBorder.contains( point ) )
			*result = HTBOTTOMLEFT;
		else if( leftBorder.contains( point ) )
			*result = HTLEFT;
		else if( topBorder.contains( point ) )
			*result = HTTOP;
		else if( rightBorder.contains( point ) )
			*result = HTRIGHT;
		else if( bottomBorder.contains( point ) )
			*result = HTBOTTOM;
		else if( captionRect.contains( point ) && !buttons->rect().contains( buttons->mapFromGlobal( point ) )
				 && ( mBar ? !mBar->rect().contains( mBar->mapFromGlobal( point ) ) : true ) )
				 *result = HTCAPTION;
		else
			*result = HTCLIENT;

		return true;
	}
	else if( msg->message == WM_SIZE && msg->wParam == SIZE_MAXIMIZED )
	{
		vLayout->setContentsMargins( QMargins( 10, 10, 10, 10 ) );
		buttons->setMaximizeButtonState( Maximized );
	}
	else if( msg->message == WM_SIZE && vLayout->contentsMargins().left() != 2 )
	{
		vLayout->setContentsMargins( QMargins( 2, 2, 2, 2 ) );
		buttons->setMaximizeButtonState( Normal );
	}

	return false;
}

void FramelessWindowHandle::minimizeButtonClicked()
{
	mainWidget->showMinimized();
}

void FramelessWindowHandle::maximizeButtonClicked( WindowState state )
{
	buttons->setMaximizeButtonState( state );
	if( state == Maximized )
		mainWidget->showMaximized();
	else
		mainWidget->showNormal();
}

void FramelessWindowHandle::closeButtonClicked()
{
	mainWidget->close();
}

WindowButtonsWidget* FramelessWindowHandle::windowButtons()
{
	return buttons;
}

FramelessWindow::FramelessWindow( QWidget* mainWidget )
{
	framelessWindowHandle = new FramelessWindowHandle( mainWidget );
}

FramelessWindow::~FramelessWindow()
{
	delete framelessWindowHandle;
}

void FramelessWindow::setCentralWidget( QWidget* widget )
{
	framelessWindowHandle->setCentralWidget( widget );
}

void FramelessWindow::setMenuBar( QMenuBar* m )
{
	framelessWindowHandle->setMenuBar( m );
}

void FramelessWindow::setTitleText( QString text )
{
	framelessWindowHandle->setTitleText( text );
}

void FramelessWindow::setTitleFont( QFont font )
{
	framelessWindowHandle->setTitleFont( font );
}

void FramelessWindow::setTitleAlignment( Qt::Alignment alignment )
{
	framelessWindowHandle->setTitleAlignment( alignment );
}

QWidget* FramelessWindow::centralWidget()
{
	return framelessWindowHandle->centralWidget();
}

QMenuBar* FramelessWindow::menuBar()
{
	return framelessWindowHandle->menuBar();
}

void FramelessWindow::paintEventHandle( QPaintEvent* e )
{
	framelessWindowHandle->paintEvent( e );
}

bool FramelessWindow::nativeEventHandle( const QByteArray &eventType, void *message, long *result )
{
	return framelessWindowHandle->nativeEvent( eventType, message, result );
}

WindowButtonsWidget* FramelessWindow::windowButtons()
{
	return framelessWindowHandle->windowButtons();
}

FramelessWidget::FramelessWidget( QWidget *parent ) : QWidget( parent ), FramelessWindow( this )
{
	
}

FramelessWidget::~FramelessWidget()
{

}

void FramelessWidget::paintEvent( QPaintEvent* e )
{
	paintEventHandle( e );
}

bool FramelessWidget::nativeEvent( const QByteArray &eventType, void *message, long *result )
{
	return nativeEventHandle( eventType, message, result );
}

FramelessDialog::FramelessDialog( QWidget *parent ) : QDialog( parent ), FramelessWindow( this )
{
	
}

FramelessDialog::~FramelessDialog()
{

}

void FramelessDialog::paintEvent( QPaintEvent* e )
{
	paintEventHandle( e );
}

bool FramelessDialog::nativeEvent( const QByteArray &eventType, void *message, long *result )
{
	return nativeEventHandle( eventType, message, result );
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

static inline QSize qSizeOfRect( const RECT &rect )
{
	return QSize( rect.right - rect.left, rect.bottom - rect.top );
}

static inline QRect qrectFromRECT( const RECT &rect )
{
	return QRect( QPoint( rect.left, rect.top ), qSizeOfRect( rect ) );
}

static inline QRect frameGeometry( HWND hwnd, bool topLevel )
{
	RECT rect = { 0, 0, 0, 0 };
	GetWindowRect( hwnd, &rect ); // Screen coordinates.
	const HWND parent = GetParent( hwnd );
	if( parent && !topLevel )
	{
		const int width = rect.right - rect.left;
		const int height = rect.bottom - rect.top;
		POINT leftTop = { rect.left, rect.top };
		ScreenToClient( parent, &leftTop );
		rect.left = leftTop.x;
		rect.top = leftTop.y;
		rect.right = leftTop.x + width;
		rect.bottom = leftTop.y + height;
	}
	return qrectFromRECT( rect );
}

void QWindowsWindow::setCustomMargins( const QMargins &newCustomMargins )
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
	if( newCustomMargins != m_data.customMargins )
	{
		const QMargins oldCustomMargins = m_data.customMargins;
		m_data.customMargins = newCustomMargins;
		// Re-trigger WM_NCALCSIZE with wParam=1 by passing SWP_FRAMECHANGED
		const QRect currentFrameGeometry = frameGeometry( handle(), isTopLevel() );
		const QPoint topLeft = currentFrameGeometry.topLeft();
		QRect newFrame = currentFrameGeometry.marginsRemoved( oldCustomMargins ) + m_data.customMargins;
		newFrame.moveTo( topLeft );
		SetWindowPos( m_data.hwnd, 0, newFrame.x(), newFrame.y(), newFrame.width(), newFrame.height(), SWP_NOZORDER | SWP_FRAMECHANGED );
	}
#else
	if( newCustomMargins != m_data.customMargins )
	{
		const QMargins oldCustomMargins = m_data.customMargins;
		m_data.customMargins = newCustomMargins;
		// Re-trigger WM_NCALCSIZE with wParam=1 by passing SWP_FRAMECHANGED
		bool isRealTopLevel = window()->isTopLevel() && !m_data.embedded;
		const QRect currentFrameGeometry = frameGeometry( m_data.hwnd, isRealTopLevel );
		const QPoint topLeft = currentFrameGeometry.topLeft();
		QRect newFrame = currentFrameGeometry.marginsRemoved( oldCustomMargins ) + m_data.customMargins;
		newFrame.moveTo( topLeft );
		setFlag( FrameDirty );
		SetWindowPos( m_data.hwnd, 0, newFrame.x(), newFrame.y(), newFrame.width(), newFrame.height(), SWP_NOZORDER | SWP_FRAMECHANGED );
	}
#endif	
}