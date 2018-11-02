#include "AccountWindow.h"
#include <QSettings>
#include <QToolTip>

AccountWindow::AccountWindow( QWidget* parent /*= nullptr */ ) : QFrame( parent )
{
	ui.setupUi( this );

	setWindowFlag( Qt::WindowFlags::enum_type::Popup );
	setFrameShape( QFrame::Shape::StyledPanel );
	setFrameShadow( QFrame::Shadow::Raised );

	QSettings settings( "settings.ini", QSettings::Format::IniFormat );
	ui.accountNameEdit->setText( settings.value( "accountName" ).toString() );
	ui.accountPasswordEdit->setText( settings.value( "accountPassword" ).toString() );
	ui.rememberMeCheckBox->setChecked( !ui.accountPasswordEdit->text().isEmpty() );

	QObject::connect( ui.logInButton, &QPushButton::released, this, &AccountWindow::buttonClicked );
	QObject::connect( ui.rememberMeCheckBox, &QCheckBox::stateChanged, this, &AccountWindow::checkBoxChanged );
}

AccountWindow::~AccountWindow()
{

}

void AccountWindow::reset()
{
	ui.logInButton->setText( "Log in" );
	ui.logInButton->setEnabled( true );
	setInputEnabled( true );
}

void AccountWindow::logInConfirmed()
{
	ui.logInButton->setText( "Log out" );
	ui.logInButton->setEnabled( true );
}

void AccountWindow::logOutConfirmed()
{
	ui.logInButton->setText( "Log in" );
	ui.logInButton->setEnabled( true );
	setInputEnabled( true );
}

void AccountWindow::passwordIncorrect()
{
	setInputEnabled( true );
	ui.logInButton->setText( "Log in" );
	ui.logInButton->setEnabled( true );

	QSettings settings( "settings.ini", QSettings::Format::IniFormat );
	settings.remove( "accountPassword" );
	ui.accountPasswordEdit->setText( "" );

	show();
	QToolTip::showText( ui.accountPasswordEdit->mapToGlobal( ui.accountPasswordEdit->rect().bottomLeft() ) + QPoint( 10, 0 ), "Incorrect account name or password.", this, rect(), 3000 );
}

void AccountWindow::buttonClicked()
{
	if( ui.logInButton->text() == "Log in" )
	{
		QSettings settings( "settings.ini", QSettings::Format::IniFormat );
		if( settings.value( "accountName" ).toString() != ui.accountNameEdit->text() )
			settings.setValue( "accountName", ui.accountNameEdit->text() );
		if( ui.rememberMeCheckBox->isChecked() )
		{
			if( settings.value( "accountPassword" ).toString() != ui.accountPasswordEdit->text() )
				settings.setValue( "accountPassword", ui.accountPasswordEdit->text() );
		}
		ui.logInButton->setEnabled( false );
		setInputEnabled( false );
		logIn( ui.accountNameEdit->text(), ui.accountPasswordEdit->text() );
	}
	else
	{
		ui.logInButton->setEnabled( false );
		logOut( ui.accountNameEdit->text() );
	}
}

void AccountWindow::checkBoxChanged( int state )
{
	if( !ui.rememberMeCheckBox->isChecked() )
	{
		QSettings settings( "settings.ini", QSettings::Format::IniFormat );
		settings.remove( "accountPassword" );
	}
}

void AccountWindow::focusOutEvent( QFocusEvent *event )
{
	hide();
}

void AccountWindow::setInputEnabled( bool enabled )
{
	ui.accountLabel->setEnabled( enabled );
	ui.passwordLabel->setEnabled( enabled );
	ui.accountNameEdit->setEnabled( enabled );
	ui.accountPasswordEdit->setEnabled( enabled );
	ui.rememberMeCheckBox->setEnabled( enabled );
}