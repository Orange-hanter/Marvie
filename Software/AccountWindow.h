#pragma once

#include <QFrame>
#include <ui_AccountWindow.h>

class AccountWindow : public QFrame
{
	Q_OBJECT

public:
	AccountWindow( QWidget* parent = nullptr );
	~AccountWindow();

	void reset();

signals:
	void logIn( QString accountName, QString accountPassword );
	void logOut( QString accountName );

public slots:
	void logInConfirmed();
	void logOutConfirmed();
	void passwordIncorrect();

private slots:
	void buttonClicked();
	void checkBoxChanged( int state );

private:
	void focusOutEvent( QFocusEvent *event );
	void setInputEnabled( bool enabled );

private:
	Ui::AccountForm ui;
};