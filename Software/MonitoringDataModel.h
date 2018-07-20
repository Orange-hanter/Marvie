#pragma once

#include <QAbstractItemModel>
#include "MonitoringDataItem.h"

class MonitoringDataModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	MonitoringDataModel();
	~MonitoringDataModel();

	void setRootItem( MonitoringDataItem* item );
	MonitoringDataItem* rootItem();
	void resetData();

	MonitoringDataItem* findItem( QString name );

	void insertTopLevelItem( int position, MonitoringDataItem* item );
	void topLevelItemDataUpdated( int position );

	void setHexadecimalOutput( bool enable );

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
	MonitoringDataItem * root;
	bool hexadecimal;
};