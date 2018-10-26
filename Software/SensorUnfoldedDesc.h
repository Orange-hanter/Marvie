#pragma once

#include <QStringList>

class SensorUnfoldedDesc
{
public:
	enum class Type { Char, Int8, Uint8, Int16, Uint16, Int32, Uint32, Int64, Uint64, Float, Double, DateTime };

	SensorUnfoldedDesc();
	~SensorUnfoldedDesc();

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