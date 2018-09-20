#include "FileSystem.h"

FileSystem::FileSystem()
{
	f_mount( &fs, "", 0 );
	err = Error::NoError;
}

FileSystem::~FileSystem()
{
	f_mount( &fs, "", 0 );
}

bool FileSystem::mount( const TCHAR* path )
{
	err = ( Error )f_mount( &fs, path, 1 );
	return err == Error::NoError;
}

bool FileSystem::unmount()
{
	err = ( Error )f_mount( &fs, "", 0 );
	return err == Error::NoError;
}

FileSystem::Error FileSystem::createFileSystem( const TCHAR* path, Format format, ClusterSize clusterSize, uint8_t* work /*= nullptr*/, uint32_t workSize /*= 0 */ )
{
	if( work == nullptr )
	{
		work = new uint8_t[1024];
		Error err = ( Error )f_mkfs( path, format, clusterSize, work, 1024 );
		delete work;

		return err;
	}

	return ( Error )f_mkfs( path, format, clusterSize, work, workSize );
}

FileSystem::Error FileSystem::lastError()
{
	return err;
}
