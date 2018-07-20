#include "SensorErrorsModel.h"

SensorErrorsModel::SensorErrorsModel( QObject* parent /*= nullptr */ ) : QAbstractItemModel( parent )
{

}

SensorErrorsModel::~SensorErrorsModel()
{

}

void SensorErrorsModel::addSensorReadError( uint sensorId, QString sensorName, SensorError error, uint8_t errorCode, QDateTime date )
{
	removeSensorReadError( sensorId );
	beginInsertRows( QModelIndex(), 0, 0 );
	list.insert( 0, Item{ sensorId, sensorName, errorName( error ) + QString( "(%1)" ).arg( errorCode ), date.toString( Qt::DateFormat::ISODate ) } );
	endInsertRows();
}

void SensorErrorsModel::removeSensorReadError( uint sensorId )
{
	int index = 0;
	for( auto i = list.begin(); i != list.end(); ++i, ++index )
	{
		if( i->id == sensorId )
		{
			beginRemoveRows( QModelIndex(), index, index );
			list.erase( i );
			endRemoveRows();
			break;
		}
	}
}

void SensorErrorsModel::clear()
{
	beginResetModel();
	list.clear();
	endResetModel();
}

QVariant SensorErrorsModel::data( const QModelIndex &index, int role ) const
{
	if( role != Qt::DisplayRole && role != Qt::EditRole )
		return QVariant();

	if( index.column() == 0 )
		return list.at( index.row() ).id + 1;
	if( index.column() == 1 )
		return list.at( index.row() ).sensorName;
	if( index.column() == 2 )
		return list.at( index.row() ).error;
	if( index.column() == 3 )
		return list.at( index.row() ).date;
	return QVariant();
}

QVariant SensorErrorsModel::headerData( int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole */ ) const
{
	if( role != Qt::DisplayRole )
		return QVariant();
	if( orientation == Qt::Orientation::Horizontal )
	{
		if( section == 0 )
			return QChar( 8470 );
		else if( section == 1 )
			return "Name";
		else if( section == 2 )
			return "Error";
		else if( section == 3 )
			return "Date";
		return QVariant();
	}

	return QString( "%1" ).arg( section + 1 );
}

QModelIndex SensorErrorsModel::index( int row, int column, const QModelIndex &parent /*= QModelIndex() */ ) const
{
	if( parent.isValid() )
		return QModelIndex();

	return createIndex( row, column, nullptr );
}

QModelIndex SensorErrorsModel::parent( const QModelIndex &index ) const
{
	return QModelIndex();
}

int SensorErrorsModel::rowCount( const QModelIndex &parent /*= QModelIndex() */ ) const
{
	return list.size();
}

int SensorErrorsModel::columnCount( const QModelIndex &parent /*= QModelIndex() */ ) const
{
	return 4;
}

QString SensorErrorsModel::errorName( SensorError error )
{
	switch( error )
	{
	case SensorErrorsModel::SensorError::CrcError:
		return "CRC";
	case SensorErrorsModel::SensorError::NoResponseError:
		return "NoResp";
	default:
		return "";
	}
}
