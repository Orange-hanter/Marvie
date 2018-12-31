#pragma once

#include "thirdparty/FramelessWindow/FramelessWindow.h"
#include "MLinkClient.h"
#include <QProgressBar>

class DataTransferProgressWindow : public FramelessDialog
{
	Q_OBJECT

public:
	enum class TransferDir { Receiving, Sending };

	DataTransferProgressWindow( MLinkClient* link, uint8_t channelId, TransferDir dir, QWidget* parent, int errPacketId = -1 );

private slots:
	void buttonClicked();
	void stateChanged( MLinkClient::State state );
	void newPacketAvailable( uint8_t type, QByteArray data );
	void newComplexPacketAvailable( uint8_t channelId, QString name, QByteArray data );
	void complexDataSendingProgress( uint8_t channelId, QString name, float progress );
	void complexDataReceivingProgress( uint8_t channelId, QString name, float progress );

private:
	void closeEvent( QCloseEvent * event );
	void okButton();
	void error();

private:
	MLinkClient* link;
	int channelId;
	TransferDir dir;
	int errPacketId;
	QProgressBar* progressBar;
	QToolButton* button;
};