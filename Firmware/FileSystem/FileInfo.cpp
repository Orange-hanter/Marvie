#include "FileInfo.h"

FileInfo::FileInfo()
{
	_size = 0;
	_attr = 0;
}

FileInfo::FileInfo( const TCHAR* path )
{
	FILINFO* info = new FILINFO;
	if( f_stat( path, info ) == FR_OK )
	{
		_size = info->fsize;
		_dateTime.time().setTime( 0,
								  ( info->ftime & 0x1F ) * 2,
								  ( ( info->ftime >> 5 ) & 0x3F ),
								  ( ( info->ftime >> 11 ) & 0x1F ) );
		_dateTime.date().setDate( ( info->fdate & 0x1F ),
								  ( ( info->fdate >> 5 ) & 0x0F ),
								  ( ( info->fdate >> 9 ) & 0x7F ) + 1980 );
		_attr = info->fattrib;
		_name = info->fname;
	}
	delete info;
}
