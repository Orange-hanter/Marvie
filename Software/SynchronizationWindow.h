#pragma once

#include <QDialog>
#include <QLabel>
#include <QMovie>

class SynchronizationWindow : private QDialog
{
	Q_OBJECT

public:
	SynchronizationWindow( QWidget* mainWindow );
	~SynchronizationWindow();

	void show();
	void hide();

private:
	void closeEvent( QCloseEvent * event );

private slots:
	void frameChanged();
	
private:
	QWidget* mainWindow;
	QLabel* label;
	QMovie movie;
};