#pragma once

#include "FileInfo.h"
#include "Dir.h"
#include <memory>

class FileInfoReader
{
public:
    using Error = FileSystem::Error;

    FileInfoReader();
    FileInfoReader( const FileInfoReader& other ) = delete;
    FileInfoReader( FileInfoReader&& other ) = delete;

    FileInfoReader& operator=( const FileInfoReader& other ) = delete;
    FileInfoReader& operator=( FileInfoReader&& other ) = delete;

    void setDir( const Dir& dir );
    void setDir( const std::string& dirName );
    void setDir( std::string&& dirName );

    void reset();
    bool next();
    FileInfo& info();

    Error lastError();
    const char* lastErrorString();

private:
	struct Impl
	{
		Impl() : opened( false ) {}
		std::string dirName;
		FileInfo fileInfo;
		DIR dir;
		FILINFO fno;
		Error err;
		bool opened;
	};
	std::unique_ptr< Impl > impl;
};