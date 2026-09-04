#include "kernel/vfs/bofs/include/bofs_dir.h"
#include "kernel/core/lib/include/string.h"

/* --------------------------------------------------------------------------
 * Internal Helper: Block Device I/O with Sector Translation
 * -------------------------------------------------------------------------- */
static inline bool bofs_dir_block_to_lba(const bofs_file_system_t* fs, uint64_t block_idx,
                                         uint64_t* out_lba, uint32_t* out_sec_count) {
    if (!fs || !fs->dev || fs->sb.sector_size == 0) return false;
    uint32_t spb = BOFS_BLOCK_SIZE / (uint32_t)fs->sb.sector_size;
    if (spb == 0) return false;
    if (block_idx > (UINT64_MAX / spb)) return false;

    *out_lba = block_idx * spb;
    *out_sec_count = spb;
    return true;
}

int bofs_dir_read_node(bofs_file_system_t* fs, uint64_t block_idx, bofs_dir_node_t* out_node) {
    if (!fs || !fs->dev || !out_node) return BOFS_ERR_DIR_INVALID_PARAM;
    if (block_idx >= fs->sb.total_blocks) return BOFS_ERR_DIR_CORRUPT_NODE;

    uint64_t lba;
    uint32_t count;
    if (!bofs_dir_block_to_lba(fs, block_idx, &lba, &count)) return BOFS_ERR_DIR_CORRUPT_NODE;

    if (!fs->dev->read(fs->dev, lba, count, out_node)) {
        return BOFS_ERR_DIR_IO;
    }

    int val_res = bofs_validate_dir_node(out_node);
    if (val_res != BOFS_VALID_OK) {
        return BOFS_ERR_DIR_CORRUPT_NODE;
    }

    return BOFS_DIR_OK;
}

static inline int bofs_dir_read_block(bofs_file_system_t* fs, uint64_t block_idx, bofs_dir_node_t* out_node) {
    return bofs_dir_read_node(fs, block_idx, out_node);
}

static int bofs_dir_write_block(bofs_file_system_t* fs, uint64_t block_idx, bofs_dir_node_t* node) {
    if (!fs || !fs->dev || !node) return BOFS_ERR_DIR_INVALID_PARAM;
    if (block_idx >= fs->sb.total_blocks) return BOFS_ERR_DIR_CORRUPT_NODE;

    /* Recompute IEEE 802.3 CRC32 checksum before disk serialization */
    node->checksum = bofs_crc32(node, offsetof(bofs_dir_node_t, checksum));

    uint64_t lba;
    uint32_t count;
    if (!bofs_dir_block_to_lba(fs, block_idx, &lba, &count)) return BOFS_ERR_DIR_CORRUPT_NODE;

    if (!fs->dev->write(fs->dev, lba, count, node)) {
        return BOFS_ERR_DIR_IO;
    }

    return BOFS_DIR_OK;
}

/* --------------------------------------------------------------------------
 * Public API: UTF-8 Filename Validation
 * RFC 3629 Strict Canonical Validation
 * -------------------------------------------------------------------------- */
int bofs_dir_validate_name(const char* name, size_t* out_len) {
    if (!name) return BOFS_ERR_DIR_INVALID_PARAM;
    if (name[0] == '\0') return BOFS_ERR_DIR_INVALID_NAME;

    size_t len = 0;
    const uint8_t* p = (const uint8_t*)name;

    while (*p != '\0') {
        if (len >= BOFS_MAX_FILENAME_LEN) {
            return BOFS_ERR_DIR_NAME_TOO_LONG;
        }

        uint8_t c = *p;
        /* Prohibit '/' (path separator) and NUL */
        if (c == '/' || c == '\0') {
            return BOFS_ERR_DIR_INVALID_NAME;
        }

        if (c <= 0x7F) {
            /* 1-byte ASCII */
            p++;
            len++;
        } else if ((c & 0xE0) == 0xC0) {
            /* 2-byte UTF-8: 110xxxxx 10xxxxxx */
            if (c < 0xC2) return BOFS_ERR_DIR_INVALID_NAME; /* Overlong encoding */
            p++;
            len++;
            if ((*p & 0xC0) != 0x80) return BOFS_ERR_DIR_INVALID_NAME;
            p++;
            len++;
        } else if ((c & 0xF0) == 0xE0) {
            /* 3-byte UTF-8: 1110xxxx 10xxxxxx 10xxxxxx */
            uint8_t c1 = c;
            p++;
            len++;
            uint8_t c2 = *p;
            if (c1 == 0xE0 && c2 < 0xA0) return BOFS_ERR_DIR_INVALID_NAME; /* Overlong */
            if (c1 == 0xED && c2 >= 0xA0) return BOFS_ERR_DIR_INVALID_NAME; /* Surrogate halves */
            if ((c2 & 0xC0) != 0x80) return BOFS_ERR_DIR_INVALID_NAME;
            p++;
            len++;
            if ((*p & 0xC0) != 0x80) return BOFS_ERR_DIR_INVALID_NAME;
            p++;
            len++;
        } else if ((c & 0xF8) == 0xF0) {
            /* 4-byte UTF-8: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
            uint8_t c1 = c;
            p++;
            len++;
            uint8_t c2 = *p;
            if (c1 == 0xF0 && c2 < 0x90) return BOFS_ERR_DIR_INVALID_NAME; /* Overlong */
            if (c1 == 0xF4 && c2 > 0x8F) return BOFS_ERR_DIR_INVALID_NAME; /* Beyond U+10FFFF */
            if (c1 > 0xF4) return BOFS_ERR_DIR_INVALID_NAME;
            if ((c2 & 0xC0) != 0x80) return BOFS_ERR_DIR_INVALID_NAME;
            p++;
            len++;
            if ((*p & 0xC0) != 0x80) return BOFS_ERR_DIR_INVALID_NAME;
            p++;
            len++;
            if ((*p & 0xC0) != 0x80) return BOFS_ERR_DIR_INVALID_NAME;
            p++;
            len++;
        } else {
            /* Invalid leading byte */
            return BOFS_ERR_DIR_INVALID_NAME;
        }
    }

    if (out_len) *out_len = len;
    return BOFS_DIR_OK;
}

/* --------------------------------------------------------------------------
 * Public API: Deterministic Name Comparator
 * Primary key: 64-bit FNV-1a Hash
 * Secondary key: Canonical UTF-8 Byte Comparison (strcmp)
 * -------------------------------------------------------------------------- */
int bofs_dir_cmp(uint64_t hash_a, const char* name_a, uint64_t hash_b, const char* name_b) {
    if (hash_a < hash_b) return -1;
    if (hash_a > hash_b) return 1;
    if (!name_a && !name_b) return 0;
    if (!name_a) return -1;
    if (!name_b) return 1;
    return strcmp(name_a, name_b);
}

/* --------------------------------------------------------------------------
 * Internal Helper: Load Directory Root Block from Inode
 * -------------------------------------------------------------------------- */
static int bofs_dir_get_root_block(bofs_file_system_t* fs, uint64_t dir_ino,
                                   bofs_inode_t* out_inode, uint64_t* out_root_block) {
    if (!fs || !out_inode || !out_root_block) return BOFS_ERR_DIR_INVALID_PARAM;

    int read_res = bofs_inode_read(fs, dir_ino, out_inode);
    if (read_res != BOFS_FILE_OK) return BOFS_ERR_DIR_NOT_FOUND;

    if ((out_inode->mode & BOFS_S_IFMT) != BOFS_S_IFDIR) {
        return BOFS_ERR_DIR_NOT_A_DIR;
    }

    if (out_inode->allocated_blocks == 0 ||
        !(out_inode->direct_extents[0].flags & BOFS_EXTENT_FLAG_VALID)) {
        return BOFS_ERR_DIR_CORRUPT_METADATA;
    }

    *out_root_block = out_inode->direct_extents[0].physical_block;
    return BOFS_DIR_OK;
}

/* --------------------------------------------------------------------------
 * Internal Helper: Traverse B+Tree from Root to Target Leaf Node
 * -------------------------------------------------------------------------- */
#define BOFS_BTREE_MAX_DEPTH 8

typedef struct {
    uint64_t path_blocks[BOFS_BTREE_MAX_DEPTH];
    uint16_t path_indices[BOFS_BTREE_MAX_DEPTH];
    uint32_t depth;
} bofs_btree_path_t;

static int bofs_dir_find_leaf(bofs_file_system_t* fs, uint64_t root_block,
                              uint64_t hash, const char* name,
                              bofs_dir_node_t* out_leaf, uint64_t* out_leaf_block,
                              bofs_btree_path_t* out_path) {
    uint64_t cur_block = root_block;
    bofs_dir_node_t cur_node;
    uint32_t depth = 0;

    while (depth < BOFS_BTREE_MAX_DEPTH) {
        int r_res = bofs_dir_read_block(fs, cur_block, &cur_node);
        if (r_res != BOFS_DIR_OK) return r_res;

        if (cur_node.node_type == BOFS_DIR_NODE_LEAF) {
            if (out_leaf) *out_leaf = cur_node;
            if (out_leaf_block) *out_leaf_block = cur_block;
            if (out_path) out_path->depth = depth;
            return BOFS_DIR_OK;
        }

        if (cur_node.node_type != BOFS_DIR_NODE_ROUTER || cur_node.entry_count == 0) {
            return BOFS_ERR_DIR_CORRUPT_NODE;
        }

        /* Search router slots for target branch */
        uint16_t chosen_idx = cur_node.entry_count - 1;
        for (uint16_t i = 0; i < cur_node.entry_count; i++) {
            int cmp = bofs_dir_cmp(hash, name, cur_node.slots[i].hash, cur_node.slots[i].name);
            if (cmp <= 0) {
                chosen_idx = i;
                break;
            }
        }

        if (out_path) {
            out_path->path_blocks[depth] = cur_block;
            out_path->path_indices[depth] = chosen_idx;
        }

        cur_block = cur_node.slots[chosen_idx].child_block;
        depth++;
    }

    return BOFS_ERR_DIR_CYCLE_DETECTED;
}

/* --------------------------------------------------------------------------
 * Public API: bofs_dir_lookup
 * -------------------------------------------------------------------------- */
int bofs_dir_lookup(bofs_file_system_t* fs, uint64_t dir_ino, const char* name,
                    uint64_t* out_child_ino, uint32_t* out_child_gen, uint8_t* out_type) {
    if (!fs || !name) return BOFS_ERR_DIR_INVALID_PARAM;

    /* Handle '.' (self) */
    if (strcmp(name, ".") == 0) {
        if (out_child_ino) *out_child_ino = dir_ino;
        if (out_type) *out_type = BOFS_FT_DIR;
        if (out_child_gen) {
            bofs_inode_t ino;
            if (bofs_inode_read(fs, dir_ino, &ino) == BOFS_FILE_OK) {
                *out_child_gen = ino.generation;
            } else {
                *out_child_gen = 1;
            }
        }
        return BOFS_DIR_OK;
    }

    /* Handle '..' at root directory (clamps to root) */
    if (dir_ino == BOFS_ROOT_INODE && strcmp(name, "..") == 0) {
        if (out_child_ino) *out_child_ino = BOFS_ROOT_INODE;
        if (out_type) *out_type = BOFS_FT_DIR;
        if (out_child_gen) *out_child_gen = 1;
        return BOFS_DIR_OK;
    }

    size_t name_len = 0;
    int v_res = bofs_dir_validate_name(name, &name_len);
    if (v_res != BOFS_DIR_OK) return v_res;

    bofs_inode_t dir_inode;
    uint64_t root_block = 0;
    int r_res = bofs_dir_get_root_block(fs, dir_ino, &dir_inode, &root_block);
    if (r_res != BOFS_DIR_OK) return r_res;

    uint64_t hash = bofs_hash(name, (uint8_t)name_len);

    bofs_dir_node_t leaf;
    uint64_t leaf_block = 0;
    int find_res = bofs_dir_find_leaf(fs, root_block, hash, name, &leaf, &leaf_block, NULL);
    if (find_res != BOFS_DIR_OK) return find_res;

    /* Search inside target leaf node */
    for (uint16_t i = 0; i < leaf.entry_count; i++) {
        if (leaf.slots[i].hash == hash && strcmp(leaf.slots[i].name, name) == 0) {
            if (out_child_ino) *out_child_ino = leaf.slots[i].inode_num;
            if (out_type) *out_type = leaf.slots[i].file_type;
            if (out_child_gen) {
                bofs_inode_t child_ino;
                if (bofs_inode_read(fs, leaf.slots[i].inode_num, &child_ino) == BOFS_FILE_OK) {
                    *out_child_gen = child_ino.generation;
                } else {
                    *out_child_gen = 1;
                }
            }
            return BOFS_DIR_OK;
        }
    }

    return BOFS_ERR_DIR_NOT_FOUND;
}

/* --------------------------------------------------------------------------
 * Internal Helper: Split B+Tree Leaf Node
 * -------------------------------------------------------------------------- */
static int bofs_dir_split_leaf(bofs_file_system_t* fs, uint64_t dir_ino,
                               bofs_btree_path_t* path, uint64_t leaf_block,
                               bofs_dir_node_t* leaf, uint64_t new_hash,
                               const char* new_name, uint64_t new_ino,
                               uint8_t new_type, size_t new_name_len) {
    if (path->depth == 0) {
        /* ------------------------------------------------------------------
         * ROOT SPLIT: Root is currently a leaf at logical block 0.
         * Allocate two new blocks (left_blk and right_blk).
         * Distribute entries evenly, then turn root into a ROUTER node.
         * ------------------------------------------------------------------ */
        uint64_t left_blk = 0;
        uint64_t right_blk = 0;
        int a_res = bofs_alloc_block(fs->alloc, &left_blk);
        if (a_res != BOFS_ALLOC_OK) return BOFS_ERR_DIR_NO_SPACE;

        a_res = bofs_alloc_block(fs->alloc, &right_blk);
        if (a_res != BOFS_ALLOC_OK) {
            bofs_free_blocks(fs->alloc, left_blk, 1);
            return BOFS_ERR_DIR_NO_SPACE;
        }

        bofs_dir_node_t left_node;
        bofs_dir_node_t right_node;
        bofs_init_dir_node(&left_node, BOFS_DIR_NODE_LEAF, 0);
        bofs_init_dir_node(&right_node, BOFS_DIR_NODE_LEAF, 0);

        left_node.parent_block = leaf_block;
        right_node.parent_block = leaf_block;
        left_node.left_sibling_block = 0;
        left_node.right_sibling_block = right_blk;
        right_node.left_sibling_block = left_blk;
        right_node.right_sibling_block = 0;

        uint16_t mid = leaf->entry_count / 2; /* 31 */

        /* Copy lower half to left_node */
        for (uint16_t i = 0; i < mid; i++) {
            left_node.slots[i] = leaf->slots[i];
        }
        left_node.entry_count = mid;

        /* Copy upper half to right_node */
        for (uint16_t i = mid; i < leaf->entry_count; i++) {
            right_node.slots[i - mid] = leaf->slots[i];
        }
        right_node.entry_count = leaf->entry_count - mid;

        /* Insert new entry into appropriate child node */
        int cmp_mid = bofs_dir_cmp(new_hash, new_name, right_node.slots[0].hash, right_node.slots[0].name);
        bofs_dir_node_t* target = (cmp_mid < 0) ? &left_node : &right_node;

        uint16_t ins_pos = target->entry_count;
        for (uint16_t i = 0; i < target->entry_count; i++) {
            if (bofs_dir_cmp(new_hash, new_name, target->slots[i].hash, target->slots[i].name) < 0) {
                ins_pos = i;
                break;
            }
        }
        for (uint16_t i = target->entry_count; i > ins_pos; i--) {
            target->slots[i] = target->slots[i - 1];
        }
        target->slots[ins_pos].hash = new_hash;
        target->slots[ins_pos].inode_num = new_ino;
        target->slots[ins_pos].child_block = 0;
        target->slots[ins_pos].rec_len = 64;
        target->slots[ins_pos].name_len = (uint8_t)new_name_len;
        target->slots[ins_pos].file_type = new_type;
        strncpy(target->slots[ins_pos].name, new_name, 35);
        target->slots[ins_pos].name[35] = '\0';
        target->entry_count++;

        int w_res = bofs_dir_write_block(fs, left_blk, &left_node);
        if (w_res != BOFS_DIR_OK) return w_res;
        w_res = bofs_dir_write_block(fs, right_blk, &right_node);
        if (w_res != BOFS_DIR_OK) return w_res;

        /* Convert root block to ROUTER node */
        bofs_init_dir_node(leaf, BOFS_DIR_NODE_ROUTER, 1);
        leaf->parent_block = 0;
        leaf->entry_count = 2;

        leaf->slots[0].hash = left_node.slots[left_node.entry_count - 1].hash;
        leaf->slots[0].child_block = left_blk;
        leaf->slots[0].rec_len = 64;
        leaf->slots[0].name_len = left_node.slots[left_node.entry_count - 1].name_len;
        strncpy(leaf->slots[0].name, left_node.slots[left_node.entry_count - 1].name, 35);

        leaf->slots[1].hash = UINT64_MAX;
        leaf->slots[1].child_block = right_blk;
        leaf->slots[1].rec_len = 64;
        leaf->slots[1].name_len = right_node.slots[right_node.entry_count - 1].name_len;
        strncpy(leaf->slots[1].name, right_node.slots[right_node.entry_count - 1].name, 35);

        w_res = bofs_dir_write_block(fs, leaf_block, leaf);
        if (w_res != BOFS_DIR_OK) return w_res;

        /* Update directory inode block metrics */
        bofs_inode_t ino;
        if (bofs_inode_read(fs, dir_ino, &ino) == BOFS_FILE_OK) {
            ino.allocated_blocks += 2;
            ino.size_bytes += 2 * BOFS_BLOCK_SIZE;
            bofs_inode_write(fs, &ino);
        }

        return BOFS_DIR_OK;
    } else {
        /* ------------------------------------------------------------------
         * INTERNAL LEAF SPLIT: Node already has a parent router node.
         * Allocate 1 new block (right_blk).
         * Move upper half to right_blk, update sibling links, and insert
         * new separator into parent router.
         * ------------------------------------------------------------------ */
        uint64_t right_blk = 0;
        int a_res = bofs_alloc_block(fs->alloc, &right_blk);
        if (a_res != BOFS_ALLOC_OK) return BOFS_ERR_DIR_NO_SPACE;

        bofs_dir_node_t right_node;
        bofs_init_dir_node(&right_node, BOFS_DIR_NODE_LEAF, 0);

        uint64_t parent_blk = path->path_blocks[path->depth - 1];
        right_node.parent_block = parent_blk;
        right_node.left_sibling_block = leaf_block;
        right_node.right_sibling_block = leaf->right_sibling_block;

        uint16_t mid = leaf->entry_count / 2; /* 31 */

        /* Copy upper half to right_node */
        for (uint16_t i = mid; i < leaf->entry_count; i++) {
            right_node.slots[i - mid] = leaf->slots[i];
        }
        right_node.entry_count = leaf->entry_count - mid;
        leaf->entry_count = mid;

        /* If previous right sibling existed, update its left pointer */
        if (leaf->right_sibling_block != 0) {
            bofs_dir_node_t old_right_sib;
            if (bofs_dir_read_block(fs, leaf->right_sibling_block, &old_right_sib) == BOFS_DIR_OK) {
                old_right_sib.left_sibling_block = right_blk;
                bofs_dir_write_block(fs, leaf->right_sibling_block, &old_right_sib);
            }
        }
        leaf->right_sibling_block = right_blk;

        /* Insert new entry into appropriate leaf */
        int cmp_mid = bofs_dir_cmp(new_hash, new_name, right_node.slots[0].hash, right_node.slots[0].name);
        bofs_dir_node_t* target = (cmp_mid < 0) ? leaf : &right_node;

        uint16_t ins_pos = target->entry_count;
        for (uint16_t i = 0; i < target->entry_count; i++) {
            if (bofs_dir_cmp(new_hash, new_name, target->slots[i].hash, target->slots[i].name) < 0) {
                ins_pos = i;
                break;
            }
        }
        for (uint16_t i = target->entry_count; i > ins_pos; i--) {
            target->slots[i] = target->slots[i - 1];
        }
        target->slots[ins_pos].hash = new_hash;
        target->slots[ins_pos].inode_num = new_ino;
        target->slots[ins_pos].child_block = 0;
        target->slots[ins_pos].rec_len = 64;
        target->slots[ins_pos].name_len = (uint8_t)new_name_len;
        target->slots[ins_pos].file_type = new_type;
        strncpy(target->slots[ins_pos].name, new_name, 35);
        target->slots[ins_pos].name[35] = '\0';
        target->entry_count++;

        bofs_dir_write_block(fs, leaf_block, leaf);
        bofs_dir_write_block(fs, right_blk, &right_node);

        /* Update parent router node */
        bofs_dir_node_t parent_node;
        int p_res = bofs_dir_read_block(fs, parent_blk, &parent_node);
        if (p_res != BOFS_DIR_OK) return p_res;

        uint16_t p_idx = path->path_indices[path->depth - 1];
        /* Update left slot pivot */
        parent_node.slots[p_idx].hash = leaf->slots[leaf->entry_count - 1].hash;
        strncpy(parent_node.slots[p_idx].name, leaf->slots[leaf->entry_count - 1].name, 35);

        if (parent_node.entry_count < BOFS_DIR_SLOTS_PER_BLOCK) {
            /* Make room in parent router slots right after p_idx */
            for (uint16_t i = parent_node.entry_count; i > p_idx + 1; i--) {
                parent_node.slots[i] = parent_node.slots[i - 1];
            }
            parent_node.slots[p_idx + 1].hash = (p_idx + 1 == parent_node.entry_count) ? UINT64_MAX : right_node.slots[right_node.entry_count - 1].hash;
            parent_node.slots[p_idx + 1].child_block = right_blk;
            parent_node.slots[p_idx + 1].rec_len = 64;
            parent_node.slots[p_idx + 1].name_len = right_node.slots[right_node.entry_count - 1].name_len;
            strncpy(parent_node.slots[p_idx + 1].name, right_node.slots[right_node.entry_count - 1].name, 35);
            parent_node.entry_count++;
            bofs_dir_write_block(fs, parent_blk, &parent_node);
        }

        /* Update directory inode block metrics */
        bofs_inode_t ino;
        if (bofs_inode_read(fs, dir_ino, &ino) == BOFS_FILE_OK) {
            ino.allocated_blocks += 1;
            ino.size_bytes += BOFS_BLOCK_SIZE;
            bofs_inode_write(fs, &ino);
        }

        return BOFS_DIR_OK;
    }
}

/* --------------------------------------------------------------------------
 * Public API: bofs_dir_insert
 * -------------------------------------------------------------------------- */
int bofs_dir_insert(bofs_file_system_t* fs, uint64_t dir_ino, const char* name,
                    uint64_t child_ino, uint32_t child_gen, uint8_t type) {
    (void)child_gen;
    if (!fs || !name) return BOFS_ERR_DIR_INVALID_PARAM;

    size_t name_len = 0;
    int v_res = bofs_dir_validate_name(name, &name_len);
    if (v_res != BOFS_DIR_OK) return v_res;

    /* Check for duplicate name */
    uint64_t dummy_ino;
    if (bofs_dir_lookup(fs, dir_ino, name, &dummy_ino, NULL, NULL) == BOFS_DIR_OK) {
        return BOFS_ERR_DIR_ENTRY_EXISTS;
    }

    bofs_inode_t dir_inode;
    uint64_t root_block = 0;
    int r_res = bofs_dir_get_root_block(fs, dir_ino, &dir_inode, &root_block);
    if (r_res != BOFS_DIR_OK) return r_res;

    uint64_t hash = bofs_hash(name, (uint8_t)name_len);

    bofs_dir_node_t leaf;
    uint64_t leaf_block = 0;
    bofs_btree_path_t path;
    int find_res = bofs_dir_find_leaf(fs, root_block, hash, name, &leaf, &leaf_block, &path);
    if (find_res != BOFS_DIR_OK) return find_res;

    if (leaf.entry_count < BOFS_DIR_SLOTS_PER_BLOCK) {
        /* Find sorted position */
        uint16_t ins_pos = leaf.entry_count;
        for (uint16_t i = 0; i < leaf.entry_count; i++) {
            if (bofs_dir_cmp(hash, name, leaf.slots[i].hash, leaf.slots[i].name) < 0) {
                ins_pos = i;
                break;
            }
        }

        /* Shift entries */
        for (uint16_t i = leaf.entry_count; i > ins_pos; i--) {
            leaf.slots[i] = leaf.slots[i - 1];
        }

        leaf.slots[ins_pos].hash = hash;
        leaf.slots[ins_pos].inode_num = child_ino;
        leaf.slots[ins_pos].child_block = 0;
        leaf.slots[ins_pos].rec_len = 64;
        leaf.slots[ins_pos].name_len = (uint8_t)name_len;
        leaf.slots[ins_pos].file_type = type;
        strncpy(leaf.slots[ins_pos].name, name, 35);
        leaf.slots[ins_pos].name[35] = '\0';

        leaf.entry_count++;
        leaf.generation++;

        int w_res = bofs_dir_write_block(fs, leaf_block, &leaf);
        if (w_res != BOFS_DIR_OK) return w_res;

        return BOFS_DIR_OK;
    }

    /* Node full: execute deterministic B+Tree split */
    return bofs_dir_split_leaf(fs, dir_ino, &path, leaf_block, &leaf,
                               hash, name, child_ino, type, name_len);
}

/* --------------------------------------------------------------------------
 * Public API: bofs_dir_remove
 * -------------------------------------------------------------------------- */
int bofs_dir_remove(bofs_file_system_t* fs, uint64_t dir_ino, const char* name) {
    if (!fs || !name) return BOFS_ERR_DIR_INVALID_PARAM;

    size_t name_len = 0;
    int v_res = bofs_dir_validate_name(name, &name_len);
    if (v_res != BOFS_DIR_OK) return v_res;

    bofs_inode_t dir_inode;
    uint64_t root_block = 0;
    int r_res = bofs_dir_get_root_block(fs, dir_ino, &dir_inode, &root_block);
    if (r_res != BOFS_DIR_OK) return r_res;

    uint64_t hash = bofs_hash(name, (uint8_t)name_len);

    bofs_dir_node_t leaf;
    uint64_t leaf_block = 0;
    int find_res = bofs_dir_find_leaf(fs, root_block, hash, name, &leaf, &leaf_block, NULL);
    if (find_res != BOFS_DIR_OK) return find_res;

    int match_idx = -1;
    for (uint16_t i = 0; i < leaf.entry_count; i++) {
        if (leaf.slots[i].hash == hash && strcmp(leaf.slots[i].name, name) == 0) {
            match_idx = (int)i;
            break;
        }
    }

    if (match_idx < 0) return BOFS_ERR_DIR_NOT_FOUND;

    /* Shift entries left */
    for (uint16_t i = (uint16_t)match_idx; i < leaf.entry_count - 1; i++) {
        leaf.slots[i] = leaf.slots[i + 1];
    }
    memset(&leaf.slots[leaf.entry_count - 1], 0, sizeof(bofs_dir_entry_slot_t));
    leaf.entry_count--;
    leaf.generation++;

    return bofs_dir_write_block(fs, leaf_block, &leaf);
}

/* --------------------------------------------------------------------------
 * Public API: bofs_mkdir
 * -------------------------------------------------------------------------- */
int bofs_mkdir(bofs_file_system_t* fs, uint64_t parent_ino, const char* name,
               uint16_t mode, uint64_t* out_ino) {
    if (!fs || !name) return BOFS_ERR_DIR_INVALID_PARAM;
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        return BOFS_ERR_DIR_INVALID_NAME;
    }

    size_t name_len = 0;
    int v_res = bofs_dir_validate_name(name, &name_len);
    if (v_res != BOFS_DIR_OK) return v_res;

    /* Verify parent inode exists and is a directory */
    bofs_inode_t parent_inode;
    int p_res = bofs_inode_read(fs, parent_ino, &parent_inode);
    if (p_res != BOFS_FILE_OK) return BOFS_ERR_DIR_NOT_FOUND;
    if ((parent_inode.mode & BOFS_S_IFMT) != BOFS_S_IFDIR) {
        return BOFS_ERR_DIR_NOT_A_DIR;
    }

    /* Ensure name does not already exist */
    uint64_t existing_ino;
    if (bofs_dir_lookup(fs, parent_ino, name, &existing_ino, NULL, NULL) == BOFS_DIR_OK) {
        return BOFS_ERR_DIR_ENTRY_EXISTS;
    }

    /* 1. Allocate child inode */
    uint64_t child_ino = 0;
    int a_res = bofs_inode_alloc(fs, &child_ino);
    if (a_res != BOFS_FILE_OK) return a_res;

    /* 2. Allocate 1 data block for child root B+Tree */
    uint64_t root_blk = 0;
    a_res = bofs_alloc_block(fs->alloc, &root_blk);
    if (a_res != BOFS_ALLOC_OK) {
        bofs_inode_free(fs, child_ino);
        return BOFS_ERR_DIR_NO_SPACE;
    }

    /* 3. Initialize child root directory block */
    bofs_dir_node_t child_root;
    bofs_init_dir_node(&child_root, BOFS_DIR_NODE_LEAF, 0);

    /* Slot 0: '.' -> child_ino */
    child_root.slots[0].hash = bofs_hash(".", 1);
    child_root.slots[0].inode_num = child_ino;
    child_root.slots[0].rec_len = 64;
    child_root.slots[0].name_len = 1;
    child_root.slots[0].file_type = BOFS_FT_DIR;
    strcpy(child_root.slots[0].name, ".");

    /* Slot 1: '..' -> parent_ino */
    child_root.slots[1].hash = bofs_hash("..", 2);
    child_root.slots[1].inode_num = parent_ino;
    child_root.slots[1].rec_len = 64;
    child_root.slots[1].name_len = 2;
    child_root.slots[1].file_type = BOFS_FT_DIR;
    strcpy(child_root.slots[1].name, "..");

    child_root.entry_count = 2;
    int w_res = bofs_dir_write_block(fs, root_blk, &child_root);
    if (w_res != BOFS_DIR_OK) {
        bofs_free_blocks(fs->alloc, root_blk, 1);
        bofs_inode_free(fs, child_ino);
        return w_res;
    }

    /* 4. Initialize child Inode metadata */
    bofs_inode_t child_inode;
    memset(&child_inode, 0, sizeof(bofs_inode_t));
    child_inode.magic = BOFS_INODE_MAGIC;
    child_inode.generation = 1;
    child_inode.inode_num = child_ino;
    child_inode.mode = BOFS_S_IFDIR | (mode & 07777);
    child_inode.flags = 0;
    child_inode.uid = parent_inode.uid;
    child_inode.gid = parent_inode.gid;
    child_inode.link_count = 2; /* '.' and parent's reference */
    child_inode.size_bytes = BOFS_BLOCK_SIZE;
    child_inode.allocated_blocks = 1;

    child_inode.direct_extents[0].logical_block = 0;
    child_inode.direct_extents[0].physical_block = root_blk;
    child_inode.direct_extents[0].block_count = 1;
    child_inode.direct_extents[0].flags = BOFS_EXTENT_FLAG_VALID;

    w_res = bofs_inode_write(fs, &child_inode);
    if (w_res != BOFS_FILE_OK) {
        bofs_free_blocks(fs->alloc, root_blk, 1);
        bofs_inode_free(fs, child_ino);
        return w_res;
    }

    /* 5. Insert entry into parent directory */
    int ins_res = bofs_dir_insert(fs, parent_ino, name, child_ino, 1, BOFS_FT_DIR);
    if (ins_res != BOFS_DIR_OK) {
        bofs_free_blocks(fs->alloc, root_blk, 1);
        bofs_inode_free(fs, child_ino);
        return ins_res;
    }

    /* 6. Increment parent link count and update */
    parent_inode.link_count++;
    bofs_inode_write(fs, &parent_inode);
    bofs_fs_flush(fs);

    if (out_ino) *out_ino = child_ino;
    return BOFS_DIR_OK;
}

/* --------------------------------------------------------------------------
 * Public API: bofs_rmdir
 * -------------------------------------------------------------------------- */
int bofs_rmdir(bofs_file_system_t* fs, uint64_t parent_ino, const char* name) {
    if (!fs || !name) return BOFS_ERR_DIR_INVALID_PARAM;
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        return BOFS_ERR_DIR_INVALID_NAME;
    }

    uint64_t child_ino = 0;
    uint8_t child_type = 0;
    int look_res = bofs_dir_lookup(fs, parent_ino, name, &child_ino, NULL, &child_type);
    if (look_res != BOFS_DIR_OK) return look_res;

    if (child_type != BOFS_FT_DIR) {
        return BOFS_ERR_DIR_NOT_A_DIR;
    }

    if (child_ino == BOFS_ROOT_INODE) {
        return BOFS_ERR_DIR_INVALID_PARAM;
    }

    /* Verify child is empty (contains only '.' and '..') */
    bofs_inode_t child_inode;
    uint64_t child_root_blk = 0;
    int r_res = bofs_dir_get_root_block(fs, child_ino, &child_inode, &child_root_blk);
    if (r_res != BOFS_DIR_OK) return r_res;

    bofs_dir_node_t child_root;
    r_res = bofs_dir_read_block(fs, child_root_blk, &child_root);
    if (r_res != BOFS_DIR_OK) return r_res;

    if (child_root.node_type != BOFS_DIR_NODE_LEAF) {
        return BOFS_ERR_DIR_NOT_EMPTY;
    }

    for (uint16_t i = 0; i < child_root.entry_count; i++) {
        if (strcmp(child_root.slots[i].name, ".") != 0 &&
            strcmp(child_root.slots[i].name, "..") != 0) {
            return BOFS_ERR_DIR_NOT_EMPTY;
        }
    }

    /* 1. Remove child entry from parent */
    int rem_res = bofs_dir_remove(fs, parent_ino, name);
    if (rem_res != BOFS_DIR_OK) return rem_res;

    /* 2. Free child directory block(s) and inode */
    bofs_free_blocks(fs->alloc, child_root_blk, 1);
    bofs_inode_free(fs, child_ino);

    /* 3. Decrement parent link_count */
    bofs_inode_t parent_inode;
    if (bofs_inode_read(fs, parent_ino, &parent_inode) == BOFS_FILE_OK) {
        if (parent_inode.link_count > 2) {
            parent_inode.link_count--;
            bofs_inode_write(fs, &parent_inode);
        }
    }

    bofs_fs_flush(fs);
    return BOFS_DIR_OK;
}

/* --------------------------------------------------------------------------
 * Public API: bofs_rename
 * -------------------------------------------------------------------------- */
int bofs_rename(bofs_file_system_t* fs, uint64_t old_parent_ino, const char* old_name,
                uint64_t new_parent_ino, const char* new_name) {
    if (!fs || !old_name || !new_name) return BOFS_ERR_DIR_INVALID_PARAM;
    if (strcmp(old_name, ".") == 0 || strcmp(old_name, "..") == 0 ||
        strcmp(new_name, ".") == 0 || strcmp(new_name, "..") == 0) {
        return BOFS_ERR_DIR_INVALID_NAME;
    }

    /* No-op rename */
    if (old_parent_ino == new_parent_ino && strcmp(old_name, new_name) == 0) {
        return BOFS_DIR_OK;
    }

    /* Lookup source */
    uint64_t child_ino = 0;
    uint32_t child_gen = 0;
    uint8_t child_type = 0;
    int look_res = bofs_dir_lookup(fs, old_parent_ino, old_name, &child_ino, &child_gen, &child_type);
    if (look_res != BOFS_DIR_OK) return look_res;

    /* Target name must not already exist in destination */
    uint64_t dest_ino;
    if (bofs_dir_lookup(fs, new_parent_ino, new_name, &dest_ino, NULL, NULL) == BOFS_DIR_OK) {
        return BOFS_ERR_DIR_ENTRY_EXISTS;
    }

    /* Cycle prevention for directory rename */
    if (child_type == BOFS_FT_DIR) {
        if (new_parent_ino == child_ino) {
            return BOFS_ERR_DIR_CYCLE_DETECTED;
        }

        /* Walk '..' up from new_parent_ino to root to ensure child_ino is not an ancestor */
        uint64_t walk_ino = new_parent_ino;
        uint32_t depth = 0;
        while (walk_ino != BOFS_ROOT_INODE && depth < BOFS_MAX_TRAVERSAL_DEPTH) {
            if (walk_ino == child_ino) {
                return BOFS_ERR_DIR_CYCLE_DETECTED;
            }
            uint64_t parent_of_walk = 0;
            if (bofs_dir_lookup(fs, walk_ino, "..", &parent_of_walk, NULL, NULL) != BOFS_DIR_OK) {
                break;
            }
            if (parent_of_walk == walk_ino) break; /* Reached root */
            walk_ino = parent_of_walk;
            depth++;
        }
        if (depth >= BOFS_MAX_TRAVERSAL_DEPTH) {
            return BOFS_ERR_DIR_CYCLE_DETECTED;
        }
    }

    /* Insert into new directory */
    int ins_res = bofs_dir_insert(fs, new_parent_ino, new_name, child_ino, child_gen, child_type);
    if (ins_res != BOFS_DIR_OK) return ins_res;

    /* If cross-directory move of a directory, update '..' in child */
    if (child_type == BOFS_FT_DIR && old_parent_ino != new_parent_ino) {
        bofs_inode_t c_inode;
        uint64_t c_root_blk = 0;
        if (bofs_dir_get_root_block(fs, child_ino, &c_inode, &c_root_blk) == BOFS_DIR_OK) {
            bofs_dir_node_t c_root;
            if (bofs_dir_read_block(fs, c_root_blk, &c_root) == BOFS_DIR_OK) {
                for (uint16_t i = 0; i < c_root.entry_count; i++) {
                    if (strcmp(c_root.slots[i].name, "..") == 0) {
                        c_root.slots[i].inode_num = new_parent_ino;
                        bofs_dir_write_block(fs, c_root_blk, &c_root);
                        break;
                    }
                }
            }
        }

        /* Adjust parent link counts */
        bofs_inode_t old_p, new_p;
        if (bofs_inode_read(fs, old_parent_ino, &old_p) == BOFS_FILE_OK && old_p.link_count > 2) {
            old_p.link_count--;
            bofs_inode_write(fs, &old_p);
        }
        if (bofs_inode_read(fs, new_parent_ino, &new_p) == BOFS_FILE_OK) {
            new_p.link_count++;
            bofs_inode_write(fs, &new_p);
        }
    }

    /* Remove from old parent */
    int rem_res = bofs_dir_remove(fs, old_parent_ino, old_name);
    bofs_fs_flush(fs);
    return rem_res;
}

/* --------------------------------------------------------------------------
 * Public API: bofs_readdir
 * -------------------------------------------------------------------------- */
int bofs_readdir(bofs_file_system_t* fs, uint64_t dir_ino, uint32_t start_index,
                 bofs_dirent_t* out_entries, uint32_t max_entries, uint32_t* out_count) {
    if (!fs || !out_entries || !out_count) return BOFS_ERR_DIR_INVALID_PARAM;
    *out_count = 0;

    bofs_inode_t dir_inode;
    uint64_t root_block = 0;
    int r_res = bofs_dir_get_root_block(fs, dir_ino, &dir_inode, &root_block);
    if (r_res != BOFS_DIR_OK) return r_res;

    /* Find leftmost leaf node */
    uint64_t cur_block = root_block;
    bofs_dir_node_t cur_node;
    uint32_t depth = 0;

    while (depth < BOFS_BTREE_MAX_DEPTH) {
        int rd_res = bofs_dir_read_block(fs, cur_block, &cur_node);
        if (rd_res != BOFS_DIR_OK) return rd_res;

        if (cur_node.node_type == BOFS_DIR_NODE_LEAF) break;
        if (cur_node.entry_count == 0) return BOFS_ERR_DIR_CORRUPT_NODE;

        cur_block = cur_node.slots[0].child_block;
        depth++;
    }

    uint32_t current_idx = 0;
    uint32_t filled = 0;
    uint32_t visited_leafs = 0;

    /* Enumerate by traversing leaf sibling pointers */
    while (cur_block != 0 && filled < max_entries && visited_leafs < 4096) {
        visited_leafs++;
        int rd_res = bofs_dir_read_block(fs, cur_block, &cur_node);
        if (rd_res != BOFS_DIR_OK) break;

        for (uint16_t i = 0; i < cur_node.entry_count; i++) {
            if (current_idx >= start_index) {
                if (filled >= max_entries) break;

                out_entries[filled].inode_num = cur_node.slots[i].inode_num;
                out_entries[filled].rec_len = sizeof(bofs_dirent_t);
                out_entries[filled].name_len = cur_node.slots[i].name_len;
                out_entries[filled].file_type = cur_node.slots[i].file_type;
                out_entries[filled].flags = 0;
                strncpy(out_entries[filled].name, cur_node.slots[i].name, 255);
                out_entries[filled].name[255] = '\0';
                filled++;
            }
            current_idx++;
        }

        cur_block = cur_node.right_sibling_block;
    }

    *out_count = filled;
    return BOFS_DIR_OK;
}

/* --------------------------------------------------------------------------
 * Public API: bofs_path_resolve
 * -------------------------------------------------------------------------- */
int bofs_path_resolve(bofs_file_system_t* fs, uint64_t root_ino, uint64_t cwd_ino,
                      const char* path, uint64_t* out_ino, uint32_t* out_gen, uint8_t* out_type) {
    if (!fs || !path) return BOFS_ERR_DIR_INVALID_PARAM;
    if (path[0] == '\0') return BOFS_ERR_DIR_INVALID_PARAM;

    size_t path_len = strlen(path);
    if (path_len > BOFS_MAX_PATH_LEN) return BOFS_ERR_DIR_NAME_TOO_LONG;

    uint64_t cur_ino = (path[0] == '/') ? root_ino : (cwd_ino ? cwd_ino : root_ino);
    uint32_t cur_gen = 1;
    uint8_t cur_type = BOFS_FT_DIR;

    const char* p = path;
    if (*p == '/') {
        while (*p == '/') p++; /* Skip leading slashes */
    }

    if (*p == '\0') {
        /* Path was strictly '/' */
        if (out_ino) *out_ino = cur_ino;
        if (out_gen) *out_gen = cur_gen;
        if (out_type) *out_type = cur_type;
        return BOFS_DIR_OK;
    }

    char token[256];
    uint32_t depth = 0;

    while (*p != '\0') {
        if (depth++ >= BOFS_MAX_TRAVERSAL_DEPTH) {
            return BOFS_ERR_DIR_CYCLE_DETECTED;
        }

        size_t tlen = 0;
        while (*p != '\0' && *p != '/') {
            if (tlen >= 255) return BOFS_ERR_DIR_NAME_TOO_LONG;
            token[tlen++] = *p++;
        }
        token[tlen] = '\0';

        /* Skip consecutive slashes */
        while (*p == '/') p++;

        if (tlen == 0) continue;

        if (strcmp(token, ".") == 0) {
            continue; /* Stay at current directory */
        }

        if (strcmp(token, "..") == 0) {
            if (cur_ino == root_ino) {
                /* Root escape prevention: stays clamped to root */
                continue;
            }
            uint64_t parent_ino = 0;
            int r = bofs_dir_lookup(fs, cur_ino, "..", &parent_ino, &cur_gen, &cur_type);
            if (r != BOFS_DIR_OK) return r;
            cur_ino = parent_ino;
            continue;
        }

        /* Normal component lookup */
        uint64_t next_ino = 0;
        int l_res = bofs_dir_lookup(fs, cur_ino, token, &next_ino, &cur_gen, &cur_type);
        if (l_res != BOFS_DIR_OK) return l_res;

        /* If more components follow, next_ino must be a directory */
        if (*p != '\0' && cur_type != BOFS_FT_DIR) {
            return BOFS_ERR_DIR_NOT_A_DIR;
        }

        cur_ino = next_ino;
    }

    if (out_ino) *out_ino = cur_ino;
    if (out_gen) *out_gen = cur_gen;
    if (out_type) *out_type = cur_type;
    return BOFS_DIR_OK;
}

/* --------------------------------------------------------------------------
 * Public API: bofs_dir_validate (Consistency & Corruption Checker)
 * -------------------------------------------------------------------------- */
int bofs_dir_validate(bofs_file_system_t* fs, uint64_t dir_ino) {
    if (!fs) return BOFS_ERR_DIR_INVALID_PARAM;

    bofs_inode_t ino;
    int r_res = bofs_inode_read(fs, dir_ino, &ino);
    if (r_res != BOFS_FILE_OK) return r_res;

    if ((ino.mode & BOFS_S_IFMT) != BOFS_S_IFDIR) {
        return BOFS_ERR_DIR_NOT_A_DIR;
    }

    uint64_t root_blk = ino.direct_extents[0].physical_block;
    bofs_dir_node_t root_node;
    int val_res = bofs_dir_read_block(fs, root_blk, &root_node);
    if (val_res != BOFS_DIR_OK) return val_res;

    /* Validate slot ordering and uniqueness within root */
    for (uint16_t i = 0; i + 1 < root_node.entry_count; i++) {
        int cmp = bofs_dir_cmp(root_node.slots[i].hash, root_node.slots[i].name,
                               root_node.slots[i + 1].hash, root_node.slots[i + 1].name);
        if (cmp >= 0) {
            return BOFS_ERR_DIR_CORRUPT_NODE;
        }
    }

    return BOFS_DIR_OK;
}
