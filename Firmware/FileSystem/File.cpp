#include "File.h"
#include <string.h>

File::File()
{
	memset( &_fil, 0, sizeof( _fil ) );
	_err = Error::NoError;
}

File::File( const string& name )
{
	_name = name;
	memset( &_fil, 0, sizeof( _fil ) );
	_err = Error::NoError;
}

File::File( string&& name )
{
	_name.swap( name );
	memset( &_fil, 0, sizeof( _fil ) );
	_err = Error::NoError;
}

File::~File()
{
	close();
}

void File::setFileName( const string& name )
{
	_name = name;
}

void File::setFileName( string&& name )
{
	_name.swap( name );
}

File::string File::fileName()
{
	return _name;
}

bool File::exists()
{
	return exists( _name.c_str() );
}

bool File::exists( const TCHAR* name )
{
	volatile bool res = false;
	FILINFO* info = new FILINFO;
	if( f_stat( name, info ) == FR_OK )
	{
		if( ( info->fattrib & AM_DIR ) == 0 )
			res = true;
	}
	delete info;

	return res;
}

bool File::open( const TCHAR* name, FileSystem::OpenMode mode )
{
	if( isOpen() )
		return false;

	_name = name;
	_err = ( Error )f_open( &_fil, name, mode );
	return _err == Error::NoError;
}

bool File::open( FileSystem::OpenMode mode )
{
	if( isOpen() )
		return false;

	_err = ( Error )f_open( &_fil, _name.c_str(), mode );
	return _err == Error::NoError;
}

bool File::isOpen()
{
	return _fil.obj.fs != 0;
}

void File::close()
{
	_err = ( Error )f_close( &_fil );
}

uint32_t File::read( uint8_t* data, uint32_t size )
{
	UINT br;
	_err = ( Error )f_read( &_fil, data, size, &br );
	return br;
}

uint32_t File::write( const uint8_t* data, uint32_t size )
{
	UINT bw;
	_err = ( Error )f_write( &_fil, data, size, &bw );
	return bw;
}

bool File::seek( uint64_t pos )
{
	_err = ( Error )f_lseek( &_fil, pos );
	return _err == Error::NoError;
}

uint64_t File::pos()
{
	return _fil.fptr;
}

bool File::flush()
{
	_err = ( Error )f_sync( &_fil );
	return _err == Error::NoError;
}

bool File::truncate()
{
	_err = ( Error )f_truncate( &_fil );
	return _err == Error::NoError;
}

uint64_t File::size()
{
	return f_size( &_fil );
}

File::Error File::lastError()
{
	return _err;
}