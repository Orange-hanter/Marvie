// Frameless window v1.10

#pragma  once

#include <QtWinExtras/QtWin>
#include "WindowButtonsWidget.h"

class FramelessWindowHandle : public QObject
{
	Q_OBJECT

public:
	FramelessWindowHandle( QWidget* mainWidget );
	~FramelessWindowHandle();

	void setCentralWidget( QWidget* );
	void setMenuBar( QMenuBar* );

	void setTitleText( QString );
	void setTitleFont( QFont );
	void setTitleAlignment( Qt::Alignment );

	WindowButtonsWidget* windowButtons();
	QWidget* centralWidget();
	QMenuBar* menuBar();
	
	void paintEvent( QPaintEvent* );
	bool nativeEvent( const QByteArray &eventType, void *message, long *result );

private slots:
	void minimizeButtonClicked();
	void maximizeButtonClicked( WindowState state );
	void closeButtonClicked();

private:
	QWidget* mainWidget;
	WindowButtonsWidget* buttons;

	QVBoxLayout* vLayout;
	QHBoxLayout* hLayout;
	QWidget* titleWidget;
	QMenuBar* mBar;
	QWidget* cWidget;
	QLabel* title;

	int borderWidth;
};

class FramelessWindow
{
public:
	FramelessWindow( QWidget* mainWidget );
	virtual ~FramelessWindow();

public:
	void setCentralWidget( QWidget* );
	void setMenuBar( QMenuBar* );

	void setTitleText( QString );
	void setTitleFont( QFont );
	void setTitleAlignment( Qt::Alignment );

	WindowButtonsWidget* windowButtons();
	QWidget* centralWidget();
	QMenuBar* menuBar();
	
	void paintEventHandle( QPaintEvent* );
	bool nativeEventHandle( const QByteArray &eventType, void *message, long *result );

private:
	FramelessWindowHandle* framelessWindowHandle;
};

class FramelessWidget : public QWidget, public FramelessWindow
{
public:
	FramelessWidget( QWidget *parent = 0 );
	~FramelessWidget();
	
private:
	void paintEvent( QPaintEvent* );
	bool nativeEvent( const QByteArray &eventType, void *message, long *result );
};

class FramelessDialog : public QDialog, public FramelessWindow
{
public:
	FramelessDialog( QWidget *parent = 0 );
	~FramelessDialog();

private:
	void paintEvent( QPaintEvent* );
	bool nativeEvent( const QByteArray &eventType, void *message, long *result );
};