#pragma once

#include "FramelessWindow/FramelessWindow.h"
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
#include "VPortOverIpModel.h"
#include "VPortsOverIpDelegate.h"
#include "MonitoringDataModel.h"
#include "SensorFieldAddressMapModel.h"
#include "MonitoringLog.h"
#include "SynchronizationWindow.h"

#include "../Firmware/MarviePackets.h"

#include "ui_MarvieController.h"
#include "ui_SdStatistics.h"

class MarvieController : public FramelessWidget
{
	Q_OBJECT

public:
	MarvieController( QWidget *parent = Q_NULLPTR );
	~MarvieController();

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
	void syncDateTimeButtonClicked();
	void sdCardMenuButtonClicked();
	void logMenuButtonClicked();

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

	bool loadConfigFromXml( QByteArray xmlData );
	QByteArray saveConfigToXml();

	struct DeviceMemoryLoad;
	void updateDeviceCpuLoad( float cpuLoad );
	void updateDeviceMemoryLoad( const MarviePackets::MemoryLoad* memoryLoad );
	void resetDeviceLoad();
	void updateDeviceStatus( const MarviePackets::DeviceStatus* );
	void updateEthernetStatus( const MarviePackets::EthernetStatus* );
	void updateGsmStatus( const MarviePackets::GsmStatus* );
	void updateServiceStatistics( const MarviePackets::ServiceStatistics* );
	void resetDeviceInfo();

	DateTime toDeviceDateTime( const QDateTime& );
	QDateTime toQtDateTime( const DateTime& );
	QString printDateTime( const QDateTime& );

	QString saveCanonicalXML( const QDomDocument& doc, int indent = 1 ) const;
	void writeDomNodeCanonically( QXmlStreamWriter &stream, const QDomNode &domNode ) const;

private:
	MLinkClient mlink;
	QIODevice* mlinkIODevice;
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

	QMenu* sdCardMenu, *logMenu;

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

	QVector< QString > deviceVPorts, deviceSensors, deviceSupportedSensors;
	enum class DeviceState { Unknown, IncorrectConfiguration, Working, Reconfiguration } deviceState;
	enum class SdCardStatus : uint8_t { Unknown, NotInserted, Initialization, InitFailed, BadFileSystem, Formatting, Working } deviceSdCardStatus;
	enum class LogState : uint8_t { Unknown, Off, Stopped, Working, Archiving, Stopping } deviceLogState;

	Ui::MarvieControllerClass ui;
};