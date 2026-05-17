/**
 * @file fs_dev.h
 * @brief FS driver, using devoptab.
 * @author yellows8
 * @author mtheall
 * @copyright libnx Authors
 */
#pragma once

#include <sys/types.h>
#include "fs.h"

#define FSDEV_DIRITER_MAGIC 0x66736476 ///< "fsdv"

/// Open directory struct
typedef struct
{
  u32               magic;         ///< "fsdv"
  FsDir_old             fd;            ///< File descriptor
  ssize_t           index;         ///< Current entry index
  size_t            size;          ///< Current batch size
  FsDirectoryEntry_old entry_data[32]; ///< Temporary storage for reading entries
} fsdev_dir_t_old;

/// Initializes and mounts the sdmc device if accessible. Also initializes current working directory to point to the folder containing the path to the executable (argv[0]), if it is provided by the environment.
Result fsdevMountSdmc_old(void);

/// Mounts the input fs with the specified device name. fsdev will handle closing the fs when required, including when fsdevMountDevice() fails.
/// Returns -1 when any errors occur.
int fsdevMountDevice_old(const char *name, FsFileSystem_old fs);

/// Unmounts the specified device.
int fsdevUnmountDevice_old(const char *name);

/// Uses fsFsCommit() with the specified device. This must be used after any savedata-write operations(not just file-write). This should be used after each file-close where file-writing was done.
/// This is not used automatically at device unmount.
Result fsdevCommitDevice_old(const char *name);

/// Returns the FsFileSystem for the specified device. Returns NULL when the specified device isn't found.
FsFileSystem_old* fsdevGetDeviceFileSystem_old(const char *name);

/// Returns the FsFileSystem for the default device (SD card), if mounted. Used internally by romfs_dev.
FsFileSystem_old* fsdevGetDefaultFileSystem_old(void);

/// Writes the FS-path to outpath (which has buffer size FS_MAX_PATH), for the input path (as used in stdio). The FsFileSystem is also written to device when not NULL.
int fsdevTranslatePath_old(const char *path, FsFileSystem_old** device, char *outpath);

/// This calls fsFsSetArchiveBit on the filesystem specified by the input path (as used in stdio).
Result fsdevSetArchiveBit_old(const char *path);

/// This calls fsFsCreateFile on the filesystem specified by the input path (as used in stdio).
Result fsdevCreateFile_old(const char* path, size_t size, int flags);

/// Recursively deletes the directory specified by the input path (as used in stdio).
Result fsdevDeleteDirectoryRecursively_old(const char *path);

/// Unmounts all devices and cleans up any resources used by the FS driver.
Result fsdevUnmountAll_old(void);

/// Retrieves the last native result code generated during a failed fsdev operation.
Result fsdevGetLastResult_old(void);
