#pragma once

#include <QStringList>

class SensorModbusDesc
{
public:
	enum class Type { Char, Int8, UInt8, Int16, UInt16, Int32, UInt32, Int64, UInt64, Float, Double, DateTime };

	SensorModbusDesc();
	~SensorModbusDesc();

	void addElement( QString name, quint32 offset, Type type );
	int elementCount() const;
	QString elementName( quint32 index ) const;
	quint32 elementOffset( quint32 index ) const;
	Type elementType( quint32 index ) const;

	static int typeSize( Type type );
	static QString toString( Type type );

private:
	struct Element
	{
		inline Element() : offset( 0 ), type( Type::Char ) {}
		inline Element( QString name, quint32 offset, Type type ) : name( name ), offset( offset ), type( type ) {}
		QString name;
		quint32 offset;
		Type type;
	};
	QList< Element > elements;
};