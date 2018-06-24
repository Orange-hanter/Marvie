#include "VPortOverIpModel.h"
#include <QRegExp>

VPortOverIpModel::VPortOverIpModel( QObject *parent /*= 0 */ ) : QAbstractItemModel( parent )
{

}

VPortOverIpModel::~VPortOverIpModel()
{

}

void VPortOverIpModel::setModelData( const QStringList& addressList )
{
	beginResetModel();
	this->addressList.clear();
	QRegExp reg( "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5]):[0-9]+$" );
	for( const auto& i : addressList )
	{
		if( reg.exactMatch( i ) )
		{
			auto prms = i.split( ':' );
			int port = prms[1].toInt();
			if( port == 0 || port > 65535 )
				continue;
			this->addressList.append( Data{ prms[0], port } );
		}
	}
	endResetModel();
}

QStringList VPortOverIpModel::modelData()
{
	QStringList list;
	for( const auto& i : addressList )
		list.append( ( i.ip + ":%1" ).arg( i.port ) );
	return list;
}

QVariant VPortOverIpModel::data( const QModelIndex &index, int role ) const
{
	if( role != Qt::DisplayRole && role != Qt::EditRole )
		return QVariant();

	if( index.column() == 0 )
		return addressList.at( index.row() ).ip;
	return addressList.at( index.row() ).port;
}

QVariant VPortOverIpModel::headerData( int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole */ ) const
{
	if( role != Qt::DisplayRole )
		return QVariant();
	if( orientation == Qt::Orientation::Horizontal )
		return section == 0 ? "IP" : "Port";
	else
		return QString( "%1" ).arg( section + 1 );
}

QModelIndex VPortOverIpModel::index( int row, int column, const QModelIndex &parent /*= QModelIndex() */ ) const
{
	if( parent.isValid() )
		return QModelIndex();

	return createIndex( row, column, nullptr );
}

QModelIndex VPortOverIpModel::parent( const QModelIndex &index ) const
{
	return QModelIndex();
}

int VPortOverIpModel::rowCount( const QModelIndex &parent /*= QModelIndex() */ ) const
{
	return addressList.size();
}

int VPortOverIpModel::columnCount( const QModelIndex &parent /*= QModelIndex() */ ) const
{
	return 2;
}

Qt::ItemFlags VPortOverIpModel::flags( const QModelIndex &index ) const
{
	if( !index.isValid() )
		return 0;

	return Qt::ItemIsEditable | QAbstractItemModel::flags( index );
}

bool VPortOverIpModel::setData( const QModelIndex &index, const QVariant &value, int role /*= Qt::EditRole */ )
{
	if( role != Qt::EditRole || !index.isValid() ||
		index.row() >= addressList.size() || index.column() >= 2 )
		return false;

	if( index.column() == 0 )
	{
		QRegExp reg( "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$" );
		if( !reg.exactMatch( value.toString() ) )
			return false;
		addressList[index.row()].ip = value.toString();
	}
	else
		addressList[index.row()].port = value.toInt();
	dataChanged( index, index );

	return true;
}

bool VPortOverIpModel::setHeaderData( int section, Qt::Orientation orientation, const QVariant &value, int role /*= Qt::EditRole */ )
{
	return false;
}

bool VPortOverIpModel::insertColumns( int position, int columns, const QModelIndex &parent /*= QModelIndex() */ )
{
	return false;
}

bool VPortOverIpModel::removeColumns( int position, int columns, const QModelIndex &parent /*= QModelIndex() */ )
{
	return false;
}

bool VPortOverIpModel::insertRows( int position, int rows, const QModelIndex &parent /*= QModelIndex() */ )
{
	if( addressList.size() < position || rows == 0 )
		return false;

	beginInsertRows( parent, position, position + rows - 1 );
	while( rows-- )
		addressList.insert( position, Data{ "0.0.0.0", 42 } );
	endInsertRows();

	return true;
}

bool VPortOverIpModel::removeRows( int position, int rows, const QModelIndex &parent /*= QModelIndex() */ )
{
	if( addressList.size() < position + rows || position < 0 || rows == 0 )
		return false;

	beginRemoveRows( parent, position, position + rows - 1 );
	addressList.remove( position, rows );
	endRemoveRows();

	return true;
}
