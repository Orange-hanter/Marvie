#pragma once

#include <QTreeView>
#include "MonitoringDataItem.h"
#include "MonitoringDataModel.h"
#include "SensorDescription.h"

class MonitoringDataTreeWidget : public QTreeView
{
public:
	MonitoringDataTreeWidget( QWidget* parent = nullptr );

	void setSensorDescriptionMap( QMap< QString, SensorDesc >* sensorDescMap );

	void updateSensorData( QString text, QString sensorName, const uint8_t* data, QDateTime dateTime );
	void updateAnalogData( uint id, const float* data, uint count );
	void updateDiscreteData( uint id, uint64_t data, uint count );
	void updateAnalogData( uint id, const float* data, uint count, QDateTime dateTime );
	void updateDiscreteData( uint id, uint64_t data, uint count, QDateTime dateTime );
	void removeAnalogData( int id = -1 );
	void removeDiscreteData( int id = -1 );
	void removeSensorData( QString text );

	void clear();
	void setHexadecimalOutput( bool enable );
	MonitoringDataModel* dataModel();

private:
	void _updateAnalogData( uint id, const float* data, uint count, QDateTime* dateTime );
	void _updateDiscreteData( uint id, uint64_t data, uint count, QDateTime* dateTime );
	void attachSensorRelatedItems( MonitoringDataItem* sensorItem, QString sensorName );
	void attachADRelatedItems( MonitoringDataItem* item, uint count );
	void setADRelatedItemsCount( MonitoringDataItem* item, uint count );
	void insertTopLevelItem( MonitoringDataItem* item );

private:
	MonitoringDataModel monitoringDataModel;
	QMap< QString, SensorDesc >* sensorDescMap;
};