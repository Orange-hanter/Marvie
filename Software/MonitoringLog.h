#pragma once

#include <QtCore>

class MonitoringLog
{
public:
	class Entry
	{
	public:
		enum class Type { DigitInputsEntry, AnalogInputsEntry, SensorEntry };

		Entry( Type t ) : t( t ) {}
		virtual ~Entry() {}
		inline Type type() { return t; }

	private:
		Type t;
	};
	class DigitInputsEntry : public Entry
	{
	public:
		DigitInputsEntry( const quint32* data, quint8 blockFlags );
		~DigitInputsEntry();

		inline quint32 data( quint32 blockId ) const { return d[blockId]; }
		inline bool isBlockPresent( quint32 blockId ) { return blockFlags & ( 1 << blockId ); }

	private:
		quint32* d;
		quint8 blockFlags;
	};
	class AnalogInputsEntry : public Entry
	{
	public:
		struct Desc
		{
			uint16_t b0 : 2;
			uint16_t b1 : 2;
			uint16_t b2 : 2;
			uint16_t b3 : 2;
			uint16_t b4 : 2;
			uint16_t b5 : 2;
			uint16_t b6 : 2;
			uint16_t b7 : 2;
		};
		AnalogInputsEntry( const float* data, Desc desc );
		~AnalogInputsEntry();

		inline const float* data( quint32 blockId ) const { return d[blockId]; }
		inline quint32 channelsCount( quint32 blockId ) { return ( ( ( *( quint16* )&desc ) >> ( blockId * 2 ) ) & 0x03 ) * 8; }

	private:
		float** d;
		Desc desc;
	};
	class SensorEntry : public Entry
	{
	public:
		SensorEntry( QByteArray data );

		inline const char* data() const { return d.constData(); }

	private:
		QByteArray d;
	};

	class DayGroup;
	class NameGroup;
	class EntryInfo
	{
		friend class DayGroup;
		QDate entryDate;
		const QMap< qint32, QSharedPointer< Entry > >& dayGroup;
		QMap< qint32, QSharedPointer< Entry > >::const_iterator entryIterator;
		inline constexpr EntryInfo( const QDate& entryDate,
									const QMap< qint32, QSharedPointer< Entry > >& dayGroup,
									const QMap< qint32, QSharedPointer< Entry > >::const_iterator& iterator )
			: entryDate( entryDate ), dayGroup( dayGroup ), entryIterator( iterator ) {}

	public:
		inline bool isValid() { return entryIterator != dayGroup.end(); }
		inline QDate date() { return entryDate; }
		inline QTime time() { return QTime::fromMSecsSinceStartOfDay( entryIterator.key() ); }
		inline QDateTime dateTime() { return QDateTime( entryDate, time() ); }
		inline QSharedPointer< Entry > entry() { return entryIterator.value(); }

		EntryInfo nextEntry();
		EntryInfo previousEntry();
	};
	class DayGroup
	{
		friend class NameGroup;
		QMap< quint64, QMap< qint32, QSharedPointer< Entry > > >& nameGroup;
		QMap< quint64, QMap< qint32, QSharedPointer< Entry > > >::iterator groupIterator;
		inline constexpr DayGroup( QMap< quint64, QMap< qint32, QSharedPointer< Entry > > >& nameGroup, const QMap< quint64, QMap< qint32, QSharedPointer< Entry > > >::iterator& groupIterator )
			: nameGroup( nameGroup ), groupIterator( groupIterator ) {}

	public:
		inline bool isValid() { return groupIterator != nameGroup.end(); }
		inline QDate date() { return QDate::fromJulianDay( groupIterator.key() ); }
		QVector< QTime > timestamps();
		EntryInfo nearestEntry( const QTime& time );
		EntryInfo lastEntry();
	};
	class NameGroup
	{
		friend class MonitoringLog;
		MonitoringLog& logData;
		QMap< QString, QMap< quint64, QMap< qint32, QSharedPointer< Entry > > > >::iterator groupIterator;
		inline constexpr NameGroup( MonitoringLog& logData, const QMap< QString, QMap< quint64, QMap< qint32, QSharedPointer< Entry > > > > ::iterator& groupIterator )
			: logData( logData ), groupIterator( groupIterator ) {}

	public:
		inline bool isValid() { return groupIterator != logData.map.end(); }
		DayGroup dayGroup( const QDate& date );
	};

	MonitoringLog();
	~MonitoringLog();

	struct SensorDesc 
	{
		QString sensorName;
		quint32 dataSize;
	};
	void setAvailableSensors( QList< SensorDesc > list );
	bool open( QString rootPath );
	void close();
	void clear();

	inline QStringList availableGroupNames() { return map.keys(); }
	inline NameGroup nameGroup( QString name ) { return NameGroup( *this, map.find( name ) ); }
	inline QDate minimumDate() { return minDate; }
	inline QDate maximumDate() { return maxDate; }

private:
	void load( QMap< QString, QMap< quint64, QMap< qint32, QSharedPointer< Entry > > > >::iterator groupIterator, const QDate& );

private:
	QMap< QString, quint32 > sensorDataSizeMap;
	QString rootPath;
	QMap< QString, QMap< quint64, QMap< qint32, QSharedPointer< Entry > > > > map;
	QDate minDate, maxDate;
};