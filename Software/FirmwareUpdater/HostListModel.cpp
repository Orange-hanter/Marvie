#include "HostListModel.h"

HostListModel::HostListModel()
{
	editMode = true;
}

HostListModel::~HostListModel()
{

}

void HostListModel::setEditMode( bool enabled )
{
	if( editMode == enabled )
		return;

	editMode = enabled;
	if( editMode )
	{
		beginRemoveColumns( QModelIndex(), 1, 2 );
		endRemoveColumns();
		headerDataChanged( Qt::Horizontal, 0, 2 );
	}
	else
	{
		beginInsertColumns( QModelIndex(), 1, 2 );
		endInsertColumns();
		headerDataChanged( Qt::Horizontal, 0, 4 );
	}
}

void HostListModel::appendHostGroup( const HostGroup& hostList )
{
	beginInsertRows( QModelIndex(), list.size(), list.size() );
	list.append( hostList );
	endInsertRows();
}

const QList< HostListModel::HostGroup > HostListModel::hostGroupList() const
{
	return list;
}

void HostListModel::setHostVersions( int groupIndex, int hostIndex, QString firmwareVersion, QString bootloaderVersion )
{
	if( list.size() <= groupIndex )
		return;
	auto& group = list[groupIndex];
	if( group.hostList.size() <= hostIndex )
		return;

	group.hostList[hostIndex].firmwareVersion = firmwareVersion;
	group.hostList[hostIndex].bootloaderVersion = bootloaderVersion;
	dataChanged( index( hostIndex, 1, index( groupIndex, 1 ) ), index( hostIndex, 2, index( groupIndex, 2 ) ) );
}

void HostListModel::setHostProgress( int groupIndex, int hostIndex, int progress )
{
	if( list.size() <= groupIndex )
		return;
	auto& group = list[groupIndex];
	if( group.hostList.size() <= hostIndex )
		return;

	group.hostList[hostIndex].progress = progress;
	auto id = index( hostIndex, 3, index( groupIndex, 3 ) );
	dataChanged( id, id );
}

void HostListModel::setHostState( int groupIndex, int hostIndex, QString state )
{
	if( list.size() <= groupIndex )
		return;
	auto& group = list[groupIndex];
	if( group.hostList.size() <= hostIndex )
		return;

	group.hostList[hostIndex].state = state;
	auto id = index( hostIndex, 4, index( groupIndex, 4 ) );
	dataChanged( id, id );
}

void HostListModel::clear()
{
	beginResetModel();
	list.clear();
	endResetModel();
}

QVariant HostListModel::data( const QModelIndex &index, int role ) const
{
	if( !index.isValid() || role != Qt::DisplayRole && role != Qt::EditRole )
		return QVariant();

	if( editMode )
	{
		if( index.column() == 0 )
		{
			if( index.internalId() == ( quintptr )-1 )
				return list.at( index.row() ).title;
			return list.at( ( int )index.internalId() ).hostList.at( index.row() ).addr.toString();
		}
		else if( index.column() == 1 )
		{
			if( index.internalId() == ( quintptr )-1 )
				return QVariant();
			return list.at( ( int )index.internalId() ).hostList.at( index.row() ).password;
		}
		else if( index.column() == 2 )
		{
			if( index.internalId() == ( quintptr )-1 )
				return  QVariant();
			return list.at( ( int )index.internalId() ).hostList.at( index.row() ).info;
		}
	}
	else
	{
		if( index.column() == 0 )
		{
			if( index.internalId() == ( quintptr )-1 )
				return list.at( index.row() ).title;
			return list.at( ( int )index.internalId() ).hostList.at( index.row() ).addr.toString();
		}
		else if( index.column() == 1 )
		{
			if( index.internalId() == ( quintptr )-1 )
				return QVariant();
			return list.at( ( int )index.internalId() ).hostList.at( index.row() ).firmwareVersion;
		}
		else if( index.column() == 2 )
		{
			if( index.internalId() == ( quintptr )-1 )
				return QVariant();
			return list.at( ( int )index.internalId() ).hostList.at( index.row() ).bootloaderVersion;
		}
		else if( index.column() == 3 )
		{
			if( index.internalId() == ( quintptr )-1 )
				return QVariant();
			return list.at( ( int )index.internalId() ).hostList.at( index.row() ).progress;
		}
		else if( index.column() == 4 )
		{
			if( index.internalId() == ( quintptr )-1 )
				return  QVariant();
			return list.at( ( int )index.internalId() ).hostList.at( index.row() ).state;
		}
	}

	return QVariant();
}

QVariant HostListModel::headerData( int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole */ ) const
{
	if( orientation == Qt::Horizontal && role == Qt::DisplayRole )
	{
		if( editMode )
		{
			if( section == 0 )
				return "Address";
			else if( section == 1 )
				return "Password";
			else if( section == 2 )
				return "Info";
		}
		if( section == 0 )
			return "Address";
		else if( section == 1 )
			return "Firmware";
		else if( section == 2 )
			return "Bootloader";
		else if( section == 3 )
			return "Progress";
		else if( section == 4 )
			return "State";
	}

	return QVariant();
}

QModelIndex HostListModel::index( int row, int column, const QModelIndex &parent /*= QModelIndex() */ ) const
{
	if( !parent.isValid() )
	{
		if( list.size() > row )
			return createIndex( row, column, ( quintptr )-1 );
		else
			return QModelIndex();
	}
	if( parent.internalId() != ( quintptr )-1 )
		return QModelIndex();
	if( list.at( parent.row() ).hostList.size() > row )
		return createIndex( row, column, parent.row() );
	else
		return QModelIndex();
}

QModelIndex HostListModel::parent( const QModelIndex &index ) const
{
	if( !index.isValid() || index.internalId() == ( quintptr )-1 )
		return QModelIndex();
	return createIndex( ( int )index.internalId(), 0, ( quintptr )-1 );
}

int HostListModel::rowCount( const QModelIndex &parent /*= QModelIndex() */ ) const
{
	if( !parent.isValid() )
		return list.size();
	if( parent.internalId() == ( quintptr )-1 )
		return list.at( parent.row() ).hostList.size();
	return 0;
}

int HostListModel::columnCount( const QModelIndex &parent /*= QModelIndex() */ ) const
{
	if( editMode )
		return 3;
	else
		return 5;
}

Qt::ItemFlags HostListModel::flags( const QModelIndex &index ) const
{
	//if( editMode )
	//	return QAbstractItemModel::flags( index ) | Qt::ItemFlag::ItemIsEditable;
	return QAbstractItemModel::flags( index ) & ~Qt::ItemFlag::ItemIsEditable;
}

bool HostListModel::setData( const QModelIndex &index, const QVariant &value, int role /*= Qt::EditRole */ )
{
	return false;
}

bool HostListModel::setHeaderData( int section, Qt::Orientation orientation, const QVariant &value, int role /*= Qt::EditRole */ )
{
	return false;
}

bool HostListModel::insertColumns( int position, int columns, const QModelIndex &parent /*= QModelIndex() */ )
{
	return false;
}

bool HostListModel::removeColumns( int position, int columns, const QModelIndex &parent /*= QModelIndex() */ )
{
	return false;
}

bool HostListModel::insertRows( int position, int rows, const QModelIndex &parent /*= QModelIndex() */ )
{
	return false;
}

bool HostListModel::removeRows( int position, int rows, const QModelIndex &parent /*= QModelIndex() */ )
{
	return false;
}
