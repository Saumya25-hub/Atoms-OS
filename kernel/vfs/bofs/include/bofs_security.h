#ifndef BOFS_SECURITY_H
#define BOFS_SECURITY_H

#include "bofs_format.h"
#include "bofs_alloc.h"
#include "bofs_file.h"
#include "bofs_dir.h"
#include "bofs_validator.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Permission Masks (POSIX-Compatible Standard R-W-X)
 * -------------------------------------------------------------------------- */
#define BOFS_PERM_EXEC             0x01U  /* X bit: execute file / search & traverse directory */
#define BOFS_PERM_WRITE            0x02U  /* W bit: write/truncate file / mutate directory */
#define BOFS_PERM_READ             0x04U  /* R bit: read file data / enumerate directory */

/* --------------------------------------------------------------------------
 * Security Return & Error Codes
 * -------------------------------------------------------------------------- */
#define BOFS_SEC_OK                0
#define BOFS_ERR_SEC_SPOOF        -1   /* Credential forgery / unauthorized identity */
#define BOFS_ERR_SEC_NOT_FOUND    -2   /* Target object not found (ENOENT) */
#define BOFS_ERR_SEC_IO           -5   /* Underlying block I/O failure (EIO) */
#define BOFS_ERR_SEC_DENIED       -13  /* Permission denied (EACCES) */
#define BOFS_ERR_SEC_EXISTS       -17  /* Entry exists (EEXIST) */
#define BOFS_ERR_SEC_NOT_DIR      -20  /* Not a directory (ENOTDIR) */
#define BOFS_ERR_SEC_IS_DIR       -21  /* Is a directory (EISDIR) */
#define BOFS_ERR_SEC_INVALID_PARAM -22 /* Invalid argument (EINVAL) */
#define BOFS_ERR_SEC_NOT_EMPTY    -39  /* Directory not empty (ENOTEMPTY) */
#define BOFS_ERR_SEC_CORRUPT      -117 /* Malformed/corrupted metadata (EBADMSG) */

/* Default Policies */
#define BOFS_DEFAULT_FILE_MODE    0644U /* rw-r--r-- */
#define BOFS_DEFAULT_DIR_MODE     0755U /* rwxr-xr-x */

/* --------------------------------------------------------------------------
 * Structure: Security Credential Snapshot
 * -------------------------------------------------------------------------- */
typedef struct {
    uint32_t uid;               /* User ID */
    uint32_t gid;               /* Group ID */
    uint64_t capability_mask;   /* Capability bitmask (BOS_CAP_*) */
} bofs_cred_t;

/* --------------------------------------------------------------------------
 * Canonical Permission Evaluator (Single Source of Truth)
 * -------------------------------------------------------------------------- */

/* Evaluates whether cred has req_mask (combination of R, W, X) on inode */
int bofs_check_permission(const bofs_inode_t* inode, const bofs_cred_t* cred, uint32_t req_mask);

/* Specialized permission checks */
int bofs_check_file_read(const bofs_inode_t* inode, const bofs_cred_t* cred);
int bofs_check_file_write(const bofs_inode_t* inode, const bofs_cred_t* cred);
int bofs_check_file_exec(const bofs_inode_t* inode, const bofs_cred_t* cred);
int bofs_check_dir_search(const bofs_inode_t* inode, const bofs_cred_t* cred);
int bofs_check_dir_read(const bofs_inode_t* inode, const bofs_cred_t* cred);
int bofs_check_dir_modify(const bofs_inode_t* inode, const bofs_cred_t* cred);

/* Validates security metadata integrity (checksum, valid type in mode bits) */
int bofs_validate_security_metadata(const bofs_inode_t* inode);

/* Construct credential helpers */
bofs_cred_t bofs_cred_create(uint32_t uid, uint32_t gid, uint64_t caps);
bofs_cred_t bofs_cred_system(void);

/* --------------------------------------------------------------------------
 * Ownership & Mode Mutation APIs (With Atomicity & Inode CRC Updates)
 * -------------------------------------------------------------------------- */
int bofs_sec_set_ownership(bofs_file_system_t* fs, uint64_t inode_num,
                           const bofs_cred_t* cred, uint32_t new_uid, uint32_t new_gid);

int bofs_sec_set_mode(bofs_file_system_t* fs, uint64_t inode_num,
                      const bofs_cred_t* cred, uint16_t new_mode);

/* --------------------------------------------------------------------------
 * Secure Filesystem Operations Wrappers
 * -------------------------------------------------------------------------- */

/* File I/O */
int bofs_sec_read_file(bofs_file_system_t* fs, uint64_t ino, const bofs_cred_t* cred,
                       uint64_t offset, void* buffer, uint64_t length, uint64_t* out_read);

int bofs_sec_write_file(bofs_file_system_t* fs, uint64_t ino, const bofs_cred_t* cred,
                        uint64_t offset, const void* buffer, uint64_t length, uint64_t* out_written);

int bofs_sec_truncate_file(bofs_file_system_t* fs, uint64_t ino, const bofs_cred_t* cred,
                           uint64_t new_size);

/* Creation & Deletion */
int bofs_sec_create_file(bofs_file_system_t* fs, uint64_t parent_ino, const char* name,
                         const bofs_cred_t* cred, uint16_t mode, uint64_t* out_ino);

int bofs_sec_mkdir(bofs_file_system_t* fs, uint64_t parent_ino, const char* name,
                   const bofs_cred_t* cred, uint16_t mode, uint64_t* out_ino);

int bofs_sec_unlink(bofs_file_system_t* fs, uint64_t parent_ino, const char* name,
                    const bofs_cred_t* cred);

int bofs_sec_rmdir(bofs_file_system_t* fs, uint64_t parent_ino, const char* name,
                   const bofs_cred_t* cred);

int bofs_sec_rename(bofs_file_system_t* fs, uint64_t old_parent, const char* old_name,
                    uint64_t new_parent, const char* new_name, const bofs_cred_t* cred);

/* Directory Traversal & Enumeration */
int bofs_sec_lookup(bofs_file_system_t* fs, uint64_t dir_ino, const char* name,
                    const bofs_cred_t* cred, uint64_t* out_ino, uint8_t* out_type);

int bofs_sec_readdir(bofs_file_system_t* fs, uint64_t dir_ino, const bofs_cred_t* cred,
                     uint32_t offset_cookie, bofs_dirent_t* out_dirents,
                     uint32_t max_count, uint32_t* out_count);

int bofs_sec_path_resolve(bofs_file_system_t* fs, uint64_t root_ino, uint64_t cwd_ino,
                          const char* path, const bofs_cred_t* cred,
                          uint64_t* out_ino, uint8_t* out_type);

#ifdef __cplusplus
}
#endif

#endif /* BOFS_SECURITY_H */
