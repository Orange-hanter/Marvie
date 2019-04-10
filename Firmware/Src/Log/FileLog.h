#pragma once

#include "cpp_wrappers/ch.hpp"
#include "FileSystem/File.h"
#include <string.h>

class FileLog
{
public:
	FileLog();
	~FileLog();

	bool open( const TCHAR* logFilePath );
	void close();

	void addRecorg( const char* data, uint32_t lenght );
	inline void addRecorg( const char* data ) { addRecorg( data, strlen( data ) ); }
	void clean();

private:
	File file;
	chibios_rt::_Mutex mutex;
};