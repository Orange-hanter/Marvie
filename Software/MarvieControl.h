#pragma once

#ifdef USE_FRAMELESS_WINDOW 
#include "FramelessWindow/FramelessWindow.h"
#endif
#include "MLinkClient.h"
#include "AccountWindow.h"
#include <QList>
#include <QVector>
#include <QDomDocument>
#include <QXmlSchemaValidator>
#include <QAbstractMessageHandler>
#include "MonitoringDataTreeWidget.h"
#include <QMenu>
#include <QMessageBox>
#include <QListWidget>
#include "VPortOverIpModel.h"
#include "VPortsOverIpDelegate.h"
#include "MonitoringDataModel.h"
#include "SensorFieldAddressMapModel.h"
#include "MonitoringLog.h"
#include "SynchronizationWindow.h"
#include "DeviceFirmwareInfoWidget.h"

#include "../Firmware/Src/MarviePackets.h"

#include "ui_MarvieControl.h"
#include "ui_SdStatistics.h"
#include "ui_ComPortSharingSettingsWindow.h"

class MLinkTerminal;
class RemoteTerminalClient;

#ifdef USE_FRAMELESS_WINDOW
class MarvieControl : public FramelessWidget
#else
class MarvieControl : public QWidget
#endif
{
	Q_OBJECT

public:
	MarvieControl( QWidget *parent = Q_NULLPTR );
	~MarvieControl();

private:
	bool eventFilter( QObject *obj, QEvent *event ) final override;

private slots:
	void mainMenuButtonClicked();
	void settingsMenuButtonClicked();

	void nextInterfaceButtonClicked();
	void connectButtonClicked();
	void logInButtonClicked( QString accountName, QString accountPassword );
	void logOutButtonClicked( QString accountName );

	void deviceRestartButtonClicked();
	void startVPortsButtonClicked();
	void stopVPortsButtonClicked();
	void updateAllSensorsButtonClicked();
	void updateSensorButtonClicked();
	void openComPortSharingSettingsButtonClicked();
	void startComPortSharingButtonClicked();
	void deviceVersionMenuButtonClicked();
	void syncDateTimeButtonClicked();
	void sdCardMenuButtonClicked();
	void logMenuButtonClicked();

	void deviceVersionMenuActionTriggered( QAction* action );
	void sdCardMenuActionTriggered( QAction* action );
	void logMenuActionTriggered( QAction* action );

	void monitoringDataViewMenuRequested( const QPoint& point );
	void monitoringDataViewMenuActionTriggered( QAction* action );

	void monitoringLogOpenButtonClicked();
	void monitoringLogDateChanged();
	void monitoringLogTimeChanged();
	void monitoringLogSliderChanged( int );
	void monitoringLogMoveEntryButtonClicked();
	void setMonitoringLogWidgetGroupEnabled( bool enabled );

	void monitoringLogViewMenuRequested( const QPoint& point );
	void monitoringLogViewMenuActionTriggered( QAction* action );

	void accountSettingsCleanButtonClicked();
	void accountSettingsChangePasswordButtonClicked();

	void sensorSettingsMenuRequested( const QPoint& point );
	void sensorSettingsMenuActionTriggered( QAction* action );

	void modbusRegMapMenuRequested( const QPoint& point );
	void modbusRegMapMenuActionTriggered( QAction* action );

	void targetDeviceChanged( QString );
	void newConfigButtonClicked();
	void importConfigButtonClicked();
	void exportConfigButtonClicked();
	void uploadConfigButtonClicked();
	void downloadConfigButtonClicked();

	void addVPortOverIpButtonClicked();
	void removeVPortOverIpButtonClicked();
	void comPortAssignmentChanged( unsigned int id, ComPortsConfigWidget::Assignment previous, ComPortsConfigWidget::Assignment current );
	void updateVPortsList();

	void sensorNameEditReturnPressed();
	void sensorNameTimerTimeout();
	void sensorNameSearchCompleted();
	void sensorAddButtonClicked();
	void sensorRemoveButtonClicked();
	void sensorMoveUpButtonClicked();
	void sensorMoveDownButtonClicked();
	void sensorCopyButtonClicked();
	void sensorsClearButtonClicked();

	void exportModbusRegMapToCsvButtonClicked();

	void mlinkStateChanged( MLinkClient::State );
	void mlinkError( MLinkClient::Error );
	void mlinkNewPacketAvailable( uint8_t type, QByteArray data );
	void mlinkNewComplexPacketAvailable( uint8_t channelId, QString name, QByteArray data );
	void mlinkComplexDataSendingProgress( uint8_t channelId, QString name, float progress );
	void mlinkComplexDataReceivingProgress( uint8_t channelId, QString name, float progress );

private:
	void sensorsInit();
	QByteArray generateSensorsXSD();

	QTreeWidgetItem* insertSensorSettings( int index, QString sensorName, QMap< QString, QString > sensorSettingsValues = QMap< QString, QString >(), bool needUpdateName = false );
	void removeSensorSettings( int index, bool needUpdateName );
	void setSensorSettingsNameNum( int index, int num );
	QMap< QString, QString > sensorSettingsValues( int index );
	void fixSensorVPortIds( bool needUpdate );
	QStringList vPortFullNames();
	void clearMainConfig();
	void clearSensorsConfig();

	bool loadConfigFromXml( QByteArray xmlData );
	QByteArray saveConfigToXml();
	void appendSensorsConfig( QDomDocument& doc, QDomElement& root );

	struct DeviceMemoryLoad;
	void updateDeviceCpuLoad( float cpuLoad );
	void updateDeviceMemoryLoad( const MarviePackets::MemoryLoad* memoryLoad );
	void resetDeviceLoad();
	void updateDeviceVersion( QString version );
	void updateDeviceStatus( const MarviePackets::DeviceStatus* );
	void updateEthernetStatus( const MarviePackets::EthernetStatus* );
	void updateGsmStatus( const MarviePackets::GsmStatus* );
	void updateServiceStatistics( const MarviePackets::ServiceStatistics* );
	void updateComPortSharingStatus( int sharedComPortIndex );
	void resetDeviceInfo();

	DateTime toDeviceDateTime( const QDateTime& );
	QDateTime toQtDateTime( const DateTime& );
	QString printDateTime( const QDateTime& );

	QString saveCanonicalXML( const QDomDocument& doc, int indent = 1 ) const;
	void writeDomNodeCanonically( QXmlStreamWriter &stream, const QDomNode &domNode ) const;
	QString toText( const QDomNode &domNode, int indent = 1 );

private:
	MLinkClient mlink;
	AccountWindow* accountWindow;

	class XmlMessageHandler : public QAbstractMessageHandler
	{
		virtual void handleMessage( QtMsgType type, const QString &description, const QUrl &identifier, const QSourceLocation &sourceLocation );
	public:
		QString description;
		QSourceLocation sourceLocation;
	} xmlMessageHandler;
	
	QMap< QString, SensorDesc > sensorDescMap;
	QMap< QString, SensorUnfoldedDesc > sensorUnfoldedDescMap;
	SensorFieldAddressMapModel sensorFieldAddressMapModel;

	QVector< QString > vPorts, loadedXmlSensors;

	class VPortIdComboBoxEventFilter : public QObject
	{
		const QVector< QString >& vPorts;
	public:
		VPortIdComboBoxEventFilter( const QVector< QString >& vPorts ) : vPorts( vPorts ) {}
		bool eventFilter( QObject *obj, QEvent *event ) final override;
	} vPortIdComboBoxEventFilter;
	VPortOverIpModel vPortsOverEthernetModel;
	VPortsOverIpDelegate vPortsOverIpDelegate;

	class MemStatistics : public QFrame
	{
	public:
		MemStatistics();

		void setStatistics( uint32_t gmemMaxUsed, uint32_t gmemHeap, uint32_t gmemFragments, uint32_t gmemLargestFragment,
							uint32_t ccmemMaxUsed, uint32_t ccmemHeap, uint32_t ccmemFragments, uint32_t ccmemLargestFragment );
		void show( QPoint point );

	private:
		void focusOutEvent( QFocusEvent *event ) override;

		QLabel* gMemMaxUsedLabel;
		QLabel* gMemHeapLabel;
		QLabel* gMemFragmentsLabel;
		QLabel* gMemLargestLabel;
		QLabel* ccMemMaxUsedLabel;
		QLabel* ccMemHeapLabel;
		QLabel* ccMemFragmentsLabel;
		QLabel* ccMemLargestLabel;
	}*memStat;

	class SdStatistics : public QFrame
	{
	public:
		SdStatistics();

		void setStatistics( uint64_t totalSize, uint64_t freeSize, uint64_t logSize );
		void show( QPoint point );

	private:
		void focusOutEvent( QFocusEvent *event ) override;
		QString printSize( uint64_t size );

		Ui::SdStatisticsForm ui;
	}*sdStat;

	QMenu* deviceVersionMenu, *sdCardMenu, *logMenu;
	QMenu* monitoringDataViewMenu;

	MonitoringLog monitoringLog;
	QVector< QTime > monitoringLogTimestamps;
	QMenu* monitoringLogViewMenu;

	QTimer sensorNameTimer;
	QListWidget* popupSensorsListWidget;

	QMenu* sensorSettingsMenu;
	QMenu* modbusRegMapMenu;

	QXmlSchemaValidator configValidator;

	SynchronizationWindow* syncWindow;

	DeviceFirmwareInfoWidget* deviceFirmwareInfoWidget;
	MarviePackets::DeviceSpecs deviceSpecs;
	QVector< QString > deviceVPorts, deviceSensors, deviceSupportedSensors;
	enum class DeviceState { Unknown, IncorrectConfiguration, Working, Reconfiguration } deviceState;
	enum class SdCardStatus : uint8_t { Unknown, NotInserted, Initialization, InitFailed, BadFileSystem, Formatting, Working } deviceSdCardStatus;
	enum class LogState : uint8_t { Unknown, Off, Stopped, Working, Archiving, Stopping } deviceLogState;

	MLinkTerminal* mlinkTerminal;

	class ComPortSharingSettingsWindow : public QFrame 
	{
	public:
		ComPortSharingSettingsWindow();

		void setMode( MarviePackets::ComPortSharingSettings::Mode _mode );
		MarviePackets::ComPortSharingSettings::Mode mode();
		void setDataFormat( MarviePackets::ComPortSharingSettings::DataFormat _format );
		MarviePackets::ComPortSharingSettings::DataFormat dataFormat();
		void setStopBits( MarviePackets::ComPortSharingSettings::StopBits _stopBits );
		MarviePackets::ComPortSharingSettings::StopBits stopBits();
		void setBaudrate( uint32_t baudrate );
		uint32_t baudrate();

		void focusOutEvent( QFocusEvent *event );
		Ui::ComPortSharingSettings ui;
	}* comPortSharingSettingsWindow;

	Ui::MarvieControlClass ui;
};

class MLinkTerminal : public QIODevice
{
	Q_OBJECT

	MLinkClient* mlink;
	RemoteTerminalClient* terminal;
	QByteArray in, out;
	QTimer timer;

public:
	MLinkTerminal( MLinkClient* mlink, RemoteTerminalClient* terminal );
	~MLinkTerminal();
	qint64 writeData( const char *data, qint64 len ) override;
	qint64 readData( char *data, qint64 maxlen ) override;
	qint64 bytesAvailable() const override;
	qint64 bytesToWrite() const override;
	bool isSequential() const override;

private slots:
	void mlinkStateChanged( MLinkClient::State );
	void mlinkNewPacketAvailable( uint8_t type, QByteArray data );
	void timeout();
};