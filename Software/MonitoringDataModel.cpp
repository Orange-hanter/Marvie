#include "MonitoringDataModel.h"

MonitoringDataModel::MonitoringDataModel()
{
	root = new MonitoringDataItem( "GG" );
	hexadecimal = false;
}

MonitoringDataModel::~MonitoringDataModel()
{
	delete root;
}

void MonitoringDataModel::setRootItem( MonitoringDataItem* item )
{
	beginResetModel();
	root = item;
	endResetModel();
}

MonitoringDataItem* MonitoringDataModel::rootItem()
{
	return root;
}

MonitoringDataItem* MonitoringDataModel::findItem( QString name )
{
	for( int i = 0; i < root->childCount(); ++i )
	{
		MonitoringDataItem* item = root->child( i );
		if( item->name() == name )
			return item;
	}

	return nullptr;
}

void MonitoringDataModel::insertTopLevelItem( int position, MonitoringDataItem* item )
{
	beginInsertRows( QModelIndex(), position, position );
	root->insertChild( position, item );
	endInsertRows();
}

void MonitoringDataModel::topLevelItemDataUpdated( int position )
{
	dataChanged( index( position, 0 ), index( position, 2 ) );
}

void MonitoringDataModel::setHexadecimalOutput( bool enable )
{
	hexadecimal = enable;
	dataChanged( index( 0, 1 ), index( root->childCount(), 1 ) );
}

template< typename T >
QString printArray( const QVector< T >& v, bool hexadecimal = false )
{
	QString s( "{ " );
	for( auto i : v )
		s.append( printValue( "%1, ", i, hexadecimal ) );
	s[s.size() - 2] = ' ';
	s[s.size() - 1] = '}';

	return s;
}

template< typename T >
QString printValue( QString format, T v, bool hexadecimal = false )
{
	if( hexadecimal )
	{
		if( sizeof( T ) == 1 )
			return format.arg( QString( "0x" ) + QString( "%1" ).arg( ( uint )*reinterpret_cast< uint8_t* >( &v ), 2, 16, QChar( '0' ) ).toUpper() );
		if( sizeof( T ) == 2 )
			return format.arg( QString( "0x" ) + QString( "%1" ).arg( ( uint )*reinterpret_cast< uint16_t* >( &v ), 4, 16, QChar( '0' ) ).toUpper() );
		if( sizeof( T ) == 4 )
			return format.arg( QString( "0x" ) + QString( "%1" ).arg( ( uint )*reinterpret_cast< uint32_t* >( &v ), 8, 16, QChar( '0' ) ).toUpper() );
		if( sizeof( T ) == 8 )
			return format.arg( QString( "0x" ) + QString( "%1" ).arg( ( unsigned long long )*reinterpret_cast< uint64_t* >( &v ), 16, 16, QChar( '0' ) ).toUpper() );
	}
	return QString( format ).arg( v );
}


QVariant MonitoringDataModel::data( const QModelIndex &index, int role ) const
{
	if( !index.isValid() || role != Qt::DisplayRole && role != Qt::EditRole )
		return QVariant();

	if( index.column() == 0 )
		return static_cast< MonitoringDataItem* >( index.internalPointer() )->name();
	else if( index.column() == 1 )
	{
		QVariant v = static_cast< MonitoringDataItem* >( index.internalPointer() )->value();
		MonitoringDataItem::ValueType t = static_cast< MonitoringDataItem* >( index.internalPointer() )->type();
		switch( t )
		{
		case MonitoringDataItem::ValueType::Bool:
			return printValue( "%1", ( uint8_t )v.toBool(), hexadecimal );
		case MonitoringDataItem::ValueType::Char:
			if( hexadecimal )
				return printValue( "%1", v.toChar().toLatin1(), true );
			return QString( "\'%1\'" ).arg( v.toChar() );
		case MonitoringDataItem::ValueType::Int8:
			return printValue( "%1", ( int8_t )v.toInt(), hexadecimal );
		case MonitoringDataItem::ValueType::UInt8:
			return printValue( "%1", ( uint8_t )v.toInt(), hexadecimal );
		case MonitoringDataItem::ValueType::Int16:
			return printValue( "%1", ( int16_t )v.toInt(), hexadecimal );
		case MonitoringDataItem::ValueType::UInt16:
			return printValue( "%1", ( uint16_t )v.toInt(), hexadecimal );
		case MonitoringDataItem::ValueType::Int32:
			return printValue( "%1", v.toInt(), hexadecimal );
		case MonitoringDataItem::ValueType::UInt32:
			return printValue( "%1", v.toUInt(), hexadecimal );
		case MonitoringDataItem::ValueType::Int64:
			return printValue( "%1", v.toLongLong(), hexadecimal );
		case MonitoringDataItem::ValueType::UInt64:
			return printValue( "%1", v.toULongLong(), hexadecimal );
		case MonitoringDataItem::ValueType::Float:
			if( hexadecimal )
				return printValue( "%1", v.toFloat(), true );
			return QString( "%1" ).arg( v.toFloat(), 0, 'g', 8 );
		case MonitoringDataItem::ValueType::Double:
			if( hexadecimal )
				return printValue( "%1", v.toDouble(), true );
			return QString( "%1" ).arg( v.toDouble(), 0, 'g', 16 );
		case MonitoringDataItem::ValueType::String:
			return QString( "\"%1\"" ).arg( v.toString() );
		case MonitoringDataItem::ValueType::Int8Array:
			return printArray( v.value< QVector< int8_t > >(), hexadecimal );
		case MonitoringDataItem::ValueType::UInt8Array:
			return printArray( v.value< QVector< uint8_t > >(), hexadecimal );
		case MonitoringDataItem::ValueType::Int16Array:
			return printArray( v.value< QVector< int16_t > >(), hexadecimal );
		case MonitoringDataItem::ValueType::UInt16Array:
			return printArray( v.value< QVector< uint16_t > >(), hexadecimal );
		case MonitoringDataItem::ValueType::Int32Array:
			return printArray( v.value< QVector< int32_t > >(), hexadecimal );
		case MonitoringDataItem::ValueType::UInt32Array:
			return printArray( v.value< QVector< uint32_t > >(), hexadecimal );
		case MonitoringDataItem::ValueType::Int64Array:
			return printArray( v.value< QVector< int64_t > >(), hexadecimal );
		case MonitoringDataItem::ValueType::UInt64Array:
			return printArray( v.value< QVector< uint64_t > >(), hexadecimal );
		case MonitoringDataItem::ValueType::FloatArray:
			return printArray( v.value< QVector< float > >(), hexadecimal );
		case MonitoringDataItem::ValueType::DoubleArray:
			return printArray( v.value< QVector< double > >(), hexadecimal );
		case MonitoringDataItem::ValueType::DateTime:
			return v.toDateTime().toString( Qt::ISODate );
		default:
			return "";
		}
	}
	else if( index.column() == 2 )
	{
		MonitoringDataItem::ValueType t = static_cast< MonitoringDataItem* >( index.internalPointer() )->type();
		int n = static_cast< MonitoringDataItem* >( index.internalPointer() )->valueArraySize();
		QString typeName = MonitoringDataItem::typeName( t ).toLower();
		if( typeName == "datetime" )
			return "";
		else
		{
			if( t == MonitoringDataItem::ValueType::String )
				return typeName + QString( "(%1)" ).arg( n );
			else if( t == MonitoringDataItem::ValueType::Int8Array || t == MonitoringDataItem::ValueType::UInt8Array ||
					 t == MonitoringDataItem::ValueType::Int16Array || t == MonitoringDataItem::ValueType::UInt16Array ||
					 t == MonitoringDataItem::ValueType::Int32Array || t == MonitoringDataItem::ValueType::UInt32Array ||
					 t == MonitoringDataItem::ValueType::Int64Array || t == MonitoringDataItem::ValueType::UInt64Array ||
					 t == MonitoringDataItem::ValueType::FloatArray || t == MonitoringDataItem::ValueType::DoubleArray )
				return typeName.insert( typeName.size() - 1, QString( "%1" ).arg( n ) );
			return typeName;
		}
	}

	return QVariant();
}

QVariant MonitoringDataModel::headerData( int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole */ ) const
{
	if( orientation == Qt::Horizontal && role == Qt::DisplayRole )
	{
		if( section == 0 )
			return "Name";
		else if( section == 1 )
			return "Value";
		else if( section == 2 )
			return "Type";
	}

	return QVariant();
}

QModelIndex MonitoringDataModel::index( int row, int column, const QModelIndex &parent /*= QModelIndex() */ ) const
{
	MonitoringDataItem* parentItem;

	if( parent.isValid() )
		parentItem = static_cast< MonitoringDataItem* >( parent.internalPointer() );
	else
		parentItem = root;
	if( parentItem->childCount() > row )
		return createIndex( row, column, parentItem->child( row ) );
	else
		return QModelIndex();
}

QModelIndex MonitoringDataModel::parent( const QModelIndex &index ) const
{
	if( !index.isValid() )
		return QModelIndex();
	MonitoringDataItem* parentItem = static_cast< MonitoringDataItem* >( index.internalPointer() )->parent();
	if( parentItem != root )
		return createIndex( parentItem->childIndex(), 0, parentItem );
	return QModelIndex();
}

int MonitoringDataModel::rowCount( const QModelIndex &parent /*= QModelIndex() */ ) const
{
	if( parent.isValid() )
		return static_cast< MonitoringDataItem* >( parent.internalPointer() )->childCount();
	return root->childCount();
}

int MonitoringDataModel::columnCount( const QModelIndex &parent /*= QModelIndex() */ ) const
{
	return 3;
}

Qt::ItemFlags MonitoringDataModel::flags( const QModelIndex &index ) const
{
	return QAbstractItemModel::flags( index );
}

bool MonitoringDataModel::setData( const QModelIndex &index, const QVariant &value, int role /*= Qt::EditRole */ )
{
	return false;
}

bool MonitoringDataModel::setHeaderData( int section, Qt::Orientation orientation, const QVariant &value, int role /*= Qt::EditRole */ )
{
	return false;
}

bool MonitoringDataModel::insertColumns( int position, int columns, const QModelIndex &parent /*= QModelIndex() */ )
{
	return false;
}

bool MonitoringDataModel::removeColumns( int position, int columns, const QModelIndex &parent /*= QModelIndex() */ )
{
	return false;
}

bool MonitoringDataModel::insertRows( int position, int rows, const QModelIndex &parent /*= QModelIndex() */ )
{
	return false;
}

bool MonitoringDataModel::removeRows( int position, int rows, const QModelIndex &parent /*= QModelIndex() */ )
{
	return false;
}