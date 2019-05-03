#pragma once

#include <QtWidgets/QWidget>
#include "ui_RemoteTerminalTest.h"
#include <QTcpSocket>

class RemoteTerminalTest : public QWidget
{
	Q_OBJECT

public:
	RemoteTerminalTest( QWidget *parent = nullptr );

private slots:
	void connectButtonClicked();
	void connected();
	void disconnected();
	void error( QAbstractSocket::SocketError socketError );

private:
	QTcpSocket* socket;
	Ui::RemoteTerminalTestClass ui;
};
