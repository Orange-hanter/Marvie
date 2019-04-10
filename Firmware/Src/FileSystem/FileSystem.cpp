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

const char* FileSystem::lastErrorString()
{
	return errorString( err );
}

const char* FileSystem::errorString( Error err )
{
	switch( err )
	{
	case FileSystem::NoError:
		return "NoError";
	case FileSystem::DiskError:
		return "DiskError";
	case FileSystem::IntError:
		return "IntError";
	case FileSystem::NotReadyError:
		return "NotReadyError";
	case FileSystem::NoFileError:
		return "NoFileError";
	case FileSystem::NoPathError:
		return "NoPathError";
	case FileSystem::InvalidNameError:
		return "InvalidNameError";
	case FileSystem::DeniedError:
		return "DeniedError";
	case FileSystem::ExistError:
		return "ExistError";
	case FileSystem::InvalidObjectError:
		return "InvalidObjectError";
	case FileSystem::WriteProtectedError:
		return "WriteProtectedError";
	case FileSystem::InvalidDriveError:
		return "InvalidDriveError";
	case FileSystem::NotEnabledError:
		return "NotEnabledError";
	case FileSystem::NoFileSystemError:
		return "NoFileSystemError";
	case FileSystem::MkfsAbortedError:
		return "MkfsAbortedError";
	case FileSystem::TimeoutError:
		return "TimeoutError";
	case FileSystem::LockedError:
		return "LockedError";
	case FileSystem::NotEnoughCoreError:
		return "NotEnoughCoreError";
	case FileSystem::TooManyOpenFilesError:
		return "TooManyOpenFilesError";
	case FileSystem::InvalidParameterError:
		return "InvalidParameterError";
	case FileSystem::MemoryError:
		return "MemoryError";
	default:
		return "UnknownError";
	}
}