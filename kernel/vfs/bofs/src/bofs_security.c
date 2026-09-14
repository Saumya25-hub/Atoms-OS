#include "kernel/vfs/bofs/include/bofs_security.h"
#include "kernel/vfs/bofs/include/bofs_format.h"
#include "kernel/vfs/bofs/include/bofs_alloc.h"
#include "kernel/vfs/bofs/include/bofs_file.h"
#include "kernel/vfs/bofs/include/bofs_dir.h"
#include "kernel/vfs/bofs/include/bofs_validator.h"
#include "kernel/core/lib/include/string.h"

/* --------------------------------------------------------------------------
 * Credential Construction Helpers
 * -------------------------------------------------------------------------- */
bofs_cred_t bofs_cred_create(uint32_t uid, uint32_t gid, uint64_t caps) {
    bofs_cred_t c;
    c.uid = uid;
    c.gid = gid;
    c.capability_mask = caps;
    return c;
}

bofs_cred_t bofs_cred_system(void) {
    return bofs_cred_create(0, 0, 0);
}

/* --------------------------------------------------------------------------
 * Metadata Integrity Validation (Fail-Closed)
 * -------------------------------------------------------------------------- */
int bofs_validate_security_metadata(const bofs_inode_t* inode) {
    if (!inode) return BOFS_ERR_SEC_INVALID_PARAM;
    if (inode->magic != BOFS_INODE_MAGIC) return BOFS_ERR_SEC_CORRUPT;

    /* Verify inode CRC32 over bytes 0x000 to 0x1FB (508 bytes) */
    uint32_t expected_crc = bofs_crc32(inode, offsetof(bofs_inode_t, checksum));
    if (inode->checksum != expected_crc) return BOFS_ERR_SEC_CORRUPT;

    /* Verify valid file type bits */
    uint16_t ftype = inode->mode & BOFS_S_IFMT;
    if (ftype != BOFS_S_IFREG && ftype != BOFS_S_IFDIR) {
        return BOFS_ERR_SEC_CORRUPT;
    }

    /* Verify no unsupported bits (setuid 04000, setgid 02000, sticky 01000) are set */
    if (inode->mode & 0x0E00) {
        return BOFS_ERR_SEC_CORRUPT;
    }

    return BOFS_SEC_OK;
}

/* --------------------------------------------------------------------------
 * Canonical Permission Evaluator
 * -------------------------------------------------------------------------- */
int bofs_check_permission(const bofs_inode_t* inode, const bofs_cred_t* cred, uint32_t req_mask) {
    /* 1. Fail-closed on metadata corruption or invalid arguments */
    int val = bofs_validate_security_metadata(inode);
    if (val != BOFS_SEC_OK) return val;
    if (!cred) return BOFS_ERR_SEC_INVALID_PARAM;

    /* 2. Determine permission class (Owner -> Group -> Other) */
    uint32_t granted = 0;
    if (cred->uid == inode->uid) {
        if (inode->mode & BOFS_S_IRUSR) granted |= BOFS_PERM_READ;
        if (inode->mode & BOFS_S_IWUSR) granted |= BOFS_PERM_WRITE;
        if (inode->mode & BOFS_S_IXUSR) granted |= BOFS_PERM_EXEC;
    } else if (cred->gid == inode->gid) {
        if (inode->mode & BOFS_S_IRGRP) granted |= BOFS_PERM_READ;
        if (inode->mode & BOFS_S_IWGRP) granted |= BOFS_PERM_WRITE;
        if (inode->mode & BOFS_S_IXGRP) granted |= BOFS_PERM_EXEC;
    } else {
        if (inode->mode & BOFS_S_IROTH) granted |= BOFS_PERM_READ;
        if (inode->mode & BOFS_S_IWOTH) granted |= BOFS_PERM_WRITE;
        if (inode->mode & BOFS_S_IXOTH) granted |= BOFS_PERM_EXEC;
    }

    /* 3. Check requested bits */
    if ((granted & req_mask) == req_mask) {
        return BOFS_SEC_OK;
    }
    return BOFS_ERR_SEC_DENIED;
}

/* --------------------------------------------------------------------------
 * Specialized Permission Checks
 * -------------------------------------------------------------------------- */
int bofs_check_file_read(const bofs_inode_t* inode, const bofs_cred_t* cred) {
    if (!inode) return BOFS_ERR_SEC_INVALID_PARAM;
    if ((inode->mode & BOFS_S_IFMT) == BOFS_S_IFDIR) return BOFS_ERR_SEC_IS_DIR;
    return bofs_check_permission(inode, cred, BOFS_PERM_READ);
}

int bofs_check_file_write(const bofs_inode_t* inode, const bofs_cred_t* cred) {
    if (!inode) return BOFS_ERR_SEC_INVALID_PARAM;
    if ((inode->mode & BOFS_S_IFMT) == BOFS_S_IFDIR) return BOFS_ERR_SEC_IS_DIR;
    return bofs_check_permission(inode, cred, BOFS_PERM_WRITE);
}

int bofs_check_file_exec(const bofs_inode_t* inode, const bofs_cred_t* cred) {
    if (!inode) return BOFS_ERR_SEC_INVALID_PARAM;
    if ((inode->mode & BOFS_S_IFMT) == BOFS_S_IFDIR) return BOFS_ERR_SEC_IS_DIR;
    return bofs_check_permission(inode, cred, BOFS_PERM_EXEC);
}

int bofs_check_dir_search(const bofs_inode_t* inode, const bofs_cred_t* cred) {
    if (!inode) return BOFS_ERR_SEC_INVALID_PARAM;
    if ((inode->mode & BOFS_S_IFMT) != BOFS_S_IFDIR) return BOFS_ERR_SEC_NOT_DIR;
    return bofs_check_permission(inode, cred, BOFS_PERM_EXEC);
}

int bofs_check_dir_read(const bofs_inode_t* inode, const bofs_cred_t* cred) {
    if (!inode) return BOFS_ERR_SEC_INVALID_PARAM;
    if ((inode->mode & BOFS_S_IFMT) != BOFS_S_IFDIR) return BOFS_ERR_SEC_NOT_DIR;
    return bofs_check_permission(inode, cred, BOFS_PERM_READ);
}

int bofs_check_dir_modify(const bofs_inode_t* inode, const bofs_cred_t* cred) {
    if (!inode) return BOFS_ERR_SEC_INVALID_PARAM;
    if ((inode->mode & BOFS_S_IFMT) != BOFS_S_IFDIR) return BOFS_ERR_SEC_NOT_DIR;
    return bofs_check_permission(inode, cred, BOFS_PERM_WRITE | BOFS_PERM_EXEC);
}

/* --------------------------------------------------------------------------
 * Ownership & Mode Mutation APIs
 * -------------------------------------------------------------------------- */
int bofs_sec_set_ownership(bofs_file_system_t* fs, uint64_t inode_num,
                           const bofs_cred_t* cred, uint32_t new_uid, uint32_t new_gid) {
    if (!fs || !cred) return BOFS_ERR_SEC_INVALID_PARAM;

    bofs_inode_t ino;
    int r = bofs_inode_read(fs, inode_num, &ino);
    if (r != BOFS_FILE_OK) return r;

    /* Only file owner or UID 0 (system) can change ownership */
    if (cred->uid != ino.uid && cred->uid != 0) {
        return BOFS_ERR_SEC_DENIED;
    }

    ino.uid = new_uid;
    ino.gid = new_gid;
    ino.checksum = bofs_crc32(&ino, offsetof(bofs_inode_t, checksum));

    r = bofs_inode_write(fs, &ino);
    if (r != BOFS_FILE_OK) return r;

    bofs_fs_flush(fs);
    return BOFS_SEC_OK;
}

int bofs_sec_set_mode(bofs_file_system_t* fs, uint64_t inode_num,
                      const bofs_cred_t* cred, uint16_t new_mode) {
    if (!fs || !cred) return BOFS_ERR_SEC_INVALID_PARAM;

    bofs_inode_t ino;
    int r = bofs_inode_read(fs, inode_num, &ino);
    if (r != BOFS_FILE_OK) return r;

    /* Only file owner or UID 0 (system) can change mode */
    if (cred->uid != ino.uid && cred->uid != 0) {
        return BOFS_ERR_SEC_DENIED;
    }

    /* Preserve file type bits */
    ino.mode = (ino.mode & BOFS_S_IFMT) | (new_mode & 07777U);
    ino.checksum = bofs_crc32(&ino, offsetof(bofs_inode_t, checksum));

    r = bofs_inode_write(fs, &ino);
    if (r != BOFS_FILE_OK) return r;

    bofs_fs_flush(fs);
    return BOFS_SEC_OK;
}

/* --------------------------------------------------------------------------
 * Secure Filesystem Operations Wrappers
 * -------------------------------------------------------------------------- */

int bofs_sec_read_file(bofs_file_system_t* fs, uint64_t ino, const bofs_cred_t* cred,
                       uint64_t offset, void* buffer, uint64_t length, uint64_t* out_read) {
    if (!fs || !cred || !buffer) return BOFS_ERR_SEC_INVALID_PARAM;

    bofs_inode_t inode;
    int r = bofs_inode_read(fs, ino, &inode);
    if (r != BOFS_FILE_OK) return r;

    int perm = bofs_check_file_read(&inode, cred);
    if (perm != BOFS_SEC_OK) return perm;

    bofs_file_t file;
    r = bofs_file_open(fs, ino, inode.generation, &file);
    if (r != BOFS_FILE_OK) return r;

    return bofs_file_read(&file, offset, buffer, length, out_read);
}

int bofs_sec_write_file(bofs_file_system_t* fs, uint64_t ino, const bofs_cred_t* cred,
                        uint64_t offset, const void* buffer, uint64_t length, uint64_t* out_written) {
    if (!fs || !cred || !buffer) return BOFS_ERR_SEC_INVALID_PARAM;

    bofs_inode_t inode;
    int r = bofs_inode_read(fs, ino, &inode);
    if (r != BOFS_FILE_OK) return r;

    int perm = bofs_check_file_write(&inode, cred);
    if (perm != BOFS_SEC_OK) return perm;

    bofs_file_t file;
    r = bofs_file_open(fs, ino, inode.generation, &file);
    if (r != BOFS_FILE_OK) return r;

    r = bofs_file_write(&file, offset, buffer, length, out_written);
    if (r == BOFS_FILE_OK) {
        bofs_fs_flush(fs);
    }
    return r;
}

int bofs_sec_truncate_file(bofs_file_system_t* fs, uint64_t ino, const bofs_cred_t* cred,
                           uint64_t new_size) {
    if (!fs || !cred) return BOFS_ERR_SEC_INVALID_PARAM;

    bofs_inode_t inode;
    int r = bofs_inode_read(fs, ino, &inode);
    if (r != BOFS_FILE_OK) return r;

    int perm = bofs_check_file_write(&inode, cred);
    if (perm != BOFS_SEC_OK) return perm;

    bofs_file_t file;
    r = bofs_file_open(fs, ino, inode.generation, &file);
    if (r != BOFS_FILE_OK) return r;

    r = bofs_file_truncate(&file, new_size);
    if (r == BOFS_FILE_OK) {
        bofs_fs_flush(fs);
    }
    return r;
}

int bofs_sec_create_file(bofs_file_system_t* fs, uint64_t parent_ino, const char* name,
                         const bofs_cred_t* cred, uint16_t mode, uint64_t* out_ino) {
    if (!fs || !cred || !name) return BOFS_ERR_SEC_INVALID_PARAM;

    /* 1. Authorize on parent directory: requires WRITE | EXECUTE */
    bofs_inode_t parent_inode;
    int r = bofs_inode_read(fs, parent_ino, &parent_inode);
    if (r != BOFS_FILE_OK) return r;

    int perm = bofs_check_dir_modify(&parent_inode, cred);
    if (perm != BOFS_SEC_OK) return perm;

    /* 2. Check if name already exists in parent */
    uint64_t existing_ino = 0;
    if (bofs_dir_lookup(fs, parent_ino, name, &existing_ino, NULL, NULL) == BOFS_DIR_OK) {
        return BOFS_ERR_SEC_EXISTS;
    }

    /* 3. Allocate child inode */
    uint64_t child_ino = 0;
    r = bofs_inode_alloc(fs, &child_ino);
    if (r != BOFS_FILE_OK) return r;

    /* 4. Initialize child inode */
    bofs_inode_t new_ino;
    memset(&new_ino, 0, sizeof(bofs_inode_t));
    new_ino.magic = BOFS_INODE_MAGIC;
    new_ino.generation = 1;
    new_ino.inode_num = child_ino;
    new_ino.mode = BOFS_S_IFREG | (mode & 0777U);
    new_ino.flags = 0;
    new_ino.uid = cred->uid;
    new_ino.gid = parent_inode.gid;
    new_ino.link_count = 1;
    new_ino.size_bytes = 0;
    new_ino.allocated_blocks = 0;
    new_ino.checksum = bofs_crc32(&new_ino, offsetof(bofs_inode_t, checksum));

    r = bofs_inode_write(fs, &new_ino);
    if (r != BOFS_FILE_OK) {
        bofs_inode_free(fs, child_ino);
        return r;
    }

    /* 5. Insert dirent into parent */
    r = bofs_dir_insert(fs, parent_ino, name, child_ino, 1, BOFS_FT_REG);
    if (r != BOFS_DIR_OK) {
        bofs_inode_free(fs, child_ino);
        return r;
    }

    bofs_fs_flush(fs);
    if (out_ino) *out_ino = child_ino;
    return BOFS_SEC_OK;
}

int bofs_sec_mkdir(bofs_file_system_t* fs, uint64_t parent_ino, const char* name,
                   const bofs_cred_t* cred, uint16_t mode, uint64_t* out_ino) {
    if (!fs || !cred || !name) return BOFS_ERR_SEC_INVALID_PARAM;

    /* 1. Authorize on parent directory: requires WRITE | EXECUTE */
    bofs_inode_t parent_inode;
    int r = bofs_inode_read(fs, parent_ino, &parent_inode);
    if (r != BOFS_FILE_OK) return r;

    int perm = bofs_check_dir_modify(&parent_inode, cred);
    if (perm != BOFS_SEC_OK) return perm;

    /* 2. Check if name already exists */
    uint64_t existing_ino = 0;
    if (bofs_dir_lookup(fs, parent_ino, name, &existing_ino, NULL, NULL) == BOFS_DIR_OK) {
        return BOFS_ERR_SEC_EXISTS;
    }

    /* 3. Call canonical bofs_mkdir */
    uint64_t child_ino = 0;
    r = bofs_mkdir(fs, parent_ino, name, mode, &child_ino);
    if (r != BOFS_DIR_OK) return r;

    /* 4. Update child inode owner to cred->uid */
    bofs_inode_t child_inode;
    if (bofs_inode_read(fs, child_ino, &child_inode) == BOFS_FILE_OK) {
        child_inode.uid = cred->uid;
        child_inode.gid = parent_inode.gid;
        child_inode.checksum = bofs_crc32(&child_inode, offsetof(bofs_inode_t, checksum));
        bofs_inode_write(fs, &child_inode);
        bofs_fs_flush(fs);
    }

    if (out_ino) *out_ino = child_ino;
    return BOFS_SEC_OK;
}

int bofs_sec_unlink(bofs_file_system_t* fs, uint64_t parent_ino, const char* name,
                    const bofs_cred_t* cred) {
    if (!fs || !cred || !name) return BOFS_ERR_SEC_INVALID_PARAM;

    /* 1. Authorize on parent directory: requires WRITE | EXECUTE */
    bofs_inode_t parent_inode;
    int r = bofs_inode_read(fs, parent_ino, &parent_inode);
    if (r != BOFS_FILE_OK) return r;

    int perm = bofs_check_dir_modify(&parent_inode, cred);
    if (perm != BOFS_SEC_OK) return perm;

    /* 2. Lookup target entry */
    uint64_t target_ino = 0;
    uint8_t target_type = 0;
    r = bofs_dir_lookup(fs, parent_ino, name, &target_ino, NULL, &target_type);
    if (r != BOFS_DIR_OK) return BOFS_ERR_SEC_NOT_FOUND;

    if (target_type == BOFS_FT_DIR) {
        return BOFS_ERR_SEC_IS_DIR;
    }

    /* 3. Remove entry from parent */
    r = bofs_dir_remove(fs, parent_ino, name);
    if (r != BOFS_DIR_OK) return r;

    /* 4. Decrement link count on target file */
    bofs_inode_t target_inode;
    r = bofs_inode_read(fs, target_ino, &target_inode);
    if (r == BOFS_FILE_OK) {
        if (target_inode.link_count > 1) {
            target_inode.link_count--;
            bofs_inode_write(fs, &target_inode);
        } else {
            /* Free file contents and inode */
            bofs_file_t file;
            if (bofs_file_open(fs, target_ino, target_inode.generation, &file) == BOFS_FILE_OK) {
                bofs_file_truncate(&file, 0);
            }
            bofs_inode_free(fs, target_ino);
        }
    }

    bofs_fs_flush(fs);
    return BOFS_SEC_OK;
}

int bofs_sec_rmdir(bofs_file_system_t* fs, uint64_t parent_ino, const char* name,
                   const bofs_cred_t* cred) {
    if (!fs || !cred || !name) return BOFS_ERR_SEC_INVALID_PARAM;

    /* 1. Authorize on parent directory: requires WRITE | EXECUTE */
    bofs_inode_t parent_inode;
    int r = bofs_inode_read(fs, parent_ino, &parent_inode);
    if (r != BOFS_FILE_OK) return r;

    int perm = bofs_check_dir_modify(&parent_inode, cred);
    if (perm != BOFS_SEC_OK) return perm;

    /* 2. Delegate to bofs_rmdir (validates directory type, non-empty, not root) */
    r = bofs_rmdir(fs, parent_ino, name);
    if (r == BOFS_DIR_OK) {
        bofs_fs_flush(fs);
        return BOFS_SEC_OK;
    }
    if (r == BOFS_ERR_DIR_NOT_EMPTY) return BOFS_ERR_SEC_NOT_EMPTY;
    if (r == BOFS_ERR_DIR_NOT_A_DIR) return BOFS_ERR_SEC_NOT_DIR;
    if (r == BOFS_ERR_DIR_NOT_FOUND) return BOFS_ERR_SEC_NOT_FOUND;
    return r;
}

int bofs_sec_rename(bofs_file_system_t* fs, uint64_t old_parent, const char* old_name,
                    uint64_t new_parent, const char* new_name, const bofs_cred_t* cred) {
    if (!fs || !cred || !old_name || !new_name) return BOFS_ERR_SEC_INVALID_PARAM;

    /* 1. Authorize on old parent directory */
    bofs_inode_t old_p;
    int r = bofs_inode_read(fs, old_parent, &old_p);
    if (r != BOFS_FILE_OK) return r;
    int perm = bofs_check_dir_modify(&old_p, cred);
    if (perm != BOFS_SEC_OK) return perm;

    /* 2. Authorize on new parent directory */
    bofs_inode_t new_p;
    if (new_parent == old_parent) {
        new_p = old_p;
    } else {
        r = bofs_inode_read(fs, new_parent, &new_p);
        if (r != BOFS_FILE_OK) return r;
        perm = bofs_check_dir_modify(&new_p, cred);
        if (perm != BOFS_SEC_OK) return perm;
    }

    /* 3. Execute rename */
    r = bofs_rename(fs, old_parent, old_name, new_parent, new_name);
    if (r == BOFS_DIR_OK) {
        bofs_fs_flush(fs);
        return BOFS_SEC_OK;
    }
    return r;
}

int bofs_sec_lookup(bofs_file_system_t* fs, uint64_t dir_ino, const char* name,
                    const bofs_cred_t* cred, uint64_t* out_ino, uint8_t* out_type) {
    if (!fs || !cred || !name) return BOFS_ERR_SEC_INVALID_PARAM;

    /* Search/lookup requires EXECUTE on directory */
    bofs_inode_t dir_inode;
    int r = bofs_inode_read(fs, dir_ino, &dir_inode);
    if (r != BOFS_FILE_OK) return r;

    int perm = bofs_check_dir_search(&dir_inode, cred);
    if (perm != BOFS_SEC_OK) return perm;

    return bofs_dir_lookup(fs, dir_ino, name, out_ino, NULL, out_type);
}

int bofs_sec_readdir(bofs_file_system_t* fs, uint64_t dir_ino, const bofs_cred_t* cred,
                     uint32_t offset_cookie, bofs_dirent_t* out_dirents,
                     uint32_t max_count, uint32_t* out_count) {
    if (!fs || !cred || !out_dirents || !out_count) return BOFS_ERR_SEC_INVALID_PARAM;

    /* Listing/enumeration requires READ on directory */
    bofs_inode_t dir_inode;
    int r = bofs_inode_read(fs, dir_ino, &dir_inode);
    if (r != BOFS_FILE_OK) return r;

    int perm = bofs_check_dir_read(&dir_inode, cred);
    if (perm != BOFS_SEC_OK) return perm;

    return bofs_readdir(fs, dir_ino, offset_cookie, out_dirents, max_count, out_count);
}

int bofs_sec_path_resolve(bofs_file_system_t* fs, uint64_t root_ino, uint64_t cwd_ino,
                          const char* path, const bofs_cred_t* cred,
                          uint64_t* out_ino, uint8_t* out_type) {
    if (!fs || !cred || !path) return BOFS_ERR_SEC_INVALID_PARAM;
    if (path[0] == '\0') return BOFS_ERR_DIR_INVALID_NAME;

    uint64_t cur_ino = (path[0] == '/') ? root_ino : cwd_ino;
    const char* p = path;
    while (*p == '/') p++;

    if (*p == '\0') {
        if (out_ino) *out_ino = cur_ino;
        if (out_type) *out_type = BOFS_FT_DIR;
        return BOFS_SEC_OK;
    }

    uint8_t cur_type = BOFS_FT_DIR;
    char token[49];

    while (*p != '\0') {
        size_t tlen = 0;
        while (*p != '/' && *p != '\0') {
            if (tlen < 48) token[tlen++] = *p;
            p++;
        }
        token[tlen] = '\0';
        while (*p == '/') p++;

        /* 1. Authorize traversal on current directory: requires EXECUTE */
        bofs_inode_t cur_inode;
        int r = bofs_inode_read(fs, cur_ino, &cur_inode);
        if (r != BOFS_FILE_OK) return r;

        int perm = bofs_check_dir_search(&cur_inode, cred);
        if (perm != BOFS_SEC_OK) return perm;

        /* 2. Handle '.' and '..' */
        if (strcmp(token, ".") == 0) {
            continue;
        }
        if (strcmp(token, "..") == 0) {
            if (cur_ino == root_ino) {
                /* Root escape clamped */
                continue;
            }
            uint64_t parent_ino = 0;
            r = bofs_dir_lookup(fs, cur_ino, "..", &parent_ino, NULL, NULL);
            if (r != BOFS_DIR_OK) return r;
            cur_ino = parent_ino;
            cur_type = BOFS_FT_DIR;
            continue;
        }

        /* 3. Lookup component */
        uint64_t next_ino = 0;
        uint8_t next_type = 0;
        r = bofs_dir_lookup(fs, cur_ino, token, &next_ino, NULL, &next_type);
        if (r != BOFS_DIR_OK) return r;

        cur_ino = next_ino;
        cur_type = next_type;

        /* If there are more components to follow, next must be a directory */
        if (*p != '\0' && cur_type != BOFS_FT_DIR) {
            return BOFS_ERR_SEC_NOT_DIR;
        }
    }

    if (out_ino) *out_ino = cur_ino;
    if (out_type) *out_type = cur_type;
    return BOFS_SEC_OK;
}
