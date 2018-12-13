#pragma once

#include "FileSystem.h"
#include "Core/DateTime.h"

class FileInfo
{
	friend class Dir;
	friend class File;

	FileInfo();

public:
	using Error = FileSystem::Error;
	using string = FileSystem::string;

	FileInfo( const TCHAR* path );

	inline uint64_t fileSize() const
	{
		return _size;
	}
	inline DateTime lastModified() const
	{
		return _dateTime;
	}
	inline FileSystem::FileAttributes attributes() const
	{
		return _attr;
	}
	inline const string& fileName() const
	{
		return _name;
	}

private:
	uint64_t _size;
	DateTime _dateTime;
	FileSystem::FileAttributes _attr;
	string _name;
};