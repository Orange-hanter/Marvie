#include "SensorDescription.h"
#include <assert.h>

int SensorDesc::Data::typeSize( Type type )
{
	switch( type )
	{
	case Type::Char:
	case Type::Int8:
	case Type::Uint8:
		return 1;
	case Type::Int16:
	case Type::Uint16:
		return 2;
	case Type::Int32:
	case Type::Uint32:
	case Type::Float:
		return 4;
	case Type::Int64:
	case Type::Uint64:
	case Type::Double:
		return 8;
	default:
		assert( true );
		return -1;
	}
}

SensorDesc::Data::Type SensorDesc::Data::toType( QString typeName )
{
	static const QMap< QString, SensorDesc::Data::Type > map = []() -> QMap< QString, SensorDesc::Data::Type >
	{
		QMap< QString, SensorDesc::Data::Type > map;
		map.insert( "char", SensorDesc::Data::Type::Char );
		map.insert( "int8", SensorDesc::Data::Type::Int8 );
		map.insert( "uint8", SensorDesc::Data::Type::Uint8 );
		map.insert( "int16", SensorDesc::Data::Type::Int16 );
		map.insert( "uint16", SensorDesc::Data::Type::Uint16 );
		map.insert( "int32", SensorDesc::Data::Type::Int32 );
		map.insert( "uint32", SensorDesc::Data::Type::Uint32 );
		map.insert( "int64", SensorDesc::Data::Type::Int64 );
		map.insert( "uint64", SensorDesc::Data::Type::Uint64 );
		map.insert( "float", SensorDesc::Data::Type::Float );
		map.insert( "double", SensorDesc::Data::Type::Double );
		map.insert( "unused", SensorDesc::Data::Type::Unused );
		map.insert( "group", SensorDesc::Data::Type::Group );
		map.insert( "array", SensorDesc::Data::Type::Array );
		map.insert( "groupArray", SensorDesc::Data::Type::GroupArray );
		return map;
	}();
	assert( map.contains( typeName ) );
	return map[typeName];
}
