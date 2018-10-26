#pragma once

#include <QAbstractItemModel>
#include <QSharedPointer>
#include <QMap>
#include "SensorModbusDesc.h"

class ModbusRegMapModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	enum class OffsetUnits { Bytes, Words, DWords };

	ModbusRegMapModel();
	~ModbusRegMapModel();

	void setSensorModbusDescMap( QMap< QString, SensorModbusDesc >* sensorModbusDescMap );
	void setHexadecimalOutput( bool enabled );
	void setRelativeOffset( bool enabled );
	void setDisplayOffsetUnits( OffsetUnits units );

	void appendSensor( QString name, QString sensorTypeName, quint32 offset );
	void resetData();

	QVariant data( const QModelIndex &index, int role ) const override;
	QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;

	QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex() ) const override;
	QModelIndex parent( const QModelIndex &index ) const override;

	int rowCount( const QModelIndex &parent = QModelIndex() ) const override;
	int columnCount( const QModelIndex &parent = QModelIndex() ) const override;

	Qt::ItemFlags flags( const QModelIndex &index ) const override;
	bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole ) override;
	bool setHeaderData( int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole ) override;

	bool insertColumns( int position, int columns, const QModelIndex &parent = QModelIndex() ) override;
	bool removeColumns( int position, int columns, const QModelIndex &parent = QModelIndex() ) override;
	bool insertRows( int position, int rows, const QModelIndex &parent = QModelIndex() ) override;
	bool removeRows( int position, int rows, const QModelIndex &parent = QModelIndex() ) override;

private:
	struct Item 
	{
		QString name;
		QString sensorTypeName;
		quint32 offset;
	};
	QList< Item > items;
	QMap< QString, SensorModbusDesc >* descMap;
	bool hexadecimal, relativeOffset;
	OffsetUnits units;
};