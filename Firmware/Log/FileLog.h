#pragma once

#include "cpp_wrappers/ch.hpp"
#include "FileSystem/File.h"

class FileLog
{
public:
	FileLog();
	~FileLog();

	bool open( const TCHAR* logFilePath );
	void close();

	void addRecorg( const char* data, uint32_t lenght );
	void clean();

private:
	File file;
	chibios_rt::Mutex mutex;
};