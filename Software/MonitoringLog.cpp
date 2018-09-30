#include "MonitoringLog.h"
#include "Crc32SW.h"
#include "../Firmware/Core/DateTime.h"
#include <assert.h>

uint32_t numberOfSetBits( uint32_t i )
{
	i = i - ( ( i >> 1 ) & 0x55555555 );
	i = ( i & 0x33333333 ) + ( ( i >> 2 ) & 0x33333333 );
	return ( ( ( i + ( i >> 4 ) ) & 0x0F0F0F0F ) * 0x01010101 ) >> 24;
}

MonitoringLog::MonitoringLog()
{

}

MonitoringLog::~MonitoringLog()
{

}

void MonitoringLog::setAvailableSensors( QList< SensorDesc > list )
{
	sensorDataSizeMap.clear();
	map.clear();
	for( auto& e : list )
		sensorDataSizeMap[e.sensorName] = e.dataSize;
}

bool MonitoringLog::open( QString rootPath )
{
	clear();
	bool res = false;
	bool flag = false;
	this->rootPath = rootPath;
	auto list = QDir( rootPath ).entryList( QDir::Filter::Dirs | QDir::Filter::NoDotAndDotDot );
	for( auto& yearStr : list )
	{
		bool ok;
		int year = yearStr.toInt( &ok );
		if( !ok || year < 1980 || year > 2118 )
			continue;
		QString yearPath = rootPath + "/" + yearStr;
		auto list = QDir( yearPath ).entryList( QDir::Filter::Dirs | QDir::Filter::NoDotAndDotDot );
		for( auto& monthStr : list )
		{
			int month = monthStr.toInt( &ok );
			if( !ok || month < 1 || month > 12 )
				continue;
			QString monthPath = yearPath + "/" + monthStr;
			auto list = QDir( monthPath ).entryList( QDir::Filter::Dirs | QDir::Filter::NoDotAndDotDot );
			for( auto& dayStr : list )
			{
				int day = dayStr.toInt( &ok );
				if( !ok || day < 1 || day > QDate( year, month, 1 ).daysInMonth() )
					continue;

				if( flag )
				{
					QDate date( year, month, day );
					if( date < minDate )
						minDate = date;
					else if( date > maxDate )
						maxDate = date;
				}
				else
				{
					flag = true;
					minDate = QDate( year, month, day );
					maxDate = minDate;
				}

				QString dayPath = monthPath + "/" + dayStr;
				auto list = QDir( dayPath ).entryList( QStringList( "*.bin" ), QDir::Filter::Files );
				for( auto& entryStr : list )
				{
					if( entryStr == "AI.bin" )
						map["AI"], res = true;
					else if( entryStr == "DI.bin" )
						map["DI"], res = true;
					else
					{
						int id = entryStr.lastIndexOf( '.', -5 );
						if( id == -1 )
							continue;
						QString sensorName = entryStr.mid( id + 1, entryStr.size() - id - 5 );
						if( sensorDataSizeMap.contains( sensorName ) )
							map[entryStr.left( entryStr.size() - 4 )];
						res = true;
					}
				}
			}
		}
	}

	if( !res )
		rootPath = "";
	return res;
}

void MonitoringLog::close()
{
	rootPath = "";
}

void MonitoringLog::clear()
{
	map.clear();
}

void MonitoringLog::load( QMap< QString, QMap< quint64, QMap< qint32, QSharedPointer< Entry > > > >::iterator groupIterator, const QDate& date )
{
	if( rootPath.isEmpty() || date < minDate || date > maxDate )
		return;
	QMap< qint32, QSharedPointer< Entry > >& dayMap = groupIterator.value()[date.toJulianDay()];
	QString path = rootPath + QString( "/%1/%2/%3/" ).arg( date.year() ).arg( date.month() ).arg( date.day() );
	if( groupIterator.key() == "AI" )
	{
		QFile file( path + "AI.bin" );
		if( !file.open( QIODevice::ReadOnly ) )
			return;
		QByteArray data = file.readAll();
		int index = -1;
		while( ( index = data.indexOf( "_log", index + 1 ) ) != -1 )
		{
			const DateTime* dateTime = ( const DateTime* )( data.constData() + index + 4 );
			AnalogInputsEntry::Desc desc = *( const AnalogInputsEntry::Desc* )( data.constData() + index + 4 + sizeof( DateTime ) );
			uint32_t count = ( desc.b0 + desc.b1 + desc.b2 + desc.b3 + desc.b4 + desc.b5 + desc.b6 + desc.b7 ) * 8;
			uint32_t crc = Crc32SW::crc( ( const uint8_t* )data.constData() + index, 4 + sizeof( DateTime ) + 2 + count * 4, 0 );
			if( count == 0 || crc != *( const uint32_t* )( data.constData() + index + 4 + sizeof( DateTime ) + 2 + count * 4 ) )
				continue;
			assert( dateTime->date().year() == date.year() && dateTime->date().month() == date.month() && dateTime->date().day() == date.day() );
			AnalogInputsEntry* entry = new AnalogInputsEntry( ( const float* )( data.constData() + index + 4 + sizeof( DateTime ) + 2 ), desc );
			dayMap[dateTime->time().msecsSinceStartOfDay()] = QSharedPointer< Entry >( entry );

			index += 4 + sizeof( DateTime ) + 2 + count * 4 + 4 - 1;
		}
	}
	else if( groupIterator.key() == "DI" )
	{
		QFile file( path + "DI.bin" );
		if( !file.open( QIODevice::ReadOnly ) )
			return;
		QByteArray data = file.readAll();
		int index = -1;
		while( ( index = data.indexOf( "_log", index + 1 ) ) != -1 )
		{
			const DateTime* dateTime = ( const DateTime* )( data.constData() + index + 4 );
			uint8_t flags = *( const uint8_t* )( data.constData() + index + 4 + sizeof( DateTime ) );
			uint32_t count = numberOfSetBits( flags );
			uint32_t crc = Crc32SW::crc( ( const uint8_t* )data.constData() + index, 4 + sizeof( DateTime ) + 1 + count * 4, 0 );
			if( count == 0 || crc != *( const uint32_t* )( data.constData() + index + 4 + sizeof( DateTime ) + 1 + count * 4 ) )
				continue;
			assert( dateTime->date().year() == date.year() && dateTime->date().month() == date.month() && dateTime->date().day() == date.day() );
			DigitInputsEntry* entry = new DigitInputsEntry( ( const uint32_t* )( data.constData() + index + 4 + sizeof( DateTime ) + 1 ), flags );
			dayMap[dateTime->time().msecsSinceStartOfDay()] = QSharedPointer< Entry >( entry );

			index += 4 + sizeof( DateTime ) + 1 + count * 4 + 4 - 1;
		}
	}
	else
	{
		QString sensorName = groupIterator.key().right( groupIterator.key().size() - groupIterator.key().lastIndexOf( '.' ) - 1 );
		quint32 dataSize = sensorDataSizeMap[sensorName];
		QFile file( path + groupIterator.key() + ".bin" );
		if( !file.open( QIODevice::ReadOnly ) )
			return;
		QByteArray data = file.readAll();
		int index = -1;
		while( ( index = data.indexOf( "_log", index + 1 ) ) != -1 )
		{
			const DateTime* dateTime = ( const DateTime* )( data.constData() + index + 4 );
			uint32_t crc = Crc32SW::crc( ( const uint8_t* )data.constData() + index, 4 + sizeof( DateTime ) + dataSize, 0 );
			if( crc != *( const uint32_t* )( data.constData() + index + 4 + sizeof( DateTime ) + dataSize ) )
				continue;
			assert( dateTime->date().year() == date.year() && dateTime->date().month() == date.month() && dateTime->date().day() == date.day() );
			SensorEntry* entry = new SensorEntry( QByteArray( data.constData() + index + 4 + sizeof( DateTime ), dataSize ) );
			dayMap[dateTime->time().msecsSinceStartOfDay()] = QSharedPointer< Entry >( entry );

			index += 4 + sizeof( DateTime ) + dataSize + 4 - 1;
		}
	}
}

MonitoringLog::DayGroup MonitoringLog::NameGroup::dayGroup( const QDate& date )
{
	auto jday = date.toJulianDay();
	if( ( *groupIterator ).contains( jday ) )
		return DayGroup( *groupIterator, ( *groupIterator ).find( jday ) );

	logData.load( groupIterator, date );
	return DayGroup( *groupIterator, ( *groupIterator ).find( jday ) );
}

QVector< QTime > MonitoringLog::DayGroup::timestamps()
{
	auto keys = ( *groupIterator ).keys();
	QVector< QTime > v( keys.size() );
	int index = 0;
	for( auto& e : keys )
		v[index++] = QTime::fromMSecsSinceStartOfDay( e );

	return v;
}

MonitoringLog::EntryInfo MonitoringLog::EntryInfo::nextEntry()
{
	if( entryIterator == dayGroup.end() - 1 )
		return EntryInfo( *this );
	return EntryInfo( entryDate, dayGroup, entryIterator + 1 );
}

MonitoringLog::EntryInfo MonitoringLog::EntryInfo::previousEntry()
{
	if( entryIterator == dayGroup.begin() )
		return EntryInfo( *this );
	return EntryInfo( entryDate, dayGroup, entryIterator - 1 );
}

MonitoringLog::EntryInfo MonitoringLog::DayGroup::nearestEntry( const QTime& time )
{
	int msec = time.msecsSinceStartOfDay();
	auto i = ( *groupIterator ).lowerBound( msec );
	if( i == ( *groupIterator ).end() )
	{
		if( ( *groupIterator ).isEmpty() )
			return EntryInfo( QDate(), ( *groupIterator ), ( *groupIterator ).constEnd() );
		--i;
	}
	if( i.key() > time.msecsSinceStartOfDay() )
	{
		if( i == ( *groupIterator ).begin() )
			return  EntryInfo( QDate(), ( *groupIterator ), ( *groupIterator ).constEnd() );
		--i;
	}
	return EntryInfo( QDate::fromJulianDay( groupIterator.key() ), ( *groupIterator ), i );
	/*if( i == ( *groupIterator ).end() )
		return EntryInfo( QDate(), ( *groupIterator ), ( *groupIterator ).constEnd() );
	if( i != ( *groupIterator ).begin() && qAbs( msec - i.key() ) > qAbs( msec - ( i - 1 ).key() ) )
		--i;
	return EntryInfo( QDate(), ( *groupIterator ), i );*/
}

MonitoringLog::EntryInfo MonitoringLog::DayGroup::lastEntry()
{
	return EntryInfo( QDate::fromJulianDay( groupIterator.key() ), groupIterator.value(), groupIterator.value().end() - 1 );
}

MonitoringLog::DigitInputsEntry::DigitInputsEntry( const quint32* data, quint8 blockFlags ) : Entry( Entry::Type::DigitInputsEntry )
{
	this->blockFlags = blockFlags;
	quint32 n = numberOfSetBits( blockFlags );
	d = new quint32[n];
	for( quint32 i = 0; i < n; ++i )
		d[i] = data[i];
}

MonitoringLog::DigitInputsEntry::~DigitInputsEntry()
{
	delete d;
}

MonitoringLog::AnalogInputsEntry::AnalogInputsEntry( const float* data, Desc desc ) : Entry( Entry::Type::AnalogInputsEntry )
{
	this->desc = desc;
	d = new float*[8];
	for( quint32 i = 0; i < 8; ++i )
	{
		quint32 n = ( ( ( *( quint16* )&desc ) >> ( i * 2 ) ) & 0x03 ) * 8;
		if( n == 0 )
			d[i] = nullptr;
		else
		{
			d[i] = new float[n];
			for( quint32 i2 = 0; i2 < n; ++i2 )
				d[i][i2] = data[i2];
			data += n;
		}
	}
}

MonitoringLog::AnalogInputsEntry::~AnalogInputsEntry()
{
	for( quint32 i = 0; i < 8; ++i )
		delete d[i];
	delete d;
}

MonitoringLog::SensorEntry::SensorEntry( QByteArray data ) : Entry( Entry::Type::SensorEntry ), d( data )
{

}
