#include "MonitoringDataTreeWidget.h"
#include <QHeaderView>

template< typename T >
QVector< T > getVector( const T* array, int size )
{
	QVector< T > v( size );
	for( int i = 0; i < size; ++i )
		v[i] = array[i];

	return v;
}

MonitoringDataTreeWidget::MonitoringDataTreeWidget( QWidget* parent /*= nullptr*/ ) : QTreeView( parent )
{
	setModel( &monitoringDataModel );
	header()->resizeSection( 0, 175 );
	header()->resizeSection( 1, 175 );
}

void MonitoringDataTreeWidget::setSensorDescriptionMap( QMap< QString, SensorDesc >* sensorDescMap )
{
	this->sensorDescMap = sensorDescMap;
}

void MonitoringDataTreeWidget::updateSensorData( QString text, QString sensorName, const uint8_t* data, QDateTime dateTime )
{
	MonitoringDataItem* item = monitoringDataModel.findItem( text );
	if( !item )
	{
		item = new MonitoringDataItem( text );
		attachSensorRelatedItems( item, sensorName );
		insertTopLevelItem( item );
	}
	item->setValue( dateTime );

	if( !sensorDescMap->contains( sensorName ) )
		return;
	auto desc = ( *sensorDescMap )[sensorName];
	if( !desc.data.root )
		return;
	std::function< void( MonitoringDataItem* item, SensorDesc::Data::Node* node ) > set = [&set, data]( MonitoringDataItem* item, SensorDesc::Data::Node* node )
	{
		int itemChildIndex = 0;
		for( auto i : node->childNodes )
		{
			switch( i->type )
			{
			case SensorDesc::Data::Type::Group:
			{
				set( item->child( itemChildIndex ), i );
				break;
			}
			case SensorDesc::Data::Type::Array:
			{
				if( i->childNodes[0]->type == SensorDesc::Data::Type::Char )
				{
					QString s;
					for( auto i2 : i->childNodes )
					{
						char c = data[i2->bias];
						if( c == 0 )
							break;
						s.append( c );
					}
					item->child( itemChildIndex )->setValue( s );
				}
				switch( i->childNodes[0]->type )
				{
				case SensorDesc::Data::Type::Int8:
					item->child( itemChildIndex )->setValue( getVector( reinterpret_cast< const int8_t* >( data + i->bias ), i->childNodes.size() ) );
					break;
				case SensorDesc::Data::Type::Uint8:
					item->child( itemChildIndex )->setValue( getVector( reinterpret_cast< const uint8_t* >( data + i->bias ), i->childNodes.size() ) );
					break;
				case SensorDesc::Data::Type::Int16:
					item->child( itemChildIndex )->setValue( getVector( reinterpret_cast< const int16_t* >( data + i->bias ), i->childNodes.size() ) );
					break;
				case SensorDesc::Data::Type::Uint16:
					item->child( itemChildIndex )->setValue( getVector( reinterpret_cast< const uint16_t* >( data + i->bias ), i->childNodes.size() ) );
					break;
				case SensorDesc::Data::Type::Int32:
					item->child( itemChildIndex )->setValue( getVector( reinterpret_cast< const int32_t* >( data + i->bias ), i->childNodes.size() ) );
					break;
				case SensorDesc::Data::Type::Uint32:
					item->child( itemChildIndex )->setValue( getVector( reinterpret_cast< const uint32_t* >( data + i->bias ), i->childNodes.size() ) );
					break;
				case SensorDesc::Data::Type::Int64:
					item->child( itemChildIndex )->setValue( getVector( reinterpret_cast< const int64_t* >( data + i->bias ), i->childNodes.size() ) );
					break;
				case SensorDesc::Data::Type::Uint64:
					item->child( itemChildIndex )->setValue( getVector( reinterpret_cast< const uint64_t* >( data + i->bias ), i->childNodes.size() ) );
					break;
				case SensorDesc::Data::Type::Float:
					item->child( itemChildIndex )->setValue( getVector( reinterpret_cast< const float* >( data + i->bias ), i->childNodes.size() ) );
					break;
				case SensorDesc::Data::Type::Double:
					item->child( itemChildIndex )->setValue( getVector( reinterpret_cast< const double* >( data + i->bias ), i->childNodes.size() ) );
					break;
				default:
					break;
				}
				set( item->child( itemChildIndex ), i );
				break;
			}
			case SensorDesc::Data::Type::Char:
				item->child( itemChildIndex )->setValue( *reinterpret_cast< const char* >( data + i->bias ) );
				break;
			case SensorDesc::Data::Type::Int8:
				item->child( itemChildIndex )->setValue( *reinterpret_cast< const int8_t* >( data + i->bias ) );
				break;
			case SensorDesc::Data::Type::Uint8:
				item->child( itemChildIndex )->setValue( *reinterpret_cast< const uint8_t* >( data + i->bias ) );
				break;
			case SensorDesc::Data::Type::Int16:
				item->child( itemChildIndex )->setValue( *reinterpret_cast< const int16_t* >( data + i->bias ) );
				break;
			case SensorDesc::Data::Type::Uint16:
				item->child( itemChildIndex )->setValue( *reinterpret_cast< const uint16_t* >( data + i->bias ) );
				break;
			case SensorDesc::Data::Type::Int32:
				item->child( itemChildIndex )->setValue( *reinterpret_cast< const int32_t* >( data + i->bias ) );
				break;
			case SensorDesc::Data::Type::Uint32:
				item->child( itemChildIndex )->setValue( *reinterpret_cast< const uint32_t* >( data + i->bias ) );
				break;
			case SensorDesc::Data::Type::Int64:
				item->child( itemChildIndex )->setValue( *reinterpret_cast< const int64_t* >( data + i->bias ) );
				break;
			case SensorDesc::Data::Type::Uint64:
				item->child( itemChildIndex )->setValue( *reinterpret_cast< const uint64_t* >( data + i->bias ) );
				break;
			case SensorDesc::Data::Type::Float:
				item->child( itemChildIndex )->setValue( *reinterpret_cast< const float* >( data + i->bias ) );
				break;
			case SensorDesc::Data::Type::Double:
				item->child( itemChildIndex )->setValue( *reinterpret_cast< const double* >( data + i->bias ) );
				break;
			default:
				break;
			}
			++itemChildIndex;
		}
	};
	set( item, desc.data.root.data() );
	monitoringDataModel.topLevelItemDataUpdated( item->childIndex() );
}

void MonitoringDataTreeWidget::updateAnalogData( uint id, const float* data, uint count )
{
	_updateAnalogData( id, data, count, nullptr );
}

void MonitoringDataTreeWidget::updateAnalogData( uint id, const float* data, uint count, QDateTime dateTime )
{
	_updateAnalogData( id, data, count, &dateTime );
}

void MonitoringDataTreeWidget::updateDiscreteData( uint id, uint64_t data, uint count )
{
	_updateDiscreteData( id, data, count, nullptr );
}

void MonitoringDataTreeWidget::updateDiscreteData( uint id, uint64_t data, uint count, QDateTime dateTime )
{
	_updateDiscreteData( id, data, count, &dateTime );
}

void MonitoringDataTreeWidget::removeAnalogData( int id )
{
	if( id == -1 )
	{
		QRegExp rx( "AI\\[\\d+\\]" );
		auto root = monitoringDataModel.rootItem();
		for( int i = 0; i < root->childCount(); )
		{
			if( rx.exactMatch( root->child( i )->name() ) )
				monitoringDataModel.removeRows( i, 1 );
			else
				++i;
		}
	}
	else
	{
		MonitoringDataItem* item = monitoringDataModel.findItem( QString( "AI[%1]" ).arg( id ) );
		if( item )
			monitoringDataModel.removeRows( item->childIndex(), 1 );
	}
}

void MonitoringDataTreeWidget::removeDiscreteData( int id )
{
	if( id == -1 )
	{
		QRegExp rx( "DI\\[\\d+\\]" );
		auto root = monitoringDataModel.rootItem();
		for( int i = 0; i < root->childCount(); )
		{
			if( rx.exactMatch( root->child( i )->name() ) )
				monitoringDataModel.removeRows( i, 1 );
			else
				++i;
		}
	}
	else
	{
		MonitoringDataItem* item = monitoringDataModel.findItem( QString( "DI[%1]" ).arg( id ) );
		if( item )
			monitoringDataModel.removeRows( item->childIndex(), 1 );
	}
}

void MonitoringDataTreeWidget::removeSensorData( QString text )
{
	MonitoringDataItem* item = monitoringDataModel.findItem( text );
	if( item )
		monitoringDataModel.removeRows( item->childIndex(), 1 );
}

void MonitoringDataTreeWidget::clear()
{
	monitoringDataModel.resetData();
}

void MonitoringDataTreeWidget::setHexadecimalOutput( bool enable )
{
	monitoringDataModel.setHexadecimalOutput( enable );
}

MonitoringDataModel* MonitoringDataTreeWidget::dataModel()
{
	return &monitoringDataModel;
}

void MonitoringDataTreeWidget::_updateAnalogData( uint id, const float* data, uint count, QDateTime* dateTime )
{
	QString itemName = QString( "AI[%1]" ).arg( id );
	MonitoringDataItem* item = monitoringDataModel.findItem( itemName );
	if( item )
		setADRelatedItemsCount( item, count );
	else
	{
		item = new MonitoringDataItem( itemName );
		attachADRelatedItems( item, count );
		insertTopLevelItem( item );
	}
	if( dateTime )
		item->setValue( *dateTime );
	else
		item->setValue( getVector( data, count ) );
	for( int i = 0; i < item->childCount(); ++i )
		item->child( i )->setValue( data[i] );
	monitoringDataModel.topLevelItemDataUpdated( item->childIndex() );
}

void MonitoringDataTreeWidget::_updateDiscreteData( uint id, uint64_t data, uint count, QDateTime* dateTime )
{
	QString itemName = QString( "DI[%1]" ).arg( id );
	MonitoringDataItem* item = monitoringDataModel.findItem( itemName );
	if( item )
		setADRelatedItemsCount( item, count );
	else
	{
		item = new MonitoringDataItem( itemName );
		attachADRelatedItems( item, count );
		insertTopLevelItem( item );
	}
	if( dateTime )
	{
		item->setValue( *dateTime );
	}
	else
	{
		if( count <= 8 )
			item->setValue( ( uint8_t )data );
		else if( count <= 16 )
			item->setValue( ( uint16_t )data );
		else if( count <= 32 )
			item->setValue( ( uint32_t )data );
		else
			item->setValue( ( uint64_t )data );
	}
	for( int i = 0; i < item->childCount(); ++i )
		item->child( i )->setValue( bool( data & ( 1 << i ) ) );
	monitoringDataModel.topLevelItemDataUpdated( item->childIndex() );
}

void MonitoringDataTreeWidget::attachSensorRelatedItems( MonitoringDataItem* sensorItem, QString sensorName )
{
	if( !sensorDescMap->contains( sensorName ) )
		return;
	auto desc = ( *sensorDescMap )[sensorName];
	if( !desc.data.root )
		return;

	static std::function< void( MonitoringDataItem* item, SensorDesc::Data::Node* node ) > add = []( MonitoringDataItem* item, SensorDesc::Data::Node* node )
	{
		for( auto i : node->childNodes )
		{
			switch( i->type )
			{
			case SensorDesc::Data::Type::Group:
			{
				MonitoringDataItem* child = new MonitoringDataItem( i->name );
				add( child, i );
				item->appendChild( child );
				break;
			}
			case SensorDesc::Data::Type::Array:
			{
				MonitoringDataItem* child = new MonitoringDataItem( i->name );
				for( int i2 = 0; i2 < i->childNodes.size(); ++i2 )
					child->appendChild( new MonitoringDataItem( QString( "[%1]" ).arg( i2 ) ) );
				item->appendChild( child );
				break;
			}
			default:
			{
				MonitoringDataItem* child = new MonitoringDataItem( i->name );
				item->appendChild( child );
				break;
			}
			}
		}
	};
	add( sensorItem, desc.data.root.data() );
}

void MonitoringDataTreeWidget::attachADRelatedItems( MonitoringDataItem* item, uint count )
{
	for( int i = 0; i < count; ++i )
		item->appendChild( new MonitoringDataItem( QString( "[%1]" ).arg( i ) ) );
}

void MonitoringDataTreeWidget::setADRelatedItemsCount( MonitoringDataItem* item, uint count )
{
	if( item->childCount() > count )
		monitoringDataModel.removeRows( count, item->childCount() - count, monitoringDataModel.index( item->childIndex(), 0 ) );
	else if( item->childCount() < count )
	{
		int n = item->childCount();
		monitoringDataModel.insertRows( n, count - n, monitoringDataModel.index( item->childIndex(), 0 ) );
		for( int i = n; i < count; ++i )
			item->child( i )->setName( QString( "[%1]" ).arg( i ) );
	}
}

void MonitoringDataTreeWidget::insertTopLevelItem( MonitoringDataItem* item )
{
	QRegExp aiExp( "AI\\[(\\d+)\\]" );
	QRegExp diExp( "DI\\[(\\d+)\\]" );
	QRegExp intExp( "^\\d+" );
	enum ItemType { DI, AI, Sensor };
	auto itemType = [&]( MonitoringDataItem* item, int& index )
	{
		if( diExp.exactMatch( item->name() ) )
		{
			index = diExp.cap( 1 ).toInt();
			return ItemType::DI;
		}
		if( aiExp.exactMatch( item->name() ) )
		{
			index = aiExp.cap( 1 ).toInt();
			return ItemType::AI;
		}
		index = -1;
		return ItemType::Sensor;
	};
	auto less = [&]( MonitoringDataItem* itemA, MonitoringDataItem* itemB ) -> bool
	{
		int indexA, indexB;
		ItemType typeA = itemType( itemA, indexA );
		ItemType typeB = itemType( itemB, indexB );
		if( typeA == typeB )
		{
			if( typeA == ItemType::AI || typeA == ItemType::DI )
				return indexA < indexB;
			int a, b;
			if( intExp.indexIn( itemA->name() ) == 0 )
			{
				a = intExp.cap().toInt();
				if( intExp.indexIn( itemB->name() ) == 0 )
				{
					b = intExp.cap().toInt();
					return a < b;
				}
			}
			return itemA->name() < itemB->name();
		}
		return typeA < typeB;
	};

	MonitoringDataItem* root = monitoringDataModel.rootItem();
	int index = qUpperBound( root->childConstBegin(), root->childConstEnd(), item, less ) - root->childConstBegin();

	monitoringDataModel.insertTopLevelItem( index, item );
}
