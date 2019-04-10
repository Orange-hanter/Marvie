#include "FileInfo.h"

FileInfo::FileInfo()
{
	_size = 0;
	_attr = 0;
}

FileInfo::FileInfo( FILINFO* info )
{
	set( info );
}

void FileInfo::set( FILINFO* info )
{
	_size = info->fsize;
	_dateTime.time().setTime( ( ( info->ftime >> 11 ) & 0x1F ),
							  ( ( info->ftime >> 5 ) & 0x3F ),
							  ( info->ftime & 0x1F ) * 2,
							  0 );
	_dateTime.date().setDate( ( ( info->fdate >> 9 ) & 0x7F ) + 1980,
							  ( ( info->fdate >> 5 ) & 0x0F ),
							  ( info->fdate & 0x1F ) );
	_attr = info->fattrib;
	_name = info->fname;
}

FileInfo::FileInfo( const TCHAR* path )
{
	FILINFO* info = new FILINFO;
	if( info == nullptr )
		return;
	if( f_stat( path, info ) == FR_OK )	
		set( info );	
	delete info;
}
