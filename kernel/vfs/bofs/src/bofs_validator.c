#include "kernel/vfs/bofs/include/bofs_validator.h"
#include "kernel/core/lib/include/string.h"

/* --------------------------------------------------------------------------
 * IEEE 802.3 Standard CRC32 Implementation
 * Polynomial: 0xEDB88320 (Reflected)
 * -------------------------------------------------------------------------- */
static const uint32_t s_crc32_table[256] = {
    0x00000000U, 0x77073096U, 0xEE0E612CU, 0x990951BAU, 0x076DC419U, 0x706AF48FU, 0xE963A535U, 0x9E6495A3U,
    0x0EDB8832U, 0x79DCB8A4U, 0xE0D5E91EU, 0x97D2D988U, 0x09B64C2BU, 0x7EB17CBDU, 0xE7B82D07U, 0x90BF1D91U,
    0x1DB71064U, 0x6AB020F2U, 0xF3B97148U, 0x84BE41DEU, 0x1ADAD47DU, 0x6DDDE4EBU, 0xF4D4B551U, 0x83D385C7U,
    0x136C9856U, 0x646BA8C0U, 0xFD62F97AU, 0x8A65C9ECU, 0x14015C4FU, 0x63066CD9U, 0xFA0F3D63U, 0x8D080DF5U,
    0x3B6E20C8U, 0x4C69105EU, 0xD56041E4U, 0xA2677172U, 0x3C03E4D1U, 0x4B04D447U, 0xD20D85FDU, 0xA50AB56BU,
    0x35B5A8FAU, 0x42B2986CU, 0xDBBBC9D6U, 0xACBCF940U, 0x32D86CE3U, 0x45DF5C75U, 0xDCD60DCFU, 0xABD13D59U,
    0x26D930ACU, 0x51DE003AU, 0xC8D75180U, 0xBFD06116U, 0x21B4F4B5U, 0x56B3C423U, 0xCFBA9599U, 0xB8BDA50FU,
    0x2802B89EU, 0x5F058808U, 0xC60CD9B2U, 0xB10BE924U, 0x2F6F7C87U, 0x58684C11U, 0xC1611DABU, 0xB6662D3DU,
    0x76DC4190U, 0x01DB7106U, 0x98D220BCU, 0xEFD5102AU, 0x71B18589U, 0x06B6B51FU, 0x9FBFE4A5U, 0xE8B8D433U,
    0x7807C9A2U, 0x0F00F934U, 0x9609A88EU, 0xE10E9818U, 0x7F6A0DBBU, 0x086D3D2DU, 0x91646C97U, 0xE6635C01U,
    0x6B6B51F4U, 0x1C6C6162U, 0x856530D8U, 0xF262004EU, 0x6C0695EDU, 0x1B01A57BU, 0x8208F4C1U, 0xF50FC457U,
    0x65B0D9C6U, 0x12B7E950U, 0x8BBEB8EAU, 0xFCB9887CU, 0x62DD1DDFU, 0x15DA2D49U, 0x8CD37CF3U, 0xFBD44C65U,
    0x4DB26158U, 0x3AB551CEU, 0xA3BC0074U, 0xD4BB30E2U, 0x4ADFA541U, 0x3DD895D7U, 0xA4D1C46DU, 0xD3D6F4FBU,
    0x4369E96AU, 0x346ED9FCU, 0xAD678846U, 0xDA60B8D0U, 0x44042D73U, 0x33031DE5U, 0xAA0A4C5FU, 0xDD0D7CC9U,
    0x5005713CU, 0x270241AAU, 0xBE0B1010U, 0xC90C2086U, 0x5768B525U, 0x206F85B3U, 0xB966D409U, 0xCE61E49FU,
    0x5EDEF90EU, 0x29D9C998U, 0xB0D09822U, 0xC7D7A8B4U, 0x59B33D17U, 0x2EB40D81U, 0xB7BD5C3BU, 0xC0BA6CADU,
    0xEDB88320U, 0x9ABFB3B6U, 0x03B6E20CU, 0x74B1D29AU, 0xEAD54739U, 0x9DD277AFU, 0x04DB2615U, 0x73DC1683U,
    0xE3630B12U, 0x94643B84U, 0x0D6D6A3EU, 0x7A6A5AA8U, 0xE40ECF0BU, 0x9309FF9DU, 0x0A00AE27U, 0x7D079EB1U,
    0xF00F9344U, 0x8708A3D2U, 0x1E01F268U, 0x6906C2FEU, 0xF762575DU, 0x806567CBU, 0x196C3671U, 0x6E6B06E7U,
    0xFED41B76U, 0x89D32BE0U, 0x10DA7A5AU, 0x67DD4ACCU, 0xF9B9DF6FU, 0x8EBEEFF9U, 0x17B7BE43U, 0x60B08ED5U,
    0xD6D6A3E8U, 0xA1D1937EU, 0x38D8C2C4U, 0x4FDFF252U, 0xD1BB67F1U, 0xA6BC5767U, 0x3FB506DDU, 0x48B2364BU,
    0xD80D2BDAU, 0xAF0A1B4CU, 0x36034AF6U, 0x41047A60U, 0xDF60EFC3U, 0xA867DF55U, 0x316E8EEFU, 0x4669BE79U,
    0xCB61B38AU, 0xBC66831CU, 0x256FD2A6U, 0x5268E230U, 0xCC0C7793U, 0xBB0B4705U, 0x220216BFU, 0x55052629U,
    0xC5BA3BBEU, 0xB2BD0B28U, 0x2BB45A92U, 0x5CB36A04U, 0xC2D7FFA7U, 0xB5D0CF31U, 0x2CD99E8BU, 0x5BDEAE1DU,
    0x9B64C2B0U, 0xEC63F226U, 0x756AA39CU, 0x026D930AU, 0x9C0906A9U, 0xEB0E363FU, 0x72076785U, 0x05005713U,
    0x95BF4A82U, 0xE2B87A14U, 0x7BB12BAEU, 0x0CB61B38U, 0x92D28E9BU, 0xE5D5BE0DU, 0x7CDCEFB7U, 0x0BDBDF21U,
    0x86D3D2D4U, 0xF1D4E242U, 0x68DDB3F8U, 0x1FDA836EU, 0x81BE16CDU, 0xF6B9265BU, 0x6FB077E1U, 0x18B74777U,
    0x88085AE6U, 0xFF0F6A70U, 0x66063BCAU, 0x11010B5CU, 0x8F659EFFU, 0xF862AE69U, 0x616BFFD3U, 0x166CCF45U,
    0xA00AE278U, 0xD70DD2EEU, 0x4E048354U, 0x3903B3C2U, 0xA7672661U, 0xD06016F7U, 0x4969474DU, 0x3E6E77DBU,
    0xAED16A4AU, 0xD9D65ADCU, 0x40DF0B66U, 0x37D83BF0U, 0xA9BCAE53U, 0xDEBB9EC5U, 0x47B2CF7FU, 0x30B5FFE9U,
    0xBDBDF21CU, 0xCABAC28AU, 0x53B39330U, 0x24B4A3A6U, 0xBAD03605U, 0xCDD70693U, 0x54DE5729U, 0x23D967BFU,
    0xB3667A2EU, 0xC4614AB8U, 0x5D681B02U, 0x2A6F2B94U, 0xB40BBE37U, 0xC30C8EA1U, 0x5A05DF1BU, 0x2D02EF8DU
};

uint32_t bofs_crc32(const void* data, size_t length) {
    if (!data || length == 0) return 0;
    const uint8_t* p = (const uint8_t*)data;
    uint32_t crc = 0xFFFFFFFFU;
    for (size_t i = 0; i < length; i++) {
        crc = s_crc32_table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFU;
}

/* --------------------------------------------------------------------------
 * 64-bit Deterministic Hash Function for B+Tree Directory Routers
 * -------------------------------------------------------------------------- */
uint64_t bofs_hash(const char* name, uint8_t len) {
    if (!name || len == 0) return 0;
    uint64_t hash = BOFS_HASH_SEED;
    for (uint8_t i = 0; i < len; i++) {
        hash ^= (uint64_t)(uint8_t)name[i];
        hash *= 0x100000001B3ULL; /* FNV-1a 64-bit prime */
    }
    return hash;
}

/* --------------------------------------------------------------------------
 * Geometry Calculation
 * -------------------------------------------------------------------------- */
int bofs_calc_geometry(uint64_t total_sectors, uint32_t sector_size, bool compact_mode, bofs_superblock_t* out_sb) {
    if (!out_sb || sector_size == 0) return BOFS_ERR_INTEGER_OVERFLOW;
    if (sector_size != 512 && sector_size != 4096) return BOFS_ERR_UNSUPPORTED_FEATURE;

    /* Prevent overflow on sector calculation */
    if (total_sectors > (UINT64_MAX / sector_size)) return BOFS_ERR_INTEGER_OVERFLOW;
    uint64_t total_bytes = total_sectors * sector_size;
    uint64_t total_blocks = total_bytes / BOFS_BLOCK_SIZE;

    /* Enforce minimum volume requirements */
    uint64_t min_blocks = compact_mode ? 64ULL : 17000ULL;
    if (total_blocks < min_blocks) return BOFS_ERR_OUT_OF_BOUNDS;

    out_sb->magic = BOFS_SUPER_MAGIC;
    out_sb->version_major = BOFS_VERSION_MAJOR;
    out_sb->version_minor = BOFS_VERSION_MINOR;
    out_sb->format_revision = BOFS_FORMAT_REVISION;
    out_sb->state_flags = BOFS_STATE_CLEAN;
    out_sb->feature_compat = BOFS_FEATURE_COMPAT_DIR_HASH | BOFS_FEATURE_COMPAT_SPARSE_FILES;
    out_sb->feature_incompat = BOFS_FEATURES_INCOMPAT_SUPPORTED;
    out_sb->feature_ro_compat = 0;

    out_sb->block_size = BOFS_BLOCK_SIZE;
    out_sb->sector_size = sector_size;
    out_sb->total_blocks = total_blocks;

    /* Region Placements */
    out_sb->primary_sb_block = 0ULL;
    out_sb->backup_sb_block = 1ULL;
    /* Blocks 2 and 3 reserved */

    out_sb->journal_start_block = 4ULL;
    out_sb->journal_block_count = compact_mode ? 16ULL : 8192ULL; /* 64KB compact or 32MB standard */

    out_sb->inode_bitmap_start_block = out_sb->journal_start_block + out_sb->journal_block_count;
    out_sb->inode_bitmap_block_count = compact_mode ? 1ULL : 2ULL; /* 1 block (32K) or 2 blocks (65K) */

    /* Total Inodes */
    out_sb->total_inodes = compact_mode ? 128ULL : 65536ULL;
    out_sb->free_inodes = out_sb->total_inodes - 16ULL; /* Inodes 0-15 reserved */

    /* Inode Table Blocks: 8 Inodes per block */
    out_sb->inode_table_block_count = (out_sb->total_inodes + BOFS_INODES_PER_BLOCK - 1) / BOFS_INODES_PER_BLOCK;

    /* Block Bitmap Sizing: 1 bit per block, 32,768 blocks tracked per 4KB block */
    out_sb->block_bitmap_start_block = out_sb->inode_bitmap_start_block + out_sb->inode_bitmap_block_count;
    out_sb->block_bitmap_block_count = (total_blocks + 32767ULL) / 32768ULL;

    out_sb->inode_table_start_block = out_sb->block_bitmap_start_block + out_sb->block_bitmap_block_count;

    out_sb->data_pool_start_block = out_sb->inode_table_start_block + out_sb->inode_table_block_count;
    if (out_sb->data_pool_start_block >= total_blocks) return BOFS_ERR_OUT_OF_BOUNDS;

    out_sb->data_pool_block_count = total_blocks - out_sb->data_pool_start_block;
    out_sb->free_blocks = out_sb->data_pool_block_count;

    /* Reserved Inodes */
    out_sb->root_inode_num = BOFS_ROOT_INODE;
    out_sb->journal_inode_num = BOFS_JOURNAL_INODE;
    out_sb->block_bitmap_inode_num = BOFS_BLOCK_BITMAP_INODE;
    out_sb->inode_bitmap_inode_num = BOFS_INODE_BITMAP_INODE;

    out_sb->mount_count = 0;
    out_sb->generation = 1;
    out_sb->last_mount_time = 0;
    out_sb->last_write_time = 0;

    return BOFS_VALID_OK;
}

/* --------------------------------------------------------------------------
 * Geometry Validation (Zero Overlap & Overflow Rejection)
 * -------------------------------------------------------------------------- */
int bofs_validate_geometry(const bofs_superblock_t* sb, uint64_t volume_block_capacity) {
    if (!sb) return BOFS_ERR_GEOMETRY_OVERLAP;

    if (sb->block_size != BOFS_BLOCK_SIZE) return BOFS_ERR_UNSUPPORTED_FEATURE;
    if (sb->total_blocks == 0 || sb->total_blocks > volume_block_capacity) return BOFS_ERR_OUT_OF_BOUNDS;

    /* Superblocks verification */
    if (sb->primary_sb_block != 0) return BOFS_ERR_GEOMETRY_OVERLAP;
    if (sb->backup_sb_block != 1) return BOFS_ERR_GEOMETRY_OVERLAP;

    /* Monotonic non-overlapping boundaries check */
    uint64_t cur = 2; /* 0 and 1 occupied */
    if (sb->journal_start_block < cur) return BOFS_ERR_GEOMETRY_OVERLAP;
    if (sb->journal_block_count == 0) return BOFS_ERR_GEOMETRY_OVERLAP;

    /* Check journal overflow */
    if (sb->journal_start_block + sb->journal_block_count < sb->journal_start_block) return BOFS_ERR_INTEGER_OVERFLOW;
    cur = sb->journal_start_block + sb->journal_block_count;

    /* Inode bitmap */
    if (sb->inode_bitmap_start_block < cur) return BOFS_ERR_GEOMETRY_OVERLAP;
    if (sb->inode_bitmap_block_count == 0) return BOFS_ERR_GEOMETRY_OVERLAP;
    if (sb->inode_bitmap_start_block + sb->inode_bitmap_block_count < sb->inode_bitmap_start_block) return BOFS_ERR_INTEGER_OVERFLOW;
    cur = sb->inode_bitmap_start_block + sb->inode_bitmap_block_count;

    /* Block bitmap */
    if (sb->block_bitmap_start_block < cur) return BOFS_ERR_GEOMETRY_OVERLAP;
    if (sb->block_bitmap_block_count == 0) return BOFS_ERR_GEOMETRY_OVERLAP;
    if (sb->block_bitmap_start_block + sb->block_bitmap_block_count < sb->block_bitmap_start_block) return BOFS_ERR_INTEGER_OVERFLOW;
    cur = sb->block_bitmap_start_block + sb->block_bitmap_block_count;

    /* Inode table */
    if (sb->inode_table_start_block < cur) return BOFS_ERR_GEOMETRY_OVERLAP;
    if (sb->inode_table_block_count == 0) return BOFS_ERR_GEOMETRY_OVERLAP;
    if (sb->inode_table_start_block + sb->inode_table_block_count < sb->inode_table_start_block) return BOFS_ERR_INTEGER_OVERFLOW;
    cur = sb->inode_table_start_block + sb->inode_table_block_count;

    /* Data pool */
    if (sb->data_pool_start_block < cur) return BOFS_ERR_GEOMETRY_OVERLAP;
    if (sb->data_pool_start_block + sb->data_pool_block_count < sb->data_pool_start_block) return BOFS_ERR_INTEGER_OVERFLOW;
    if (sb->data_pool_start_block + sb->data_pool_block_count > sb->total_blocks) return BOFS_ERR_OUT_OF_BOUNDS;

    return BOFS_VALID_OK;
}

/* --------------------------------------------------------------------------
 * Core Validation Functions
 * -------------------------------------------------------------------------- */
int bofs_validate_superblock(const bofs_superblock_t* sb, uint64_t volume_block_capacity) {
    if (!sb) return BOFS_ERR_INVALID_MAGIC;

    /* 1. Magic check */
    if (sb->magic != BOFS_SUPER_MAGIC) return BOFS_ERR_INVALID_MAGIC;

    /* 2. Version check */
    if (sb->version_major != BOFS_VERSION_MAJOR) return BOFS_ERR_UNSUPPORTED_VERSION;

    /* 3. Incompatible features check */
    if ((sb->feature_incompat & ~BOFS_FEATURES_INCOMPAT_SUPPORTED) != 0) {
        return BOFS_ERR_UNSUPPORTED_FEATURE;
    }

    /* 4. Geometry validity check */
    int geom_res = bofs_validate_geometry(sb, volume_block_capacity);
    if (geom_res != BOFS_VALID_OK) return geom_res;

    /* 5. Checksum verification */
    uint32_t computed_crc = bofs_crc32(sb, offsetof(bofs_superblock_t, checksum));
    if (computed_crc != sb->checksum) return BOFS_ERR_CHECKSUM_MISMATCH;

    return BOFS_VALID_OK;
}

int bofs_validate_inode(const bofs_inode_t* inode, uint64_t expected_inode_num) {
    if (!inode) return BOFS_ERR_INVALID_MAGIC;

    /* 1. Magic check */
    if (inode->magic != BOFS_INODE_MAGIC) return BOFS_ERR_INVALID_MAGIC;

    /* 2. Inode number identity check */
    if (expected_inode_num != 0 && inode->inode_num != expected_inode_num) {
        return BOFS_ERR_CORRUPTED_INODE;
    }

    /* 3. File type check */
    uint16_t file_type = inode->mode & BOFS_S_IFMT;
    if (file_type != BOFS_S_IFREG && file_type != BOFS_S_IFDIR &&
        file_type != BOFS_S_IFLNK && file_type != BOFS_S_IFBLK &&
        file_type != BOFS_S_IFCHR && file_type != BOFS_S_IFIFO &&
        file_type != BOFS_S_IFSOCK) {
        return BOFS_ERR_CORRUPTED_INODE;
    }

    /* 4. Checksum verification */
    uint32_t computed_crc = bofs_crc32(inode, offsetof(bofs_inode_t, checksum));
    if (computed_crc != inode->checksum) return BOFS_ERR_CHECKSUM_MISMATCH;

    return BOFS_VALID_OK;
}

int bofs_validate_dir_node(const bofs_dir_node_t* node) {
    if (!node) return BOFS_ERR_INVALID_MAGIC;

    if (node->magic != BOFS_DIR_MAGIC) return BOFS_ERR_INVALID_MAGIC;
    if (node->node_type != BOFS_DIR_NODE_LEAF && node->node_type != BOFS_DIR_NODE_ROUTER) {
        return BOFS_ERR_CORRUPTED_DIR_NODE;
    }
    if (node->entry_count > BOFS_DIR_SLOTS_PER_BLOCK) return BOFS_ERR_INVALID_ENTRY_COUNT;

    uint32_t computed_crc = bofs_crc32(node, offsetof(bofs_dir_node_t, checksum));
    if (computed_crc != node->checksum) return BOFS_ERR_CHECKSUM_MISMATCH;

    return BOFS_VALID_OK;
}

int bofs_validate_journal_header(const bofs_journal_header_t* jh) {
    if (!jh) return BOFS_ERR_INVALID_MAGIC;

    if (jh->magic != BOFS_JOURNAL_MAGIC) return BOFS_ERR_INVALID_MAGIC;
    if (jh->block_size != BOFS_BLOCK_SIZE) return BOFS_ERR_UNSUPPORTED_FEATURE;
    if (jh->head_block >= jh->total_blocks || jh->tail_block >= jh->total_blocks) {
        return BOFS_ERR_CORRUPTED_JOURNAL;
    }

    uint32_t computed_crc = bofs_crc32(jh, offsetof(bofs_journal_header_t, checksum));
    if (computed_crc != jh->checksum) return BOFS_ERR_CHECKSUM_MISMATCH;

    return BOFS_VALID_OK;
}

int bofs_validate_journal_desc(const bofs_journal_desc_t* desc) {
    if (!desc) return BOFS_ERR_INVALID_MAGIC;

    if (desc->magic != BOFS_TXN_DESC_MAGIC) return BOFS_ERR_INVALID_MAGIC;
    if (desc->record_type != 1) return BOFS_ERR_CORRUPTED_JOURNAL;
    if (desc->block_count > BOFS_MAX_TXN_BLOCKS) return BOFS_ERR_CORRUPTED_JOURNAL;

    uint32_t computed_crc = bofs_crc32(desc, offsetof(bofs_journal_desc_t, checksum));
    if (computed_crc != desc->checksum) return BOFS_ERR_CHECKSUM_MISMATCH;

    return BOFS_VALID_OK;
}

int bofs_validate_journal_commit(const bofs_journal_commit_t* commit) {
    if (!commit) return BOFS_ERR_INVALID_MAGIC;

    if (commit->magic != BOFS_TXN_COMMIT_MAGIC) return BOFS_ERR_INVALID_MAGIC;
    if (commit->record_type != 2) return BOFS_ERR_CORRUPTED_JOURNAL;

    uint32_t computed_crc = bofs_crc32(commit, offsetof(bofs_journal_commit_t, checksum));
    if (computed_crc != commit->checksum) return BOFS_ERR_CHECKSUM_MISMATCH;

    return BOFS_VALID_OK;
}

/* --------------------------------------------------------------------------
 * Serialization Helpers
 * -------------------------------------------------------------------------- */
void bofs_init_superblock(bofs_superblock_t* sb, const uint8_t uuid[16], const char* label) {
    if (!sb) return;
    if (uuid) memcpy(sb->uuid, uuid, 16);
    if (label) {
        strncpy(sb->volume_label, label, sizeof(sb->volume_label) - 1);
        sb->volume_label[sizeof(sb->volume_label) - 1] = '\0';
    }
    sb->checksum = bofs_crc32(sb, offsetof(bofs_superblock_t, checksum));
}

void bofs_init_root_inode(bofs_inode_t* inode) {
    if (!inode) return;
    memset(inode, 0, sizeof(bofs_inode_t));
    inode->magic = BOFS_INODE_MAGIC;
    inode->generation = 1;
    inode->inode_num = BOFS_ROOT_INODE;
    inode->mode = BOFS_S_IFDIR | 0755U; /* Directory, rwxr-xr-x */
    inode->flags = BOFS_INODE_FLAG_SYSTEM;
    inode->uid = 0; /* root */
    inode->gid = 0; /* root */
    inode->link_count = 2; /* '.' and '..' */
    inode->size_bytes = BOFS_BLOCK_SIZE;
    inode->allocated_blocks = 1;

    inode->checksum = bofs_crc32(inode, offsetof(bofs_inode_t, checksum));
}

void bofs_init_journal_header(bofs_journal_header_t* jh, uint64_t total_journal_blocks) {
    if (!jh) return;
    memset(jh, 0, sizeof(bofs_journal_header_t));
    jh->magic = BOFS_JOURNAL_MAGIC;
    jh->version = 1;
    jh->flags = 0;
    jh->block_size = BOFS_BLOCK_SIZE;
    jh->total_blocks = total_journal_blocks;
    jh->head_block = 1; /* Block 0 is journal superblock itself */
    jh->tail_block = 1;
    jh->sequence_number = 1;
    jh->last_commit_seq = 0;

    jh->checksum = bofs_crc32(jh, offsetof(bofs_journal_header_t, checksum));
}

void bofs_init_dir_node(bofs_dir_node_t* node, uint16_t node_type, uint32_t level) {
    if (!node) return;
    memset(node, 0, sizeof(bofs_dir_node_t));
    node->magic = BOFS_DIR_MAGIC;
    node->node_type = node_type;
    node->entry_count = 0;
    node->tree_level = level;
    node->generation = 1;

    node->checksum = bofs_crc32(node, offsetof(bofs_dir_node_t, checksum));
}
