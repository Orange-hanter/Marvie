// Window buttons widget v1.10

#pragma once

#include <QtWidgets>

enum WindowState { Minimized, Maximized, Normal };
enum ButtonType { Minimize = 0x1, Maximize = 0x2, Close = 0x4 };

class WindowButtonsWidget : public QWidget
{
	Q_OBJECT

public:
	WindowButtonsWidget( QWidget* parent = 0 );
	~WindowButtonsWidget();

	void setButtonColor( int type, QColor color );
	void setButtons( int );

signals:
	void minimizeButtonClicked();
	void maximizeButtonClicked( WindowState state );
	void closeButtonClicked();

public slots:
	void setMaximizeButtonState( WindowState state );

private:
	bool eventFilter( QObject*, QEvent* );
	void timerEvent( QTimerEvent* );

private:
	int buttons;
	QPoint iconPosition;
	WindowState state;
	QHBoxLayout* layout;
	QWidget *minimizeButton, *maximizeButton, *closeButton;
	int minimizeButtonState, maximizeButtonState, closeButtonState;
	QImage minimizeIconImage, maximizeIconImage, restoreIconImage, closeIconImage;
};

