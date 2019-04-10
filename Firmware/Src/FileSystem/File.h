#pragma once

#include "FileSystem.h"
#include "Core/Flags.h"

class File
{
public:
	using Error = FileSystem::Error;
	using string = FileSystem::string;

	File();
	File( const string& name );
	File( string&& name );
	~File();

	void setFileName( const string& name );
	void setFileName( string&& name );
	string fileName();

	bool exists();
	static bool exists( const TCHAR* name );
	bool open( FileSystem::OpenMode mode );
	bool open( const TCHAR* name, FileSystem::OpenMode mode );
	bool isOpen();
	void close();

	uint32_t read( uint8_t* data, uint32_t size );
	uint32_t write( const uint8_t* data, uint32_t size );
	bool seek( uint64_t pos );
	uint64_t pos();
	bool flush();
	bool truncate();
	uint64_t size();

	Error lastError();
	const char* lastErrorString();

private:
	string _name;
	FIL _fil;
	Error _err;
};
