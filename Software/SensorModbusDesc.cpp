#include "SensorModbusDesc.h"
#include <assert.h>

SensorModbusDesc::SensorModbusDesc()
{

}

SensorModbusDesc::~SensorModbusDesc()
{

}

void SensorModbusDesc::addElement( QString name, quint32 offset, Type type )
{
	elements.append( Element( name, offset, type ) );
}

int SensorModbusDesc::elementCount() const
{
	return elements.size();
}

QString SensorModbusDesc::elementName( quint32 index ) const
{
	return elements.at( ( int )index ).name;
}

quint32 SensorModbusDesc::elementOffset( quint32 index ) const
{
	return elements.at( ( int )index ).offset;
}

SensorModbusDesc::Type SensorModbusDesc::elementType( quint32 index ) const
{
	return elements.at( ( int )index ).type;
}

int SensorModbusDesc::typeSize( Type type )
{
	switch( type )
	{
	case Type::Char:
	case Type::Int8:
	case Type::UInt8:
		return 1;
	case Type::Int16:
	case Type::UInt16:
		return 2;
	case Type::Int32:
	case Type::UInt32:
	case Type::Float:
		return 4;
	case Type::Int64:
	case Type::UInt64:
	case Type::Double:
	case Type::DateTime:
		return 8;
	default:
		assert( true );
		return -1;
	}
}

QString SensorModbusDesc::toString( Type type )
{
	switch( type )
	{
	case Type::Char:
		return "Char";
	case Type::Int8:
		return "Int8";
	case Type::UInt8:
		return "UInt8";
	case Type::Int16:
		return "Int16";
	case Type::UInt16:
		return "UInt16";
	case Type::Int32:
		return "Int32";
	case Type::UInt32:
		return "UInt32";
	case Type::Int64:
		return "Int64";
	case Type::UInt64:
		return "UInt64";
	case Type::Float:
		return "Float";
	case Type::Double:
		return "Double";	
	case Type::DateTime:
		return "DateTime";
	default:
		return "";
	}
}
