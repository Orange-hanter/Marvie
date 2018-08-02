#pragma once

#include "FramelessWindow/FramelessWindow.h"
#include "MLinkClient.h"
#include <QMap>
#include <QList>
#include <QVector>
#include <QRegExp>
#include <QDomDocument>
#include <QXmlSchemaValidator>
#include <QAbstractMessageHandler>
#include <QSharedPointer>
#include <QMenu>
#include <QMessageBox>
#include "VPortOverIpModel.h"
#include "VPortsOverIpDelegate.h"
#include "MonitoringDataModel.h"
#include "SynchronizationWindow.h"

#include "../Firmware/MarviePackets.h"

#include "ui_MarvieController.h"

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

	void startVPortsButtonClicked();
	void stopVPortsButtonClicked();
	void updateAllSensorsButtonClicked();
	void updateSensorButtonClicked();
	void syncDateTimeButtonClicked();
	void formatSdCardButtonClicked();

	void monitoringDataViewMenuRequested( const QPoint& point );
	void monitoringDataViewMenuActionTriggered( QAction* action );

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
	void sensorMoveButtonClicked();
	void sensorsClearButtonClicked();

	void mlinkStateChanged( MLinkClient::State );
	void mlinkNewPacketAvailable( uint8_t type, QByteArray data );
	void mlinkNewComplexPacketAvailable( uint8_t channelId, QString name, QByteArray data );
	void mlinkComplexDataSendingProgress( uint8_t channelId, QString name, float progress );
	void mlinkComplexDataReceivingProgress( uint8_t channelId, QString name, float progress );

private:
	void sensorsInit();
	QByteArray generateSensorsXSD();

	QTreeWidgetItem* insertSensorSettings( int index, QString sensorName, QMap< QString, QString > sensorSettingsValues = QMap< QString, QString >() );
	void removeSensorSettings( int index, bool needUpdateName );
	void setSensorSettingsNameNum( int index, int num );
	QMap< QString, QString > sensorSettingsValues( int index );
	QStringList vPortFullNames();

	bool loadConfigFromXml( QByteArray xmlData );
	QByteArray saveConfigToXml();

	struct DeviceMemoryLoad;
	void updateDeviceCpuLoad( float cpuLoad );
	void updateDeviceMemoryLoad( const DeviceMemoryLoad& memoryLoad );
	void resetDeviceLoad();
	void updateDeviceStatus( const MarviePackets::DeviceStatus* );
	void resetDeviceStatus();

	void updateSensorData( uint id, QString sensorName, const uint8_t* data );
	void attachSensorRelatedMonitoringDataItems( MonitoringDataItem* sensorItem, QString sensorName );
	void updateAnalogData( uint id, float* data, uint count );
	void updateDiscreteData( uint id, uint64_t data, uint count );
	void attachADRelatedMonitoringDataItems( MonitoringDataItem* item, uint count );
	void insertTopLevelMonitoringDataItem( MonitoringDataItem* item );

	DateTime toDeviceDateTime( const QDateTime& );
	QDateTime toQtDateTime( const DateTime& );
	QString printDateTime( const QDateTime& );

	QString saveCanonicalXML( const QDomDocument& doc, int indent = 1 ) const;
	void writeDomNodeCanonically( QXmlStreamWriter &stream, const QDomNode &domNode ) const;

private:
	MLinkClient mlink;
	QIODevice* mlinkIODevice;

	class XmlMessageHandler : public QAbstractMessageHandler
	{
		virtual void handleMessage( QtMsgType type, const QString &description, const QUrl &identifier, const QSourceLocation &sourceLocation );
	public:
		QString description;
		QSourceLocation sourceLocation;
	} xmlMessageHandler;
	struct SensorDesc
	{
		struct Settings
		{			
			enum class Target { BR, SR } target;
			struct Parameter
			{
				enum class Type { Int, Float, Double, Bool, String, Enum } type;
				QString name;
				Parameter( Type type, QString name ) : type( type ), name( name ) {}
			};
			struct IntParameter : public Parameter
			{
				int defaultValue, min, max;
				IntParameter( QString name, int defaultValue, int min, int max ) : Parameter( Type::Int, name ), defaultValue( defaultValue ), min( min ), max( max ) {}
			};
			struct FloatParameter : public Parameter
			{
				float defaultValue, min, max;
				FloatParameter( QString name, float defaultValue, float min, float max ) : Parameter( Type::Float, name ), defaultValue( defaultValue ), min( min ), max( max ) {}
			};
			struct DoubleParameter : public Parameter
			{
				double defaultValue, min, max;
				DoubleParameter( QString name, double defaultValue, double min, double max ) : Parameter( Type::Double, name ), defaultValue( defaultValue ), min( min ), max( max ) {}
			};
			struct BoolParameter : public Parameter
			{
				bool defaultValue;
				BoolParameter( QString name, bool defaultValue ) : Parameter( Type::Bool, name ), defaultValue( defaultValue ) {}
			};
			struct StringParameter : public Parameter
			{
				QString defaultValue;
				QRegExp regExp;
				StringParameter( QString name, QString defaultValue, QRegExp regExp ) : Parameter( Type::String, name ), defaultValue( defaultValue ), regExp( regExp ) {}
			};
			struct EnumParameter : public Parameter
			{
				QMap< QString, int > map;
				QString defaultValue;
				EnumParameter( QString name, QMap< QString, int > map, QString defaultValue ) : Parameter( Type::Enum, name ), map( map ), defaultValue( defaultValue ) {}
			};
			QList< QSharedPointer< Parameter > > prmList;
		} settings;

		struct Data
		{
			enum class Type { Char, Int8, Uint8, Int16, Uint16, Int32, Uint32, Int64, Uint64, Float, Double, Unused, Array, Group, GroupArray };
			struct Node
			{
				Node( Type type, int bias, QString name = QString() ) : type( type ), bias( bias ), name( name ) {}	
				~Node() { qDeleteAll( childNodes ); }
				QString name;
				Type type;
				int bias;
				QList< Node* > childNodes;
			};
			static int typeSize( Type type );
			static Type toType( QString typeName );
			QSharedPointer< Node > root;
		} data;
	};
	QMap< QString, SensorDesc > sensorDescMap;

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

	struct DeviceMemoryLoad
	{
		uint32_t totalRam;
		uint32_t staticAllocatedRam;
		uint32_t heapAllocatedRam;
		unsigned long long sdCardCapacity;
		unsigned long long sdCardFreeSpace;
	};

	MonitoringDataModel monitoringDataModel;
	QMenu* monitoringDataViewMenu;

	QTimer sensorNameTimer;
	QTreeWidget* popupSensorsListWidget;

	QXmlSchemaValidator configValidator;

	SynchronizationWindow* syncWindow;

	QVector< QString > deviceVPorts, deviceSensors, deviceSupportedSensors;
	enum class DeviceState { Unknown, IncorrectConfiguration, Working, Reconfiguration } deviceState;

	Ui::MarvieControllerClass ui;
};