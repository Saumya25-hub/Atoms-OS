#ifndef BOFS_VFS_H
#define BOFS_VFS_H

#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/vfs/bofs/include/bofs_format.h"
#include "kernel/vfs/bofs/include/bofs_alloc.h"
#include "kernel/vfs/bofs/include/bofs_file.h"
#include "kernel/vfs/bofs/include/bofs_dir.h"
#include "kernel/vfs/bofs/include/bofs_security.h"
#include "kernel/vfs/bofs/include/bofs_wal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * BOFS In-Memory Mount Context Structure
 * -------------------------------------------------------------------------- */
typedef struct {
    BlockDevice*        bdev;
    bofs_superblock_t   sb;
    bofs_allocator_t    alloc;
    bofs_file_system_t  fs;
    bofs_wal_t          wal;
    bool                wal_active;
    bool                read_only;
} bofs_mount_context_t;

/* --------------------------------------------------------------------------
 * BOFS In-Memory File Handle (attached to VFS_Node private_data)
 * -------------------------------------------------------------------------- */
typedef struct {
    bofs_mount_context_t* mctx;
    uint64_t              inode_num;
    uint32_t              generation;
    uint32_t              mode;
    uint64_t              file_size;
    bofs_file_t           file;
    bool                  is_dir;
} bofs_vfs_handle_t;

/* Exported VFS Filesystem Driver */
extern FilesystemDriver bofs_fs_driver;

/* Public VFS Driver Registration and Initialization */
void bofs_vfs_init(void);

/* Helper to set process credentials for security tests */
void bofs_vfs_set_caller_cred(const bofs_cred_t* cred);
bofs_cred_t bofs_vfs_get_caller_cred(void);
void bofs_vfs_reset_caller_cred(void);

#ifdef __cplusplus
}
#endif

#endif /* BOFS_VFS_H */
