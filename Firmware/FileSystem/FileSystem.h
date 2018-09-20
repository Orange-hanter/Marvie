#pragma once

#include "ff.h"
#include <string>
#include "Core/Flags.h"

class FileSystem
{
public:
	enum Error
	{
		NoError = FR_OK,                                 // Succeeded
		DiskError = FR_DISK_ERR,                         // A hard error occurred in the low level disk I/O layer
		IntError = FR_INT_ERR,                           // Assertion failed
		NotReadyError = FR_NOT_READY,                    // The physical drive cannot work
		NoFileError = FR_NO_FILE,                        // Could not find the file
		NoPathError = FR_NO_PATH,                        // Could not find the path
		InvalidNameError = FR_INVALID_NAME,              // The path name format is invalid
		DeniedError = FR_DENIED,                         // Access denied due to prohibited access or directory full
		ExistError = FR_EXIST,                           // Access denied due to prohibited access
		InvalidObjectError = FR_INVALID_OBJECT,          // The file/directory object is invalid
		WriteProtectedError = FR_WRITE_PROTECTED,        // The physical drive is write protected
		InvalidDriveError = FR_INVALID_DRIVE,            // The logical drive number is invalid 
		NotEnabledError = FR_NOT_ENABLED,                // The volume has no work area
		NoFileSystemError = FR_NO_FILESYSTEM,            // There is no valid FAT volume
		MkfsAbortedError = FR_MKFS_ABORTED,              // The f_mkfs() aborted due to any problem
		TimeoutError = FR_TIMEOUT,                       // Could not get a grant to access the volume within defined period
		LockedError = FR_LOCKED,                         // The operation is rejected according to the file sharing policy
		NotEnoughCoreError = FR_NOT_ENOUGH_CORE,         // LFN working buffer could not be allocated
		TooManyOpenFilesError = FR_TOO_MANY_OPEN_FILES,  // Number of open files > FF_FS_LOCK
		InvalidParameterError = FR_INVALID_PARAMETER,    // Given parameter is invalid
		MemoryError
	};
	enum Type
	{
		Fat12Type = FS_FAT12,
		Fat16Type = FS_FAT16,
		Fat32Type = FS_FAT32,
		ExFatType = FS_EXFAT,
	};
	enum Format
	{
		FatFormat = FM_FAT,
		Fat32Format = FM_FAT32,
		ExFatFormat = FM_EXFAT,
		AnyFormat = FM_ANY,
		SfdFormat = FM_SFD,
	};
	enum ClusterSize
	{
		Cluster512 = 512,
		Cluster1024 = 1024,
		Cluster2048 = 2048,
		Cluster4096 = 4096,
		Cluster8192 = 8192,
		Cluster16K = 16384,
		Cluster32K = 32768,
		Cluster64K = 65536,
	};
	enum OpenModeFlag : int
	{
		OpenExisting = FA_OPEN_EXISTING,  // Opens the file. The function fails if the file is not existing.
		Read = FA_READ,                   // Specifies read access to the object. Data can be read from the file.
		Write = FA_WRITE,                 // Specifies write access to the object. Data can be written to the file.
		ReadWrite = FA_READ | FA_WRITE,   // Read-write access
		CreateNew = FA_CREATE_NEW,        // Creates a new file. The function fails with FR_EXIST if the file is existing.
		CreateAlways = FA_CREATE_ALWAYS,  // Creates a new file. If the file is existing, it will be truncated and overwritten.
		OpenAlways = FA_OPEN_ALWAYS,      // Opens the file if it is existing. If not, a new file will be created.
		OpenAppend = FA_OPEN_APPEND       // Same as OpenAlways except the read/write pointer is set end of the file.
	};
	enum FileAttribute
	{
		ReadOnlyAttr = AM_RDO,
		HiddenAttr = AM_HID,
		SystemAttr = AM_SYS,
		DirectoryAttr = AM_DIR,
		ArchiveAttr = AM_ARC
	};
	typedef Flags< OpenModeFlag > OpenMode;
	typedef Flags< FileAttribute > FileAttributes;
#if FF_LFN_UNICODE
	using string = std::wstring;
#else
	using string = std::string;
#endif

	FileSystem();
	~FileSystem();

	bool mount( const TCHAR* path );
	bool unmount();

	static Error createFileSystem( const TCHAR* path, Format format, ClusterSize clusterSize, uint8_t* work = nullptr, uint32_t workSize = 0 );

	Error lastError();

private:
	FATFS fs;
	Error err;
};