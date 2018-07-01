#pragma once

#include <QtWidgets/QWidget>
#include "MLinkClient.h"
#include "ui_MLinkClientTest.h"

class MLinkClientTest : public QWidget
{
	Q_OBJECT

public:
	MLinkClientTest( QWidget *parent = Q_NULLPTR );

private:
	bool eventFilter( QObject *obj, QEvent *event ) final override;

private slots:
	void connectButtonClicked();
	void ch2ButtonClicked();
	void ch3ButtonClicked();

	void mlinkStateChanged( MLinkClient::State );
	void mlinkNewPacketAvailable( uint8_t type, QByteArray data );
	void mlinkNewComplexPacketAvailable( uint8_t channelId, QString name, QByteArray data );
	void mlinkComplexDataSendingProgress( uint8_t channelId, QString name, float progress );
	void mlinkComplexDataSendindCanceled( uint8_t channelId, QString name );
	void mlinkComplexDataReceivingProgress( uint8_t channelId, QString name, float progress );
	void mlinkComplexDataReceivingCanceled( uint8_t channelId, QString name );

	void updateLabels();

private:
	QIODevice * ioDevice;
	MLinkClient mlink;
	int dwc[2], upc[2], spc;
	QByteArray cdata[2];
	uint32_t ch3Size;

	Ui::MLinkClientTestClass ui;
};
