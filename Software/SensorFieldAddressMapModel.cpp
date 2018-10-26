#include "SensorFieldAddressMapModel.h"

SensorFieldAddressMapModel::SensorFieldAddressMapModel()
{
	hexadecimal = false;
	relativeOffset = false;
	units = OffsetUnits::Bytes;
}

SensorFieldAddressMapModel::~SensorFieldAddressMapModel()
{

}

void SensorFieldAddressMapModel::setSensorUnfoldedDescMap( QMap< QString, SensorUnfoldedDesc >* descMap )
{
	this->descMap = descMap;
}

void SensorFieldAddressMapModel::setHexadecimalOutput( bool enabled )
{
	hexadecimal = enabled;
	dataChanged( index( 0, 1 ), index( items.size(), 1 ) );
}

void SensorFieldAddressMapModel::setRelativeOffset( bool enabled )
{
	relativeOffset = enabled;
	dataChanged( index( 0, 1 ), index( items.size(), 1 ) );
}

void SensorFieldAddressMapModel::setDisplayOffsetUnits( OffsetUnits units )
{
	this->units = units;
	dataChanged( index( 0, 1 ), index( items.size(), 1 ) );
}

void SensorFieldAddressMapModel::appendSensor( QString name, QString sensorTypeName, quint32 offset )
{
	if( !descMap->contains( sensorTypeName ) )
		return;
	beginInsertRows( QModelIndex(), items.size(), items.size() );
	items.append( Item{ name, sensorTypeName, offset } );
	endInsertRows();
	/*beginInsertRows( index( items.size() - 1, 0 ), 0, descMap->value( sensorTypeName ).elementCount() - 1 );
	endInsertRows();*/
}

void SensorFieldAddressMapModel::resetData()
{
	beginResetModel();
	items.clear();
	endResetModel();
}

QVariant SensorFieldAddressMapModel::data( const QModelIndex &index, int role ) const
{
	if( !index.isValid() || role != Qt::DisplayRole && role != Qt::EditRole )
		return QVariant();

	if( index.column() == 0 )
	{
		if( index.internalId() == ( quintptr )-1 )
			return items.at( index.row() ).name;
		return descMap->value( items.at( ( int )index.internalId() ).sensorTypeName ).elementName( index.row() );
	}
	else if( index.column() == 1 )
	{
		quint32 offset;
		if( index.internalId() == ( quintptr )-1 )
			offset = items.at( index.row() ).offset;
		else
		{
			offset = descMap->value( items.at( ( int )index.internalId() ).sensorTypeName ).elementOffset( index.row() );
			if( !relativeOffset )
				offset += items.at( ( int )index.internalId() ).offset;
		}
		if( units == OffsetUnits::Words )
			offset /= 2;
		else if( units == OffsetUnits::DWords )
			offset /= 4;
		if( hexadecimal )
			return QString( "0x" ) + QString( "%1" ).arg( offset, 0, 16 ).toUpper();
		return QString( "%1" ).arg( offset );
	}
	else if( index.column() == 2 )
	{
		if( index.internalId() == ( quintptr )-1 )
			return QString();
		const auto type = descMap->value( items.at( ( int )index.internalId() ).sensorTypeName ).elementType( index.row() );
		return SensorUnfoldedDesc::toString( type ).toLower();
	}

	return QVariant();
}

QVariant SensorFieldAddressMapModel::headerData( int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole */ ) const
{
	if( orientation == Qt::Horizontal && role == Qt::DisplayRole )
	{
		if( section == 0 )
			return "Name";
		else if( section == 1 )
			return "Offset";
		else if( section == 2 )
			return "Type";
	}

	return QVariant();
}

QModelIndex SensorFieldAddressMapModel::index( int row, int column, const QModelIndex &parent /*= QModelIndex() */ ) const
{
	if( !parent.isValid() )
	{
		if( items.size() > row )
			return createIndex( row, column, ( quintptr )-1 );
		else
			return QModelIndex();
	}
	if( parent.internalId() != ( quintptr )-1 )
		return QModelIndex();
	if( descMap->value( items.at( parent.row() ).sensorTypeName ).elementCount() > row )
		return createIndex( row, column, parent.row() );
	else
		return QModelIndex();
}

QModelIndex SensorFieldAddressMapModel::parent( const QModelIndex &index ) const
{
	if( !index.isValid() || index.internalId() == ( quintptr )-1 )
		return QModelIndex();
	return createIndex( ( int )index.internalId(), 0, ( quintptr )-1 );
}

int SensorFieldAddressMapModel::rowCount( const QModelIndex &parent /*= QModelIndex() */ ) const
{
	if( !parent.isValid() )
		return items.size();
	if( parent.internalId() == ( quintptr )-1 )
		return descMap->value( items.at( parent.row() ).sensorTypeName ).elementCount();
	return 0;
}

int SensorFieldAddressMapModel::columnCount( const QModelIndex &parent /*= QModelIndex() */ ) const
{
	return 3;
}

Qt::ItemFlags SensorFieldAddressMapModel::flags( const QModelIndex &index ) const
{
	return QAbstractItemModel::flags( index );
}

bool SensorFieldAddressMapModel::setData( const QModelIndex &index, const QVariant &value, int role /*= Qt::EditRole */ )
{
	return false;
}

bool SensorFieldAddressMapModel::setHeaderData( int section, Qt::Orientation orientation, const QVariant &value, int role /*= Qt::EditRole */ )
{
	return false;
}

bool SensorFieldAddressMapModel::insertColumns( int position, int columns, const QModelIndex &parent /*= QModelIndex() */ )
{
	return false;
}

bool SensorFieldAddressMapModel::removeColumns( int position, int columns, const QModelIndex &parent /*= QModelIndex() */ )
{
	return false;
}

bool SensorFieldAddressMapModel::insertRows( int position, int rows, const QModelIndex &parent /*= QModelIndex() */ )
{
	return false;
}

bool SensorFieldAddressMapModel::removeRows( int position, int rows, const QModelIndex &parent /*= QModelIndex() */ )
{
	return false;
}
