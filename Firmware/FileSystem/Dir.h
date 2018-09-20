#pragma once

#include "FileSystem.h"
#include <vector>
#include <string>

class Dir
{
public:
	using string = FileSystem::string;
	using Error = FileSystem::Error;
	enum Filter
	{
		Dirs = 1,
		Files = 2
	};
	enum SortFlag
	{
		NoSort = 1,
		Name = 2
	};
	typedef Flags< Filter > Filters;

	Dir();
	Dir( const string& path );
	Dir( const Dir& dir );
	Dir( string&& path );
	Dir( Dir&& dir );
	~Dir();

	void setPath( string path );
	string path() const;

	bool exists() const;
	bool exists( const TCHAR* path ) const;

	std::vector< std::string > entryList( Filters filters, SortFlag sortFlag = NoSort ) const;

	bool mkdir( const TCHAR* path = nullptr );
	bool rmdir( const TCHAR* path = nullptr );
	bool mkpath( const TCHAR* path = nullptr );
	bool rmpath( const TCHAR* path = nullptr );

	bool rename( const TCHAR* oldName, const TCHAR* newName );
	bool remove( const TCHAR* fileName );
	bool removeRecursively();
	uint64_t contentSize();

	void swap( Dir& other );
	Dir& operator=( Dir& other );
	Dir& operator=( Dir&& other );

	static string cleanPath( const string& path );

private:
	static Error _removeRecursively( const string& path, FILINFO& fno );
	static Error _contentSize( const string& path, FILINFO& fno, uint64_t& totalSize );
	static void _cleanPath( string& );

private:
	string _path;
	Error _err;
};