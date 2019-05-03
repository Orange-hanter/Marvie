#include "FileLog.h"
#include "Core/DateTimeService.h"

FileLog::FileLog()
{

}

FileLog::~FileLog()
{

}

bool FileLog::open( const TCHAR* logFilePath )
{
	mutex.lock();
	if( file.isOpen() )
		file.close();

	volatile bool res = file.open( logFilePath, FileSystem::OpenAppend | FileSystem::Write );
	mutex.unlock();

	return res;
}

void FileLog::close()
{
	mutex.lock();
	file.close();
	mutex.unlock();
}

void FileLog::addRecorg( const char* data, uint32_t lenght )
{
	mutex.lock();
	if( !file.isOpen() )
	{
		mutex.unlock();
		return;
	}

	char str[25];
	char* s = DateTimeService::currentDateTime().printDateTime( str );
	*s++ = ' ';
	*s++ = ':';
	*s++ = ' ';

	file.write( ( uint8_t* )str, s - str );
	file.write( ( uint8_t* )data, lenght );
	file.write( ( uint8_t* )"\n", 1 );
	file.flush();
	mutex.unlock();
}

void FileLog::clean()
{
	mutex.lock();
	if( file.isOpen() )
	{
		file.seek( 0 );
		file.truncate();
		file.flush();
	}
	mutex.unlock();
}
