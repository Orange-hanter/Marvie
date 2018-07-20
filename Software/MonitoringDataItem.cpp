#include "MonitoringDataItem.h"

MonitoringDataItem::MonitoringDataItem( QString name ) : _name( name ), _type( ValueType::Invalid ), _valueArraySize( -1 ), _parent( nullptr )
{

}

MonitoringDataItem::~MonitoringDataItem()
{
	qDeleteAll( _childItems );
	if( _parent )
		_parent->_childItems.removeOne( this );
}

QString MonitoringDataItem::name()
{
	return _name;
}

QVariant MonitoringDataItem::value()
{
	return _value;
}

int MonitoringDataItem::valueArraySize()
{
	return _valueArraySize;
}

MonitoringDataItem::ValueType MonitoringDataItem::type()
{
	return _type;
}

QString MonitoringDataItem::typeName()
{
	return typeName( _type );
}

QString MonitoringDataItem::typeName( ValueType type )
{
	switch( type )
	{
	case MonitoringDataItem::ValueType::Bool:
		return "Bool";
	case MonitoringDataItem::ValueType::Char:
		return "Char";
	case MonitoringDataItem::ValueType::Int8:
		return "Int8";
	case MonitoringDataItem::ValueType::UInt8:
		return "UInt8";
	case MonitoringDataItem::ValueType::Int16:
		return "Int16";
	case MonitoringDataItem::ValueType::UInt16:
		return "UInt16";
	case MonitoringDataItem::ValueType::Int32:
		return "Int32";
	case MonitoringDataItem::ValueType::UInt32:
		return "UInt32";
	case MonitoringDataItem::ValueType::Int64:
		return "Int64";
	case MonitoringDataItem::ValueType::UInt64:
		return "UInt64";
	case MonitoringDataItem::ValueType::Float:
		return "Float";
	case MonitoringDataItem::ValueType::Double:
		return "Double";
	case MonitoringDataItem::ValueType::String:
		return "String";
	case MonitoringDataItem::ValueType::Int8Array:
		return "Int8[]";
	case MonitoringDataItem::ValueType::UInt8Array:
		return "UInt8[]";
	case MonitoringDataItem::ValueType::Int16Array:
		return "Int16[]";
	case MonitoringDataItem::ValueType::UInt16Array:
		return "UInt16[]";
	case MonitoringDataItem::ValueType::Int32Array:
		return "Int32[]";
	case MonitoringDataItem::ValueType::UInt32Array:
		return "UInt32[]";
	case MonitoringDataItem::ValueType::Int64Array:
		return "Int64[]";
	case MonitoringDataItem::ValueType::UInt64Array:
		return "UInt64[]";
	case MonitoringDataItem::ValueType::FloatArray:
		return "Float[]";
	case MonitoringDataItem::ValueType::DoubleArray:
		return "Double[]";
	case MonitoringDataItem::ValueType::DateTime:
		return "DateTime";
	default:
		return "";
	}
}

MonitoringDataItem* MonitoringDataItem::child( int index )
{
	return _childItems[index];
}

MonitoringDataItem* MonitoringDataItem::parent()
{
	return _parent;
}

int MonitoringDataItem::childCount()
{
	return _childItems.count();
}

int MonitoringDataItem::childIndex()
{
	if( _parent )
		return _parent->_childItems.indexOf( this );
	return -1;
}

void MonitoringDataItem::insertChild( int index, MonitoringDataItem* item )
{
	_childItems.insert( index, item );
	item->_parent = this;
}

void MonitoringDataItem::appendChild( MonitoringDataItem* item )
{
	_childItems.append( item );
	item->_parent = this;
}

void MonitoringDataItem::removeChild( int index )
{
	MonitoringDataItem* item = _childItems.takeAt( index );
	item->_parent = nullptr;
	delete item;
}

void MonitoringDataItem::removeAllChildren()
{
	qDeleteAll( _childItems );
}

void MonitoringDataItem::setValue( bool value )
{
	_value.setValue( value );
	_type = ValueType::Bool;
	_valueArraySize = -1;
}

void MonitoringDataItem::setValue( char value )
{
	_value.setValue( value );
	_type = ValueType::Char;
	_valueArraySize = -1;
}

void MonitoringDataItem::setValue( int8_t value )
{
	_value.setValue( value );
	_type = ValueType::Int8;
	_valueArraySize = -1;
}

void MonitoringDataItem::setValue( uint8_t value )
{
	_value.setValue( value );
	_type = ValueType::UInt8;
	_valueArraySize = -1;
}

void MonitoringDataItem::setValue( int16_t value )
{
	_value.setValue( value );
	_type = ValueType::Int16;
	_valueArraySize = -1;
}

void MonitoringDataItem::setValue( uint16_t value )
{
	_value.setValue( value );
	_type = ValueType::UInt16;
	_valueArraySize = -1;
}

void MonitoringDataItem::setValue( int32_t value )
{
	_value.setValue( value );
	_type = ValueType::Int32;
	_valueArraySize = -1;
}

void MonitoringDataItem::setValue( uint32_t value )
{
	_value.setValue( value );
	_type = ValueType::UInt32;
	_valueArraySize = -1;
}

void MonitoringDataItem::setValue( int64_t value )
{
	_value.setValue( value );
	_type = ValueType::Int64;
	_valueArraySize = -1;
}

void MonitoringDataItem::setValue( uint64_t value )
{
	_value.setValue( value );
	_type = ValueType::UInt64;
	_valueArraySize = -1;
}

void MonitoringDataItem::setValue( float value )
{
	_value.setValue( value );
	_type = ValueType::Float;
	_valueArraySize = -1;
}

void MonitoringDataItem::setValue( double value )
{
	_value.setValue( value );
	_type = ValueType::Double;
	_valueArraySize = -1;
}

void MonitoringDataItem::setValue( const QString& value )
{
	_value.setValue( value );
	_type = ValueType::String;
	_valueArraySize = value.size();
}

void MonitoringDataItem::setValue( QVector< int8_t > vect )
{
	_value.setValue( vect );
	_type = ValueType::Int8Array;
	_valueArraySize = vect.size();
}

void MonitoringDataItem::setValue( QVector< uint8_t > vect )
{
	_value.setValue( vect );
	_type = ValueType::UInt8Array;
}

void MonitoringDataItem::setValue( QVector< int16_t > vect )
{
	_value.setValue( vect );
	_type = ValueType::Int16Array;
	_valueArraySize = vect.size();
}

void MonitoringDataItem::setValue( QVector< uint16_t > vect )
{
	_value.setValue( vect );
	_type = ValueType::UInt16Array;
	_valueArraySize = vect.size();
}

void MonitoringDataItem::setValue( QVector< int32_t > vect )
{
	_value.setValue( vect );
	_type = ValueType::Int32Array;
	_valueArraySize = vect.size();
}

void MonitoringDataItem::setValue( QVector< uint32_t > vect )
{
	_value.setValue( vect );
	_type = ValueType::UInt32Array;
	_valueArraySize = vect.size();
}

void MonitoringDataItem::setValue( QVector< int64_t > vect )
{
	_value.setValue( vect );
	_type = ValueType::Int64Array;
	_valueArraySize = vect.size();
}

void MonitoringDataItem::setValue( QVector< uint64_t > vect )
{
	_value.setValue( vect );
	_type = ValueType::UInt64Array;
	_valueArraySize = vect.size();
}

void MonitoringDataItem::setValue( QVector< float > vect )
{
	_value.setValue( vect );
	_type = ValueType::FloatArray;
	_valueArraySize = vect.size();
}

void MonitoringDataItem::setValue( QVector< double > vect )
{
	_value.setValue( vect );
	_type = ValueType::DoubleArray;
	_valueArraySize = vect.size();
}

void MonitoringDataItem::setValue( const QDateTime& value )
{
	_value.setValue( value );
	_type = ValueType::DateTime;
	_valueArraySize = -1;
}