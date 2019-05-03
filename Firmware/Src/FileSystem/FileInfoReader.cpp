#include "FileInfoReader.h"

FileInfoReader::FileInfoReader() : impl( new Impl )
{

}

void FileInfoReader::setDir( const Dir& dir )
{
	impl->dirName = dir.path();
}

void FileInfoReader::setDir( const std::string& dirName )
{
	impl->dirName = dirName;
}

void FileInfoReader::setDir( std::string&& dirName )
{
	impl->dirName = std::move( dirName );
}

void FileInfoReader::reset()
{
	if( impl && impl->opened )
	{
		f_closedir( &impl->dir );
		impl->opened = false;
	}
}

bool FileInfoReader::next()
{
	if( impl == nullptr )
		return false;

	if( !impl->opened )
	{
		if( ( impl->err = ( Error )f_opendir( &impl->dir, impl->dirName.c_str() ) ) != Error::NoError )
			return false;
		impl->opened = true;
	}

Next:
	if( ( impl->err = ( Error )f_readdir( &impl->dir, &impl->fno ) ) == Error::NoError && impl->fno.fname[0] )
	{
		if( FF_FS_RPATH && impl->fno.fname[0] == '.' )
			goto Next;

		impl->fileInfo.set( &impl->fno );
		return true;
	}

	return false;
}

FileInfo& FileInfoReader::info()
{
	return impl->fileInfo;
}

FileInfoReader::Error FileInfoReader::lastError()
{
	if( impl == nullptr )
		return Error::MemoryError;
	return impl->err;
}

const char* FileInfoReader::lastErrorString()
{
	if( impl == nullptr )
		return FileSystem::errorString( Error::MemoryError );
	return FileSystem::errorString( impl->err );
}
