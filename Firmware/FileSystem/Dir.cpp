#include "Dir.h"
#include <list>
#include <string.h>
#include <algorithm>

Dir::Dir()
{
	_err = Error::NoError;
}

Dir::Dir( const string& path )
{
	_path = path;
	_cleanPath( _path );
	_err = Error::NoError;	
}

Dir::Dir( const Dir& dir )
{
	_path = dir._path;
	_err = Error::NoError;
}

Dir::Dir( string&& path )
{
	_path.swap( path );
	_cleanPath( _path );
	_err = Error::NoError;
}

Dir::Dir( Dir&& dir )
{
	swap( dir );
}

Dir::~Dir()
{

}

void Dir::setPath( string path )
{
	_path = path;
	_cleanPath( _path );
}

Dir::string Dir::path() const
{
	return _path;
}

bool Dir::exists() const
{
	DIR dir;
	if( f_opendir( &dir, _path.c_str() ) == FR_OK )
	{
		f_closedir( &dir );
		return true;
	}

	return false;
}

bool Dir::exists( const TCHAR* path ) const
{
	auto fullPath = _path + '/' + path;
	_cleanPath( fullPath );

	DIR dir;
	if( f_opendir( &dir, fullPath.c_str() ) == FR_OK )
	{
		f_closedir( &dir );
		return true;
	}

	return false;
}

std::vector< std::string > Dir::entryList( Filters filters, SortFlag sortFlag /*= NoSort*/ ) const
{
	struct Resource
	{
		DIR dir;
		FILINFO fno;
	}*_r = new Resource;
	if( _r == nullptr )
		return std::vector< std::string >();

	std::vector< std::string > vect;
	if( f_opendir( &_r->dir, _path.c_str() ) == FR_OK )
	{
		while( f_readdir( &_r->dir, &_r->fno ) == FR_OK && _r->fno.fname[0] )
		{
			if( FF_FS_RPATH && _r->fno.fname[0] == '.' )
				continue;

			if( _r->fno.fattrib & AM_DIR )
			{
				if( filters.testFlag( Filter::Dirs ) )
					vect.push_back( _r->fno.fname );
			}
			else
			{
				if( filters.testFlag( Filter::Files ) )
					vect.push_back( _r->fno.fname );
			}
		}

		f_closedir( &_r->dir );
	}
	delete _r;

	//std::vector< std::string > vect{ std::make_move_iterator( list.begin() ), std::make_move_iterator( list.end() ) };
	if( sortFlag == Name )
		std::sort( vect.begin(), vect.end() );

	return vect;
}

bool Dir::mkdir( const TCHAR* path )
{
	if( path == nullptr )
		path = "";
	auto fullPath = _path + '/' + path;
	_cleanPath( fullPath );
	_err = ( Error )f_mkdir( fullPath.c_str() );

	return _err == Error::NoError;
}

bool Dir::rmdir( const TCHAR* path )
{
	if( path == nullptr )
		path = "";
	auto fullPath = _path + '/' + path;
	_cleanPath( fullPath );
	_err = ( Error )f_rmdir( fullPath.c_str() );

	return _err == Error::NoError;
}

bool Dir::mkpath( const TCHAR* path )
{
	if( path == nullptr )
		path = "";
	auto fullPath = _path + '/' + path;
	_cleanPath( fullPath );
	if( ( _err = ( Error )f_mkdir( fullPath.c_str() ) ) == Error::NoError || _err == Error::ExistError )
		return true;
	for( std::size_t i = 1; i < fullPath.size(); ++i )
	{
		if( fullPath[i] == '/' )
		{
			fullPath[i] = 0;
			if( ( _err = ( Error )f_mkdir( fullPath.c_str() ) ) != Error::NoError && _err != Error::ExistError )
				return false;
			fullPath[i] = '/';
		}
	}

	if( ( _err = ( Error )f_mkdir( fullPath.c_str() ) ) != Error::NoError && _err != Error::ExistError )
		return false;

	return true;
}

bool Dir::rmpath( const TCHAR* path )
{
	if( path == nullptr )
		path = "";
	auto fullPath = _path + '/' + path;
	_cleanPath( fullPath );

	if( ( _err = ( Error )f_rmdir( fullPath.c_str() ) ) != Error::NoError )
		return false;

	for( std::size_t i = fullPath.size() - 1; i > 0; --i )
	{
		if( fullPath[i] == '/' )
		{
			fullPath[i] = 0;
			if( ( Error )f_rmdir( fullPath.c_str() ) != Error::NoError )
				return true;
		}
	}

	return true;
}

bool Dir::rename( const TCHAR* oldName, const TCHAR* newName )
{
	auto oldPath = _path + '/' + oldName;
	_cleanPath( oldPath );
	auto newPath = _path + '/' + newName;
	_cleanPath( newPath );
	_err = ( Error )f_rename( oldPath.c_str(), newPath.c_str() );

	return _err == Error::NoError;
}

bool Dir::remove( const TCHAR* fileName )
{
	auto fullPath = _path + '/' + fileName;
	_cleanPath( fullPath );
	_err = ( Error )f_unlink( fullPath.c_str() );

	return _err == Error::NoError;
}

bool Dir::removeRecursively()
{
	FILINFO* fno = new FILINFO;
	if( fno == nullptr )
	{
		_err = Error::MemoryError;
		return false;
	}
	if( ( _err = _removeRecursively( _path, *fno ) ) == Error::NoError )
		_err = ( Error )f_unlink( _path.c_str() );
	delete fno;

	return _err == Error::NoError || _err == Error::NoPathError;
}

uint64_t Dir::contentSize()
{
	FILINFO* fno = new FILINFO;
	if( fno == nullptr )
	{
		_err = Error::MemoryError;
		return false;
	}
	uint64_t totalSize = 0;
	_err = _contentSize( _path, *fno, totalSize );
	delete fno;

	return totalSize;
}

void Dir::swap( Dir& other )
{
	_path.swap( other._path );
}

Dir& Dir::operator=( Dir&& other )
{
	swap( other );
	return *this;
}

Dir& Dir::operator=( Dir& other )
{
	_path = other._path;
	_err = Error::NoError;
	return *this;
}

Dir::Error Dir::_removeRecursively( const string& path, FILINFO& fno )
{
	Error res;
	DIR* dir = new DIR;
	if( dir == nullptr )
		return Error::MemoryError;

	res = ( Error )f_opendir( dir, path.c_str() );
	if( res == Error::NoError )
	{
		while( ( ( res = ( Error )f_readdir( dir, &fno ) ) == Error::NoError ) && fno.fname[0] )
		{
			if( FF_FS_RPATH && fno.fname[0] == '.' )
				continue;

			string nextPath = path + '/' + fno.fname;
			if( fno.fattrib & AM_DIR )
			{
				res = _removeRecursively( nextPath, fno );
				if( res != Error::NoError )
					break;
			}

			f_unlink( nextPath.c_str() );
		}
		res = ( Error )f_closedir( dir );
	}

	delete dir;
	return res;
}

Dir::Error Dir::_contentSize( const string& path, FILINFO& fno, uint64_t& totalSize )
{
	Error res;
	DIR* dir = new DIR;
	if( dir == nullptr )
		return Error::MemoryError;

	res = ( Error )f_opendir( dir, path.c_str() );
	if( res == Error::NoError )
	{
		while( ( ( res = ( Error )f_readdir( dir, &fno ) ) == Error::NoError ) && fno.fname[0] )
		{
			if( FF_FS_RPATH && fno.fname[0] == '.' )
				continue;

			string nextPath = path + '/' + fno.fname;
			if( fno.fattrib & AM_DIR )
			{
				res = _contentSize( nextPath, fno, totalSize );
				if( res != Error::NoError )
					break;
			}
			else
				totalSize += fno.fsize;
		}
		res = ( Error )f_closedir( dir );
	}

	delete dir;
	return res;
}

void Dir::_cleanPath( string& path )
{
	// TODO: add impl
	for( std::size_t i = 1; i < path.size(); ++i )
	{
		if( path[i - 1] == '/' && path[i] == '/' )
		{
			path.erase( i, 1 );
			--i;
		}
	}
	if( !path.empty() && path.back() == '/' )
		path.resize( path.size() - 1 );
}

Dir::string Dir::cleanPath( const string& path )
{
	string tmp = path;
	_cleanPath( tmp );

	return tmp;
}
