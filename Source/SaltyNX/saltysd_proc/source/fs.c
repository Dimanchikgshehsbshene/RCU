// Copyright 2017 plutoo
#include <string.h>
#include <switch.h>
#include "fs.h"
#include "ipc.h"

Service_old g_fsSrv;

Result fsMountSdcard_old(FsFileSystem_old* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_fsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 18;

    Result rc = serviceIpcDispatch(&g_fsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_fsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice_old(&out->s, &g_fsSrv, &r, 0);
        }
    }

    return rc;
}

// IFileSystem impl
Result fsFsCreateFile_old(FsFileSystem_old* fs, const char* path, size_t size, int flags) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, path, FS_MAX_PATH, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 flags;
        u64 size;
    } *raw;

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->flags = flags;
    raw->size = size;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsFsDeleteFile_old(FsFileSystem_old* fs, const char* path) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, path, FS_MAX_PATH, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsFsCreateDirectory_old(FsFileSystem_old* fs, const char* path) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, path, FS_MAX_PATH, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsFsDeleteDirectory_old(FsFileSystem_old* fs, const char* path) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, path, FS_MAX_PATH, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsFsDeleteDirectoryRecursively_old(FsFileSystem_old* fs, const char* path) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, path, FS_MAX_PATH, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsFsRenameFile_old(FsFileSystem_old* fs, const char* cur_path, const char* new_path) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, cur_path, FS_MAX_PATH, 0);
    ipcAddSendStatic(&c, new_path, FS_MAX_PATH, 1);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsFsRenameDirectory_old(FsFileSystem_old* fs, const char* cur_path, const char* new_path) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, cur_path, FS_MAX_PATH, 0);
    ipcAddSendStatic(&c, new_path, FS_MAX_PATH, 1);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 6;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsFsGetEntryType_old(FsFileSystem_old* fs, const char* path, FsEntryType_old* out) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, path, FS_MAX_PATH, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 7;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 type;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out = resp->type;
        }
    }

    return rc;
}

Result fsFsOpenFile_old(FsFileSystem_old* fs, const char* path, int flags, FsFile_old* out) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, path, FS_MAX_PATH, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 flags;
    } *raw;

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 8;
    raw->flags = flags;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice_old(&out->s, &fs->s, &r, 0);
        }
    }

    return rc;
}

Result fsFsOpenDirectory_old(FsFileSystem_old* fs, const char* path, int flags, FsDir_old* out) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, path, FS_MAX_PATH, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 flags;
    } *raw;

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 9;
    raw->flags = flags;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice_old(&out->s, &fs->s, &r, 0);
        }
    }

    return rc;
}

Result fsFsCommit_old(FsFileSystem_old* fs) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 10;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsFsGetFreeSpace_old(FsFileSystem_old* fs, const char* path, u64* out) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, path, FS_MAX_PATH, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 11;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 space;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out = resp->space;
        }
    }

    return rc;
}

Result fsFsGetTotalSpace_old(FsFileSystem_old* fs, const char* path, u64* out) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, path, FS_MAX_PATH, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 12;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 space;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out = resp->space;
        }
    }

    return rc;
}

Result fsFsGetFileTimeStampRaw_old(FsFileSystem_old* fs, const char* path, FsTimeStampRaw_old *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    char send_path[FS_MAX_PATH] = {0};
    strncpy(send_path, path, sizeof(send_path)-1);

    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, send_path, sizeof(send_path), 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 14;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            FsTimeStampRaw_old out;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && out) *out = resp->out;
    }

    return rc;
}

Result fsFsQueryEntry_old(FsFileSystem_old* fs, void *out, size_t out_size, const void *in, size_t in_size, const char* path, FsFileSystemQueryType_old query_type) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    char send_path[FS_MAX_PATH] = {0};
    strncpy(send_path, path, sizeof(send_path)-1);

    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, send_path, sizeof(send_path), 0);
    ipcAddSendBuffer(&c, in, in_size, BufferType_Type1);
    ipcAddRecvBuffer(&c, out, out_size, BufferType_Type1);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 query_type;
    } *raw;

    raw = serviceIpcPrepareHeader(&fs->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 15;
    raw->query_type = query_type;

    Result rc = serviceIpcDispatch(&fs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&fs->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsFsSetArchiveBit_old(FsFileSystem_old* fs, const char *path) {
    return fsFsQueryEntry_old(fs, NULL, 0, NULL, 0, path, FsFileSystemQueryType_SetArchiveBit);
}

void fsFsClose_old(FsFileSystem_old* fs) {
    serviceClose_old(&fs->s);
}

// IFile implementation
Result fsFileRead_old(FsFile_old* f, u64 off, void* buf, size_t len, u32 option, size_t* out) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, buf, len, 1);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 option;
        u64 offset;
        u64 read_size;
    } *raw;

    raw = serviceIpcPrepareHeader(&f->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->option = option;
    raw->offset = off;
    raw->read_size = len;

    Result rc = serviceIpcDispatch(&f->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 bytes_read;
        } *resp;

        serviceIpcParse(&f->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out = resp->bytes_read;
        }
    }

    return rc;
}

Result fsFileWrite_old(FsFile_old* f, u64 off, const void* buf, size_t len, u32 option) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, buf, len, 1);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 option;
        u64 offset;
        u64 write_size;
    } *raw;

    raw = serviceIpcPrepareHeader(&f->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->option = option;
    raw->offset = off;
    raw->write_size = len;

    Result rc = serviceIpcDispatch(&f->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&f->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsFileFlush_old(FsFile_old* f) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&f->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;

    Result rc = serviceIpcDispatch(&f->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&f->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsFileSetSize_old(FsFile_old* f, u64 sz) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 size;
    } *raw;

    raw = serviceIpcPrepareHeader(&f->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;
    raw->size = sz;

    Result rc = serviceIpcDispatch(&f->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&f->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result fsFileGetSize_old(FsFile_old* f, u64* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&f->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;

    Result rc = serviceIpcDispatch(&f->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 size;
        } *resp;

        serviceIpcParse(&f->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc) && out) *out = resp->size;
    }

    return rc;
}

void fsFileClose_old(FsFile_old* f) {
    serviceClose_old(&f->s);
}

// IDirectory implementation
void fsDirClose_old(FsDir_old* d) {
    serviceClose_old(&d->s);
}

Result fsDirRead_old(FsDir_old* d, u64 inval, size_t* total_entries, size_t max_entries, FsDirectoryEntry_old *buf) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, buf, sizeof(FsDirectoryEntry_old)*max_entries, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 inval;
    } *raw;

    raw = serviceIpcPrepareHeader(&d->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->inval = inval;

    Result rc = serviceIpcDispatch(&d->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 total_entries;
        } *resp;

        serviceIpcParse(&d->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (total_entries) *total_entries = resp->total_entries;
        }
    }

    return rc;
}