#include "kernel/vfs/bofs/include/bofs_vfs.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

extern void com1_dbg(const char *msg);

#define BOFS_VFS_SECTORS_PER_BLOCK (BOFS_BLOCK_SIZE / 512U)

/* --------------------------------------------------------------------------
 * Caller Security Credentials Authority
 * -------------------------------------------------------------------------- */
static bool s_has_custom_cred = false;
static bofs_cred_t s_custom_cred;

void bofs_vfs_set_caller_cred(const bofs_cred_t* cred) {
    if (cred) {
        s_custom_cred = *cred;
        s_has_custom_cred = true;
    }
}

void bofs_vfs_reset_caller_cred(void) {
    s_has_custom_cred = false;
}

bofs_cred_t bofs_vfs_get_caller_cred(void) {
    if (s_has_custom_cred) {
        return s_custom_cred;
    }
    // Default system root credentials (uid=0, gid=0, full caps)
    return bofs_cred_system();
}

/* --------------------------------------------------------------------------
 * Path Helper: Split Path into Parent Directory and Leaf Name
 * -------------------------------------------------------------------------- */
static void bofs_vfs_split_path(const char* path, char* out_parent, size_t max_parent,
                                char* out_leaf, size_t max_leaf) {
    if (!path || path[0] == '\0') {
        strncpy(out_parent, "/", max_parent - 1);
        out_parent[max_parent - 1] = '\0';
        out_leaf[0] = '\0';
        return;
    }

    size_t len = strlen(path);
    int last_slash = -1;
    for (int i = (int)len - 1; i >= 0; i--) {
        if (path[i] == '/') {
            last_slash = i;
            break;
        }
    }

    if (last_slash < 0) {
        strncpy(out_parent, "/", max_parent - 1);
        out_parent[max_parent - 1] = '\0';
        strncpy(out_leaf, path, max_leaf - 1);
        out_leaf[max_leaf - 1] = '\0';
    } else if (last_slash == 0) {
        strncpy(out_parent, "/", max_parent - 1);
        out_parent[max_parent - 1] = '\0';
        strncpy(out_leaf, path + 1, max_leaf - 1);
        out_leaf[max_leaf - 1] = '\0';
    } else {
        size_t p_len = (size_t)last_slash;
        if (p_len >= max_parent) p_len = max_parent - 1;
        memcpy(out_parent, path, p_len);
        out_parent[p_len] = '\0';

        strncpy(out_leaf, path + last_slash + 1, max_leaf - 1);
        out_leaf[max_leaf - 1] = '\0';
    }
}

/* --------------------------------------------------------------------------
 * VFS Driver Callback: Mount
 * -------------------------------------------------------------------------- */
static VFS_Node* bofs_vfs_mount_cb(BlockDevice* device) {
    if (!device) {
        return NULL;
    }

    bofs_mount_context_t* mctx = (bofs_mount_context_t*)kmalloc(sizeof(bofs_mount_context_t));
    if (!mctx) {
        return NULL;
    }
    memset(mctx, 0, sizeof(bofs_mount_context_t));
    mctx->bdev = device;

    /* 1. Read Superblock from Block 0 (Sectors 0..7) */
    uint8_t sb_buf[BOFS_BLOCK_SIZE] __attribute__((aligned(64)));
    if (!block_device_read(device->id, 0, BOFS_VFS_SECTORS_PER_BLOCK, sb_buf)) {
        kfree(mctx);
        return NULL;
    }
    memcpy(&mctx->sb, sb_buf, sizeof(bofs_superblock_t));

    /* 2. Validate Superblock Signature & CRC */
    uint64_t total_volume_blocks = device->sector_count / BOFS_VFS_SECTORS_PER_BLOCK;
    if (bofs_validate_superblock(&mctx->sb, total_volume_blocks) != BOFS_VALID_OK) {
        kfree(mctx);
        return NULL;
    }

    /* 3. Initialize WAL Engine and Run Recovery */
    bofs_wal_recovery_stats_t rec_stats;
    memset(&rec_stats, 0, sizeof(rec_stats));
    int wal_res = bofs_wal_mount(&mctx->wal, device, &mctx->sb, &rec_stats);
    if (wal_res != BOFS_WAL_OK) {
        kfree(mctx);
        return NULL;
    }

    int rec_res = bofs_wal_recover(&mctx->wal, &rec_stats);
    if (rec_res != BOFS_WAL_OK) {
        // Recovery failed: fail-closed! Do not mount.
        kfree(mctx);
        return NULL;
    }
    mctx->wal_active = true;

    /* 4. Initialize Block Allocator and File System Context */
    bofs_allocator_init(&mctx->alloc, device);
    bofs_fs_init(&mctx->fs, device, &mctx->alloc);

    /* 5. Create VFS Root Node */
    VFS_Node* root = (VFS_Node*)kmalloc(sizeof(VFS_Node));
    if (!root) {
        kfree(mctx);
        return NULL;
    }
    memset(root, 0, sizeof(VFS_Node));
    strcpy(root->name, "/");
    root->type = VFS_MOUNTPOINT;
    root->size = 0;
    root->fs_driver = &bofs_fs_driver;
    root->private_data = mctx;

    return root;
}

/* --------------------------------------------------------------------------
 * VFS Driver Callback: Unmount
 * -------------------------------------------------------------------------- */
static int bofs_vfs_unmount_cb(VFS_Node* root_node) {
    if (!root_node || !root_node->private_data) {
        return -1;
    }

    bofs_mount_context_t* mctx = (bofs_mount_context_t*)root_node->private_data;

    /* Checkpoint WAL and flush dirty blocks */
    if (mctx->wal_active) {
        bofs_wal_checkpoint(&mctx->wal);
    }
    bofs_fs_flush(&mctx->fs);

    kfree(mctx);
    root_node->private_data = NULL;
    kfree(root_node);
    return 0;
}

/* --------------------------------------------------------------------------
 * VFS Driver Callback: Open
 * -------------------------------------------------------------------------- */
static int bofs_vfs_open_cb(VFS_Node* node, const char* path) {
    if (!node || !path) {
        return -22; /* -EINVAL */
    }

    bofs_mount_context_t* mctx = NULL;
    if (node->parent && node->parent->private_data) {
        mctx = (bofs_mount_context_t*)node->parent->private_data;
    } else if (node->private_data) {
        mctx = (bofs_mount_context_t*)node->private_data;
    }

    if (!mctx) {
        return -22;
    }

    bofs_cred_t cred = bofs_vfs_get_caller_cred();
    uint64_t target_ino = 0;
    uint8_t target_type = 0;

    int r = bofs_sec_path_resolve(&mctx->fs, BOFS_ROOT_INODE, BOFS_ROOT_INODE,
                                  path, &cred, &target_ino, &target_type);
    if (r != BOFS_SEC_OK) {
        return r;
    }

    bofs_inode_t inode;
    r = bofs_inode_read(&mctx->fs, target_ino, &inode);
    if (r != BOFS_FILE_OK) {
        return r;
    }

    /* Verify read permission */
    r = bofs_check_file_read(&inode, &cred);
    if (r != BOFS_SEC_OK) {
        return r;
    }

    bofs_vfs_handle_t* handle = (bofs_vfs_handle_t*)kmalloc(sizeof(bofs_vfs_handle_t));
    if (!handle) {
        return -12; /* -ENOMEM */
    }
    memset(handle, 0, sizeof(bofs_vfs_handle_t));
    handle->mctx = mctx;
    handle->inode_num = target_ino;
    handle->generation = inode.generation;
    handle->mode = inode.mode;
    handle->file_size = inode.size_bytes;
    handle->is_dir = (target_type == BOFS_FT_DIR);

    if (!handle->is_dir) {
        r = bofs_file_open(&mctx->fs, target_ino, inode.generation, &handle->file);
        if (r != BOFS_FILE_OK) {
            kfree(handle);
            return r;
        }
    }

    node->type = handle->is_dir ? VFS_DIRECTORY : VFS_FILE;
    node->size = inode.size_bytes;
    node->private_data = handle;

    return 0;
}

/* --------------------------------------------------------------------------
 * VFS Driver Callback: Read
 * -------------------------------------------------------------------------- */
static int bofs_vfs_read_cb(VFS_Node* node, uint64_t offset, uint32_t size, void* buffer) {
    if (!node || !node->private_data || !buffer) {
        return -22;
    }
    bofs_vfs_handle_t* handle = (bofs_vfs_handle_t*)node->private_data;
    if (handle->is_dir) {
        return -21; /* -EISDIR */
    }

    bofs_cred_t cred = bofs_vfs_get_caller_cred();
    uint64_t bytes_read = 0;
    int r = bofs_sec_read_file(&handle->mctx->fs, handle->inode_num, &cred,
                               offset, buffer, (uint64_t)size, &bytes_read);
    if (r != BOFS_SEC_OK) {
        return r;
    }
    return (int)bytes_read;
}

/* --------------------------------------------------------------------------
 * VFS Driver Callback: Write
 * -------------------------------------------------------------------------- */
static int bofs_vfs_write_cb(VFS_Node* node, uint64_t offset, uint32_t size, void* buffer) {
    if (!node || !node->private_data || !buffer) {
        return -22;
    }
    bofs_vfs_handle_t* handle = (bofs_vfs_handle_t*)node->private_data;
    if (handle->is_dir) {
        return -21; /* -EISDIR */
    }

    bofs_cred_t cred = bofs_vfs_get_caller_cred();

    /* Begin Phase 8 WAL Transaction */
    bofs_tx_t* tx = NULL;
    int tx_res = bofs_tx_begin(&handle->mctx->wal, &tx);
    if (tx_res != BOFS_WAL_OK) {
        return tx_res;
    }

    uint64_t bytes_written = 0;
    int r = bofs_sec_write_file(&handle->mctx->fs, handle->inode_num, &cred,
                                offset, buffer, (uint64_t)size, &bytes_written);
    if (r == BOFS_SEC_OK) {
        bofs_tx_commit(&handle->mctx->wal, tx);
        if (offset + bytes_written > node->size) {
            node->size = offset + bytes_written;
        }
        return (int)bytes_written;
    } else {
        bofs_tx_abort(&handle->mctx->wal, tx);
        return r;
    }
}

/* --------------------------------------------------------------------------
 * VFS Driver Callback: Close
 * -------------------------------------------------------------------------- */
static int bofs_vfs_close_cb(VFS_Node* node) {
    if (!node || !node->private_data) {
        return 0;
    }
    bofs_vfs_handle_t* handle = (bofs_vfs_handle_t*)node->private_data;
    if (!handle->is_dir && handle->file.is_open) {
        bofs_file_close(&handle->file);
    }
    kfree(handle);
    node->private_data = NULL;
    return 0;
}

/* --------------------------------------------------------------------------
 * VFS Driver Callback: Readdir
 * -------------------------------------------------------------------------- */
static int bofs_vfs_readdir_cb(VFS_Node* node, const char* path, int index, vfs_dirent_t* out_entry) {
    if (!node || !path || !out_entry) {
        return -22;
    }
    bofs_mount_context_t* mctx = (bofs_mount_context_t*)node->private_data;
    if (!mctx) return -22;

    bofs_cred_t cred = bofs_vfs_get_caller_cred();
    uint64_t dir_ino = 0;
    uint8_t dir_type = 0;

    int r = bofs_sec_path_resolve(&mctx->fs, BOFS_ROOT_INODE, BOFS_ROOT_INODE,
                                  path, &cred, &dir_ino, &dir_type);
    if (r != BOFS_SEC_OK) {
        return r;
    }
    if (dir_type != BOFS_FT_DIR) {
        return -20; /* -ENOTDIR */
    }

    bofs_dirent_t dirent;
    uint32_t count = 0;
    r = bofs_sec_readdir(&mctx->fs, dir_ino, &cred, (uint32_t)index, &dirent, 1, &count);
    if (r != BOFS_SEC_OK) {
        return r;
    }

    if (count == 0) {
        return 1; /* EOF */
    }

    strncpy(out_entry->name, dirent.name, sizeof(out_entry->name) - 1);
    out_entry->name[sizeof(out_entry->name) - 1] = '\0';
    out_entry->is_directory = (dirent.file_type == BOFS_FT_DIR) ? 1 : 0;
    out_entry->cluster = (uint32_t)dirent.inode_num;
    out_entry->size = 0;
    if (dirent.file_type != BOFS_FT_DIR) {
        bofs_inode_t ino;
        if (bofs_inode_read(&mctx->fs, dirent.inode_num, &ino) == BOFS_FILE_OK) {
            out_entry->size = (uint32_t)ino.size_bytes;
        }
    }
    return 0;
}

/* --------------------------------------------------------------------------
 * VFS Driver Callback: Mkdir
 * -------------------------------------------------------------------------- */
static int bofs_vfs_mkdir_cb(VFS_Node* node, const char* path) {
    if (!node || !path) return -22;
    bofs_mount_context_t* mctx = (bofs_mount_context_t*)node->private_data;
    if (!mctx) return -22;

    char parent_path[256];
    char leaf_name[128];
    bofs_vfs_split_path(path, parent_path, sizeof(parent_path), leaf_name, sizeof(leaf_name));

    bofs_cred_t cred = bofs_vfs_get_caller_cred();
    uint64_t parent_ino = 0;
    uint8_t parent_type = 0;
    int r = bofs_sec_path_resolve(&mctx->fs, BOFS_ROOT_INODE, BOFS_ROOT_INODE,
                                  parent_path, &cred, &parent_ino, &parent_type);
    if (r != BOFS_SEC_OK) return r;

    bofs_tx_t* tx = NULL;
    bofs_tx_begin(&mctx->wal, &tx);

    uint64_t new_ino = 0;
    r = bofs_sec_mkdir(&mctx->fs, parent_ino, leaf_name, &cred, 0755, &new_ino);
    if (r == BOFS_SEC_OK) {
        bofs_tx_commit(&mctx->wal, tx);
        return 0;
    } else {
        bofs_tx_abort(&mctx->wal, tx);
        return r;
    }
}

/* --------------------------------------------------------------------------
 * VFS Driver Callback: Create
 * -------------------------------------------------------------------------- */
static int bofs_vfs_create_cb(VFS_Node* node, const char* path) {
    if (!node || !path) return -22;
    bofs_mount_context_t* mctx = (bofs_mount_context_t*)node->private_data;
    if (!mctx) return -22;

    char parent_path[256];
    char leaf_name[128];
    bofs_vfs_split_path(path, parent_path, sizeof(parent_path), leaf_name, sizeof(leaf_name));

    bofs_cred_t cred = bofs_vfs_get_caller_cred();
    uint64_t parent_ino = 0;
    uint8_t parent_type = 0;
    int r = bofs_sec_path_resolve(&mctx->fs, BOFS_ROOT_INODE, BOFS_ROOT_INODE,
                                  parent_path, &cred, &parent_ino, &parent_type);
    if (r != BOFS_SEC_OK) return r;

    bofs_tx_t* tx = NULL;
    bofs_tx_begin(&mctx->wal, &tx);

    uint64_t new_ino = 0;
    r = bofs_sec_create_file(&mctx->fs, parent_ino, leaf_name, &cred, 0644, &new_ino);
    if (r == BOFS_SEC_OK) {
        bofs_tx_commit(&mctx->wal, tx);
        return 0;
    } else {
        bofs_tx_abort(&mctx->wal, tx);
        return r;
    }
}

/* --------------------------------------------------------------------------
 * VFS Driver Callback: Rename
 * -------------------------------------------------------------------------- */
static int bofs_vfs_rename_cb(VFS_Node* node, const char* old_path, const char* new_path) {
    if (!node || !old_path || !new_path) return -22;
    bofs_mount_context_t* mctx = (bofs_mount_context_t*)node->private_data;
    if (!mctx) return -22;

    char old_parent_path[256];
    char old_leaf[128];
    bofs_vfs_split_path(old_path, old_parent_path, sizeof(old_parent_path), old_leaf, sizeof(old_leaf));

    char new_parent_path[256];
    char new_leaf[128];
    bofs_vfs_split_path(new_path, new_parent_path, sizeof(new_parent_path), new_leaf, sizeof(new_leaf));

    bofs_cred_t cred = bofs_vfs_get_caller_cred();
    uint64_t old_parent_ino = 0;
    uint8_t old_p_type = 0;
    int r = bofs_sec_path_resolve(&mctx->fs, BOFS_ROOT_INODE, BOFS_ROOT_INODE,
                                  old_parent_path, &cred, &old_parent_ino, &old_p_type);
    if (r != BOFS_SEC_OK) return r;

    uint64_t new_parent_ino = 0;
    uint8_t new_p_type = 0;
    r = bofs_sec_path_resolve(&mctx->fs, BOFS_ROOT_INODE, BOFS_ROOT_INODE,
                              new_parent_path, &cred, &new_parent_ino, &new_p_type);
    if (r != BOFS_SEC_OK) return r;

    bofs_tx_t* tx = NULL;
    bofs_tx_begin(&mctx->wal, &tx);

    r = bofs_sec_rename(&mctx->fs, old_parent_ino, old_leaf, new_parent_ino, new_leaf, &cred);
    if (r == BOFS_SEC_OK) {
        bofs_tx_commit(&mctx->wal, tx);
        return 0;
    } else {
        bofs_tx_abort(&mctx->wal, tx);
        return r;
    }
}

/* --------------------------------------------------------------------------
 * VFS Driver Callback: Delete (Unlink)
 * -------------------------------------------------------------------------- */
static int bofs_vfs_delete_cb(VFS_Node* node, const char* path) {
    if (!node || !path) return -22;
    bofs_mount_context_t* mctx = (bofs_mount_context_t*)node->private_data;
    if (!mctx) return -22;

    char parent_path[256];
    char leaf_name[128];
    bofs_vfs_split_path(path, parent_path, sizeof(parent_path), leaf_name, sizeof(leaf_name));

    bofs_cred_t cred = bofs_vfs_get_caller_cred();
    uint64_t parent_ino = 0;
    uint8_t parent_type = 0;
    int r = bofs_sec_path_resolve(&mctx->fs, BOFS_ROOT_INODE, BOFS_ROOT_INODE,
                                  parent_path, &cred, &parent_ino, &parent_type);
    if (r != BOFS_SEC_OK) return r;

    bofs_tx_t* tx = NULL;
    bofs_tx_begin(&mctx->wal, &tx);

    r = bofs_sec_unlink(&mctx->fs, parent_ino, leaf_name, &cred);
    if (r == BOFS_SEC_OK) {
        bofs_tx_commit(&mctx->wal, tx);
        return 0;
    } else {
        bofs_tx_abort(&mctx->wal, tx);
        return r;
    }
}

/* --------------------------------------------------------------------------
 * VFS Driver Callback: Rmdir
 * -------------------------------------------------------------------------- */
static int bofs_vfs_rmdir_cb(VFS_Node* node, const char* path) {
    if (!node || !path) return -22;
    bofs_mount_context_t* mctx = (bofs_mount_context_t*)node->private_data;
    if (!mctx) return -22;

    char parent_path[256];
    char leaf_name[128];
    bofs_vfs_split_path(path, parent_path, sizeof(parent_path), leaf_name, sizeof(leaf_name));

    bofs_cred_t cred = bofs_vfs_get_caller_cred();
    uint64_t parent_ino = 0;
    uint8_t parent_type = 0;
    int r = bofs_sec_path_resolve(&mctx->fs, BOFS_ROOT_INODE, BOFS_ROOT_INODE,
                                  parent_path, &cred, &parent_ino, &parent_type);
    if (r != BOFS_SEC_OK) return r;

    bofs_tx_t* tx = NULL;
    bofs_tx_begin(&mctx->wal, &tx);

    r = bofs_sec_rmdir(&mctx->fs, parent_ino, leaf_name, &cred);
    if (r == BOFS_SEC_OK) {
        bofs_tx_commit(&mctx->wal, tx);
        return 0;
    } else {
        bofs_tx_abort(&mctx->wal, tx);
        return r;
    }
}

/* --------------------------------------------------------------------------
 * VFS Driver Callback: Stat
 * -------------------------------------------------------------------------- */
static int bofs_vfs_stat_cb(VFS_Node* node, const char* path, atoms_stat_t* out_stat) {
    if (!node || !path || !out_stat) return -22;
    bofs_mount_context_t* mctx = (bofs_mount_context_t*)node->private_data;
    if (!mctx) return -22;

    bofs_cred_t cred = bofs_vfs_get_caller_cred();
    uint64_t target_ino = 0;
    uint8_t target_type = 0;
    int r = bofs_sec_path_resolve(&mctx->fs, BOFS_ROOT_INODE, BOFS_ROOT_INODE,
                                  path, &cred, &target_ino, &target_type);
    if (r != BOFS_SEC_OK) return r;

    bofs_inode_t inode;
    r = bofs_inode_read(&mctx->fs, target_ino, &inode);
    if (r != BOFS_FILE_OK) return r;

    memset(out_stat, 0, sizeof(atoms_stat_t));
    out_stat->st_ino = inode.inode_num;
    out_stat->st_mode = inode.mode;
    out_stat->st_nlink = inode.link_count;
    out_stat->st_uid = inode.uid;
    out_stat->st_gid = inode.gid;
    out_stat->st_size = inode.size_bytes;
    out_stat->st_blocks = (inode.size_bytes + 511) / 512;
    out_stat->st_atime = inode.atime_sec;
    out_stat->st_mtime = inode.mtime_sec;
    out_stat->st_ctime = inode.ctime_sec;

    return 0;
}

/* --------------------------------------------------------------------------
 * Exported Filesystem Driver Instance
 * -------------------------------------------------------------------------- */
FilesystemDriver bofs_fs_driver = {
    .name = "bofs",
    .mount = bofs_vfs_mount_cb,
    .unmount = bofs_vfs_unmount_cb,
    .open = bofs_vfs_open_cb,
    .read = bofs_vfs_read_cb,
    .write = bofs_vfs_write_cb,
    .close = bofs_vfs_close_cb,
    .readdir = bofs_vfs_readdir_cb,
    .mkdir = bofs_vfs_mkdir_cb,
    .create = bofs_vfs_create_cb,
    .rename = bofs_vfs_rename_cb,
    .delete = bofs_vfs_delete_cb,
    .rmdir = bofs_vfs_rmdir_cb,
    .stat = bofs_vfs_stat_cb
};

void bofs_vfs_init(void) {
    vfs_register_fs(&bofs_fs_driver);
}
