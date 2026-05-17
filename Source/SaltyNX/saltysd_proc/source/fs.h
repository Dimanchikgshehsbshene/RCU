/**
 * @file fs.h
 * @brief Filesystem (fsp-srv) service IPC wrapper.
 * Normally applications should just use standard stdio not FS-serv directly. However this can be used if obtaining a FsFileSystem, FsFile, or FsStorage, for mounting with fs_dev/romfs_dev, etc.
 * @author plutoo
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include <switch/types.h>
#include <switch/services/sm.h>
#include "ipc.h"

// We use wrapped handles for type safety.

#define FS_MAX_PATH 0x301

/// For use with FsSave.
#define FS_SAVEDATA_CURRENT_TITLEID 0

/// For use with \ref FsSave and \ref FsSaveDataInfo.
#define FS_SAVEDATA_USERID_COMMONSAVE 0

typedef struct {
    Service_old  s;
} FsFileSystem_old;

typedef struct {
    Service_old  s;
} FsFile_old;

typedef struct {
    Service_old  s;
} FsDir_old;

/// Directory entry.
typedef struct
{
    char name[FS_MAX_PATH];      ///< Entry name.
    u8 pad[3];
    s8 type;       ///< See FsEntryType.
    u8 pad2[3];     ///< ?
    u64 fileSize;         ///< File size.
} FsDirectoryEntry_old;

typedef struct
{
    u64 created;  ///< POSIX timestamp.
    u64 modified; ///< POSIX timestamp.
    u64 accessed; ///< POSIX timestamp.
    u8 is_valid;  ///< 0x1 when the timestamps are set.
    u8 padding[7];
} FsTimeStampRaw_old;

typedef enum {
    ENTRYTYPE_DIR  = 0,
    ENTRYTYPE_FILE = 1,
} FsEntryType_old;

typedef enum
{
    FS_OPEN_READ   = BIT(0), ///< Open for reading.
    FS_OPEN_WRITE  = BIT(1), ///< Open for writing.
    FS_OPEN_APPEND = BIT(2), ///< Append file.
} FsFileFlags_old;

typedef enum
{
    FS_CREATE_BIG_FILE = BIT(0), ///< Creates a ConcatenationFile (dir with archive bit) instead of file.
} FsFileCreateFlags_old;

/// For use with fsFsOpenDirectory.
typedef enum
{
    FS_DIROPEN_DIRECTORY    = BIT(0),  ///< Enable reading directory entries.
    FS_DIROPEN_FILE         = BIT(1),  ///< Enable reading file entries.
    FS_DIROPEN_NO_FILE_SIZE = BIT(31), ///< Causes result entries to not contain filesize information (always 0).
} FsDirectoryFlags_old;

typedef enum
{
    FS_READOPTION_NONE = 0, ///< No Option.
} FsReadOption_old;

typedef enum
{
    FS_WRITEOPTION_NONE  = 0,      ///< No option.
    FS_WRITEOPTION_FLUSH = BIT(0), ///< Forces a flush after write.
} FsWriteOption_old;

typedef struct {
    u32 aes_ctr_key_type;           ///< Contains bitflags describing how data is AES encrypted.
    u32 speed_emulation_type;       ///< Contains bitflags describing how data is emulated.
    u32 reserved[0x38/sizeof(u32)];
} FsRangeInfo_old;



/// Do not call this directly, see fs_dev.h.
Result fsMountSdcard_old(FsFileSystem_old* out);

typedef enum
{
    FsFileSystemType_old_Logo               = 2,
    FsFileSystemType_old_ContentControl     = 3,
    FsFileSystemType_old_ContentManual      = 4,
    FsFileSystemType_old_ContentMeta        = 5,
    FsFileSystemType_old_ContentData        = 6,
    FsFileSystemType_old_ApplicationPackage = 7,
} FsFileSystemType_old;

typedef enum
{
    FsFileSystemQueryType_SetArchiveBit = 0,
} FsFileSystemQueryType_old;

typedef enum {
    FsOperationId_old_Clear,           ///< Fill range with zero for supported file/storage.
    FsOperationId_old_ClearSignature,  ///< Clears signature for supported file/storage.
    FsOperationId_old_InvalidateCache, ///< Invalidates cache for supported file/storage.
    FsOperationId_old_QueryRange,      ///< Retrieves information on data for supported file/storage.
} FsOperationId_old;

// IFileSystem
Result fsFsCreateFile_old(FsFileSystem_old* fs, const char* path, size_t size, int flags);
Result fsFsDeleteFile_old(FsFileSystem_old* fs, const char* path);
Result fsFsCreateDirectory_old(FsFileSystem_old* fs, const char* path);
Result fsFsDeleteDirectory_old(FsFileSystem_old* fs, const char* path);
Result fsFsDeleteDirectoryRecursively_old(FsFileSystem_old* fs, const char* path);
Result fsFsRenameFile_old(FsFileSystem_old* fs, const char* cur_path, const char* new_path);
Result fsFsRenameDirectory_old(FsFileSystem_old* fs, const char* cur_path, const char* new_path);
Result fsFsGetEntryType_old(FsFileSystem_old* fs, const char* path, FsEntryType_old* out);
Result fsFsOpenFile_old(FsFileSystem_old* fs, const char* path, int flags, FsFile_old* out);
Result fsFsOpenDirectory_old(FsFileSystem_old* fs, const char* path, int flags, FsDir_old* out);
Result fsFsCommit_old(FsFileSystem_old* fs);
Result fsFsGetFreeSpace_old(FsFileSystem_old* fs, const char* path, u64* out);
Result fsFsGetTotalSpace_old(FsFileSystem_old* fs, const char* path, u64* out);
Result fsFsGetFileTimeStampRaw_old(FsFileSystem_old* fs, const char* path, FsTimeStampRaw_old *out);/// 3.0.0+
Result fsFsQueryEntry_old(FsFileSystem_old* fs, void *out, size_t out_size, const void *in, size_t in_size, const char* path, FsFileSystemQueryType_old query_type);/// 4.0.0+
void fsFsClose_old(FsFileSystem_old* fs);

/// Uses \ref fsFsQueryEntry to set the archive bit on the specified absolute directory path.
/// This will cause HOS to treat the directory as if it were a file containing the directory's concatenated contents.
Result fsFsSetArchiveBit_old(FsFileSystem_old* fs, const char *path);

// IFile
Result fsFileRead_old(FsFile_old* f, u64 off, void* buf, size_t len, u32 option, size_t* out);
Result fsFileWrite_old(FsFile_old* f, u64 off, const void* buf, size_t len, u32 option);
Result fsFileFlush_old(FsFile_old* f);
Result fsFileSetSize_old(FsFile_old* f, u64 sz);
Result fsFileGetSize_old(FsFile_old* f, u64* out);
Result fsFileOperateRange_old(FsFile_old* f, FsOperationId_old op_id, u64 off, size_t len, FsRangeInfo_old* out); /// 4.0.0+
void fsFileClose_old(FsFile_old* f);

// IDirectory
Result fsDirRead_old(FsDir_old* d, u64 inval, size_t* total_entries, size_t max_entries, FsDirectoryEntry_old *buf);
void fsDirClose_old(FsDir_old* d);