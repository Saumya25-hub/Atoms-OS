#ifndef BOFS_DIR_H
#define BOFS_DIR_H

#include "bofs_format.h"
#include "bofs_alloc.h"
#include "bofs_file.h"
#include "bofs_validator.h"
#include "kernel/vfs/vfs_legacy/storage/include/block_device.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Directory Engine Return & Error Codes
 * -------------------------------------------------------------------------- */
#define BOFS_DIR_OK                      0
#define BOFS_ERR_DIR_NOT_FOUND          -2   /* ENOENT */
#define BOFS_ERR_DIR_IO                 -5   /* EIO */
#define BOFS_ERR_DIR_CORRUPT_NODE       -11  /* BOFS_ERR_CORRUPTED_DIR_NODE */
#define BOFS_ERR_DIR_ENTRY_EXISTS       -17  /* EEXIST */
#define BOFS_ERR_DIR_ROOT_ESCAPE        -18  /* Root escape prevention */
#define BOFS_ERR_DIR_NOT_A_DIR          -20  /* ENOTDIR */
#define BOFS_ERR_DIR_IS_A_DIR           -21  /* EISDIR */
#define BOFS_ERR_DIR_INVALID_PARAM      -22  /* EINVAL */
#define BOFS_ERR_DIR_INVALID_NAME       -23  /* Invalid / malformed filename */
#define BOFS_ERR_DIR_NO_SPACE           -28  /* ENOSPC */
#define BOFS_ERR_DIR_NAME_TOO_LONG      -36  /* ENAMETOOLONG */
#define BOFS_ERR_DIR_NOT_EMPTY          -39  /* ENOTEMPTY */
#define BOFS_ERR_DIR_CYCLE_DETECTED     -40  /* ELOOP */
#define BOFS_ERR_DIR_CORRUPT_METADATA   -117 /* Malformed metadata */
#define BOFS_ERR_DIR_NODE_FULL          -120 /* Node full */

#define BOFS_MAX_PATH_LEN               4096U
#define BOFS_MAX_TRAVERSAL_DEPTH        256U

/* --------------------------------------------------------------------------
 * Name Comparator & Validation
 * -------------------------------------------------------------------------- */

/* Validate that name is valid UTF-8, no NUL, no '/', 1 <= len <= 255 */
int bofs_dir_validate_name(const char* name, size_t* out_len);

/* Deterministic comparator: primary key hash, secondary canonical byte strcmp */
int bofs_dir_cmp(uint64_t hash_a, const char* name_a, uint64_t hash_b, const char* name_b);

/* --------------------------------------------------------------------------
 * Directory Operations API
 * -------------------------------------------------------------------------- */

/* Lookup an entry by name within directory inode */
int bofs_dir_lookup(bofs_file_system_t* fs, uint64_t dir_ino, const char* name,
                    uint64_t* out_child_ino, uint32_t* out_child_gen, uint8_t* out_type);

/* Insert an entry into directory inode */
int bofs_dir_insert(bofs_file_system_t* fs, uint64_t dir_ino, const char* name,
                    uint64_t child_ino, uint32_t child_gen, uint8_t type);

/* Remove an entry by name from directory inode (does not delete child object) */
int bofs_dir_remove(bofs_file_system_t* fs, uint64_t dir_ino, const char* name);

/* Create a new directory under parent_ino, allocating inode and root B+Tree block */
int bofs_mkdir(bofs_file_system_t* fs, uint64_t parent_ino, const char* name,
               uint16_t mode, uint64_t* out_ino);

/* Remove an empty directory (fails with BOFS_ERR_DIR_NOT_EMPTY if non-empty) */
int bofs_rmdir(bofs_file_system_t* fs, uint64_t parent_ino, const char* name);

/* Rename an entry within same directory or across different directories */
int bofs_rename(bofs_file_system_t* fs, uint64_t old_parent_ino, const char* old_name,
                uint64_t new_parent_ino, const char* new_name);

/* Enumerate entries from directory inode */
int bofs_readdir(bofs_file_system_t* fs, uint64_t dir_ino, uint32_t start_index,
                 bofs_dirent_t* out_entries, uint32_t max_entries, uint32_t* out_count);

/* Low-level pathname resolution from root / cwd */
int bofs_path_resolve(bofs_file_system_t* fs, uint64_t root_ino, uint64_t cwd_ino,
                      const char* path, uint64_t* out_ino, uint32_t* out_gen, uint8_t* out_type);

/* Consistency & integrity verification for directory B+Tree */
int bofs_dir_validate(bofs_file_system_t* fs, uint64_t dir_ino);

/* Read and validate a directory node block from storage */
int bofs_dir_read_node(bofs_file_system_t* fs, uint64_t block_idx, bofs_dir_node_t* out_node);

#ifdef __cplusplus
}
#endif

#endif /* BOFS_DIR_H */
