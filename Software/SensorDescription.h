#pragma once

#include <QMap>
#include <QString>
#include <QRegExp>
#include <QSharedPointer>

struct SensorDesc
{
	struct Settings
	{
		enum class Target { BR, SR } target;
		struct Parameter
		{
			enum class Type { Int, Float, Double, Bool, String, Enum } type;
			QString name;
			Parameter( Type type, QString name ) : type( type ), name( name ) {}
		};
		struct IntParameter : public Parameter
		{
			int defaultValue, min, max;
			IntParameter( QString name, int defaultValue, int min, int max ) : Parameter( Type::Int, name ), defaultValue( defaultValue ), min( min ), max( max ) {}
		};
		struct FloatParameter : public Parameter
		{
			float defaultValue, min, max;
			FloatParameter( QString name, float defaultValue, float min, float max ) : Parameter( Type::Float, name ), defaultValue( defaultValue ), min( min ), max( max ) {}
		};
		struct DoubleParameter : public Parameter
		{
			double defaultValue, min, max;
			DoubleParameter( QString name, double defaultValue, double min, double max ) : Parameter( Type::Double, name ), defaultValue( defaultValue ), min( min ), max( max ) {}
		};
		struct BoolParameter : public Parameter
		{
			bool defaultValue;
			BoolParameter( QString name, bool defaultValue ) : Parameter( Type::Bool, name ), defaultValue( defaultValue ) {}
		};
		struct StringParameter : public Parameter
		{
			QString defaultValue;
			QRegExp regExp;
			StringParameter( QString name, QString defaultValue, QRegExp regExp ) : Parameter( Type::String, name ), defaultValue( defaultValue ), regExp( regExp ) {}
		};
		struct EnumParameter : public Parameter
		{
			QMap< QString, int > map;
			QString defaultValue;
			EnumParameter( QString name, QMap< QString, int > map, QString defaultValue ) : Parameter( Type::Enum, name ), map( map ), defaultValue( defaultValue ) {}
		};
		QList< QSharedPointer< Parameter > > prmList;
	} settings;

	struct Data
	{
		enum class Type { Char, Int8, Uint8, Int16, Uint16, Int32, Uint32, Int64, Uint64, Float, Double, Unused, Array, Group, GroupArray };
		struct Node
		{
			Node( Type type, int bias, QString name = QString() ) : type( type ), bias( bias ), name( name ) {}
			~Node() { qDeleteAll( childNodes ); }
			QString name;
			Type type;
			int bias;
			QList< Node* > childNodes;
		};
		static int typeSize( Type type );
		static Type toType( QString typeName );
		QSharedPointer< Node > root;
		int size = -1;
	} data;
};