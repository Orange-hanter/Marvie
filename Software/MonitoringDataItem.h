#pragma once

#include <QList>
#include <QVector>
#include <QVariant>
#include <QDateTime>

class MonitoringDataItem
{
public:
	enum class ValueType {
		Invalid, Bool,
		Char, Int8, UInt8, Int16, UInt16, Int32, UInt32, Int64, UInt64, Float, Double,
		String, Int8Array, UInt8Array, Int16Array, UInt16Array, Int32Array, UInt32Array, Int64Array, UInt64Array, FloatArray, DoubleArray,
		DateTime
	};

	MonitoringDataItem( QString name );
	~MonitoringDataItem();

	QString name();
	QVariant value();
	int valueArraySize();
	ValueType type();
	QString typeName();
	static QString typeName( ValueType type );
	MonitoringDataItem* child( int index );
	QList< MonitoringDataItem* >::iterator childBegin();
	QList< MonitoringDataItem* >::const_iterator childConstBegin() const;
	QList< MonitoringDataItem* >::iterator childEnd();
	QList< MonitoringDataItem* >::const_iterator childConstEnd() const;
	MonitoringDataItem* parent();
	int childCount();
	int childIndex();

	void insertChild( int index, MonitoringDataItem* item );
	void appendChild( MonitoringDataItem* item );
	void removeChild( int index );
	void removeChilds( int begin, int count );
	void removeAllChildren();

	void setName( QString name );

	void setValue( bool value );
	void setValue( char value );
	void setValue( int8_t value );
	void setValue( uint8_t value );
	void setValue( int16_t value );
	void setValue( uint16_t value );
	void setValue( int32_t value );
	void setValue( uint32_t value );
	void setValue( int64_t value );
	void setValue( uint64_t value );
	void setValue( float value );
	void setValue( double value );

	void setValue( const QString& value );
	void setValue( QVector< int8_t > vect );
	void setValue( QVector< uint8_t > vect );
	void setValue( QVector< int16_t > vect );
	void setValue( QVector< uint16_t > vect );
	void setValue( QVector< int32_t > vect );
	void setValue( QVector< uint32_t > vect );
	void setValue( QVector< int64_t > vect );
	void setValue( QVector< uint64_t > vect );
	void setValue( QVector< float > vect );
	void setValue( QVector< double > vect );

	void setValue( const QDateTime& value );

private:
	QString _name;
	QVariant _value;
	ValueType _type;
	int _valueArraySize;
	QList< MonitoringDataItem* > _childItems;
	MonitoringDataItem* _parent;
};