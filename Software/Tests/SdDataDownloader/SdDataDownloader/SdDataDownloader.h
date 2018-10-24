#pragma once

#include <QtWidgets/QWidget>
#include <QTcpSocket>
#include <QFile>
#include "ui_SdDataDownloader.h"

class SdDataDownloader : public QWidget
{
	Q_OBJECT

public:
	SdDataDownloader( QWidget *parent = nullptr );

private slots:
	void downloadButtonClicked();

	void socketConnected();
	void socketDisconnected();
	void socketError( QAbstractSocket::SocketError socketError );
	void newDataAvailable();

private:
	QTcpSocket* socket;
	QFile file;
	Ui::SdDataDownloaderClass ui;
};
