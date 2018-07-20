#pragma once

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QDateTime>
#include <QVariant>

class SensorErrorsModel : public QAbstractItemModel
{
public:
	enum class SensorError { CrcError, NoResponseError };

	SensorErrorsModel( QObject* parent = nullptr );
	~SensorErrorsModel();

	void addSensorReadError( uint sensorId, QString sensorName, SensorError error, uint8_t errorCode, QDateTime date );
	void removeSensorReadError( uint sensorId );
	void clear();

	QVariant data( const QModelIndex &index, int role ) const override;
	QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;

	QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex() ) const override;
	QModelIndex parent( const QModelIndex &index ) const override;

	int rowCount( const QModelIndex &parent = QModelIndex() ) const override;
	int columnCount( const QModelIndex &parent = QModelIndex() ) const override;

	static QString errorName( SensorError error );

private:
	struct Item 
	{
		uint id;
		QString sensorName;
		QString error;
		QString date;
	};
	QList< Item > list;
};