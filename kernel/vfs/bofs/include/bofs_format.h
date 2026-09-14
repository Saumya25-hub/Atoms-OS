#ifndef BOFS_FORMAT_H
#define BOFS_FORMAT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==========================================================================
 * ATOMS OS — BOFS (BOS Operating Filesystem) V1 On-Disk Format Specification
 * Document ID: ATOMS-BOFS-PHASE3-SPEC-001
 * Canonical Byte Order: Little-Endian (Standard x86_64)
 * Fundamental Allocation Quantum: 4,096 Bytes (4 KB Block)
 * ========================================================================== */

/* --------------------------------------------------------------------------
 * Magic Signatures (Little-Endian uint32)
 * -------------------------------------------------------------------------- */
#define BOFS_SUPER_MAGIC            0x53464F42U  /* 'BOFS' -> 'B'=0x42, 'O'=0x4F, 'F'=0x46, 'S'=0x53 */
#define BOFS_INODE_MAGIC            0x4F4E4942U  /* 'BINO' -> 'B'=0x42, 'I'=0x49, 'N'=0x4E, 'O'=0x4F */
#define BOFS_DIR_MAGIC              0x52494442U  /* 'BDIR' -> 'B'=0x42, 'D'=0x44, 'I'=0x49, 'R'=0x52 */
#define BOFS_JOURNAL_MAGIC          0x4C4E4A42U  /* 'BJNL' -> 'B'=0x42, 'J'=0x4A, 'N'=0x4E, 'L'=0x4C */
#define BOFS_TXN_DESC_MAGIC         0x4E585442U  /* 'BTXN' -> 'B'=0x42, 'T'=0x54, 'X'=0x58, 'N'=0x4E */
#define BOFS_TXN_COMMIT_MAGIC       0x544D4342U  /* 'BCMT' -> 'B'=0x42, 'C'=0x43, 'M'=0x4D, 'T'=0x54 */

/* --------------------------------------------------------------------------
 * Versioning & ABI
 * -------------------------------------------------------------------------- */
#define BOFS_VERSION_MAJOR          1U
#define BOFS_VERSION_MINOR          0U
#define BOFS_FORMAT_REVISION        1U
#define BOFS_ABI_VERSION            1U

/* --------------------------------------------------------------------------
 * Storage Geometry Constants
 * -------------------------------------------------------------------------- */
#define BOFS_BLOCK_SIZE             4096U
#define BOFS_BLOCK_BITS             12U
#define BOFS_INODE_SIZE             512U
#define BOFS_INODES_PER_BLOCK       8U       /* 4096 / 512 = 8 */

#define BOFS_MIN_SECTOR_SIZE        512U
#define BOFS_MAX_SECTOR_SIZE        4096U

/* Reserved Inode Numbers */
#define BOFS_NULL_INODE             0ULL
#define BOFS_ROOT_INODE             1ULL     /* Volume Root Directory '/' */
#define BOFS_JOURNAL_INODE          2ULL     /* Write-Ahead Journal Ring */
#define BOFS_BLOCK_BITMAP_INODE     3ULL     /* Block Allocation Bitmap */
#define BOFS_INODE_BITMAP_INODE     4ULL     /* Inode Allocation Bitmap */
#define BOFS_BAD_BLOCKS_INODE       5ULL     /* Bad Blocks Extent Map */
#define BOFS_FIRST_USER_INODE       16ULL    /* First Allocatable Inode */

/* --------------------------------------------------------------------------
 * Superblock State & Feature Flags
 * -------------------------------------------------------------------------- */
#define BOFS_STATE_CLEAN            0x0001U  /* Volume was cleanly unmounted */
#define BOFS_STATE_DIRTY            0x0002U  /* Volume unmount was interrupted (journal replay required) */
#define BOFS_STATE_RECOVERING       0x0004U  /* In active journal recovery mode */
#define BOFS_STATE_DEGRADED         0x0008U  /* Checksum failure detected; mounted read-only */

/* Feature Compatibility Flags */
#define BOFS_FEATURE_COMPAT_DIR_HASH      0x00000001U  /* B+Tree Hash Indexing */
#define BOFS_FEATURE_COMPAT_SPARSE_FILES  0x00000002U  /* Explicit sparse extent flags */

/* Feature Incompatibility Flags (Mount rejected if bit unrecognized) */
#define BOFS_FEATURE_INCOMPAT_EXTENTS     0x00000001U  /* Extent-based addressing (Mandatory V1) */
#define BOFS_FEATURE_INCOMPAT_JOURNAL     0x00000002U  /* Ordered WAL Journaling (Mandatory V1) */
#define BOFS_FEATURE_INCOMPAT_64BIT       0x00000004U  /* Native 64-bit addressing (Mandatory V1) */

#define BOFS_FEATURES_INCOMPAT_SUPPORTED  (BOFS_FEATURE_INCOMPAT_EXTENTS | \
                                           BOFS_FEATURE_INCOMPAT_JOURNAL | \
                                           BOFS_FEATURE_INCOMPAT_64BIT)

/* --------------------------------------------------------------------------
 * Structure: Extent Descriptor (24 Bytes)
 * -------------------------------------------------------------------------- */
#define BOFS_EXTENT_FLAG_VALID            0x00000001U  /* Extent record is active */
#define BOFS_EXTENT_FLAG_SPARSE           0x00000002U  /* Explicit sparse hole (no physical block allocated) */
#define BOFS_EXTENT_FLAG_UNWRITTEN        0x00000004U  /* Allocated but uninitialized */

typedef struct {
    uint64_t logical_block;      /* Logical block offset within file */
    uint64_t physical_block;     /* Physical BOFS block index on partition (ignored if SPARSE) */
    uint32_t block_count;        /* Contiguous 4KB block count in this extent */
    uint32_t flags;              /* BOFS_EXTENT_FLAG_* */
} __attribute__((packed)) bofs_extent_t;

_Static_assert(sizeof(bofs_extent_t) == 24, "BOFS: bofs_extent_t must be exactly 24 bytes");

/* --------------------------------------------------------------------------
 * Structure: Inode (512 Bytes)
 * -------------------------------------------------------------------------- */
#define BOFS_INODE_FLAG_IMMUTABLE         0x0001U
#define BOFS_INODE_FLAG_SYSTEM            0x0002U
#define BOFS_INODE_FLAG_HIDDEN            0x0004U
#define BOFS_INODE_FLAG_ARCHIVE           0x0008U
#define BOFS_INODE_FLAG_INDEXED_DIR       0x0010U  /* Directory uses B+Tree rather than linear */

/* Standard POSIX File Mode Types (Octal) */
#define BOFS_S_IFMT                       0170000U
#define BOFS_S_IFSOCK                     0140000U
#define BOFS_S_IFLNK                      0120000U
#define BOFS_S_IFREG                      0100000U
#define BOFS_S_IFBLK                      0060000U
#define BOFS_S_IFDIR                      0040000U
#define BOFS_S_IFCHR                      0020000U
#define BOFS_S_IFIFO                      0010000U

/* POSIX Mode Permissions */
#define BOFS_S_ISUID                      04000U
#define BOFS_S_ISGID                      02000U
#define BOFS_S_ISVTX                      01000U
#define BOFS_S_IRWXU                      00700U
#define BOFS_S_IRUSR                      00400U
#define BOFS_S_IWUSR                      00200U
#define BOFS_S_IXUSR                      00100U
#define BOFS_S_IRWXG                      00070U
#define BOFS_S_IRGRP                      00040U
#define BOFS_S_IWGRP                      00020U
#define BOFS_S_IXGRP                      00010U
#define BOFS_S_IRWXO                      00007U
#define BOFS_S_IROTH                      00004U
#define BOFS_S_IWOTH                      00002U
#define BOFS_S_IXOTH                      00001U

#define BOFS_INODE_DIRECT_EXTENTS         12U

typedef struct {
    uint32_t magic;              /* Offset 0x000: 'BINO' (0x4F4E4942) */
    uint32_t generation;         /* Offset 0x004: Inode lifecycle generation counter */
    uint64_t inode_num;          /* Offset 0x008: Absolute Inode index */
    uint16_t mode;               /* Offset 0x010: POSIX mode bits (Type + Permissions) */
    uint16_t flags;              /* Offset 0x012: BOFS_INODE_FLAG_* */
    uint32_t uid;                /* Offset 0x014: Owner User ID */
    uint32_t gid;                /* Offset 0x018: Owner Group ID */
    uint32_t link_count;         /* Offset 0x01C: Hard link reference counter */
    uint64_t size_bytes;         /* Offset 0x020: Logical file size in bytes */
    uint64_t allocated_blocks;   /* Offset 0x028: Total 4KB physical blocks allocated */

    /* Nanosecond High-Resolution POSIX Timestamps (UTC relative to Epoch) */
    uint64_t atime_sec;          /* Offset 0x030: Last access time (seconds) */
    uint32_t atime_nsec;         /* Offset 0x038: Last access time (nanoseconds) */
    uint32_t reserved_time1;     /* Offset 0x03C: Alignment padding */

    uint64_t mtime_sec;          /* Offset 0x040: Last modification time (seconds) */
    uint32_t mtime_nsec;         /* Offset 0x048: Last modification time (nanoseconds) */
    uint32_t reserved_time2;     /* Offset 0x04C: Alignment padding */

    uint64_t ctime_sec;          /* Offset 0x050: Last metadata change time (seconds) */
    uint32_t ctime_nsec;         /* Offset 0x058: Last metadata change time (nanoseconds) */
    uint32_t reserved_time3;     /* Offset 0x05C: Alignment padding */

    uint64_t crtime_sec;         /* Offset 0x060: File creation / birth time (seconds) */
    uint32_t crtime_nsec;        /* Offset 0x068: File creation time (nanoseconds) */
    uint32_t reserved_time4;     /* Offset 0x06C: Alignment padding */

    /* Direct Extent Descriptors: 12 Extents * 24 Bytes = 288 Bytes */
    bofs_extent_t direct_extents[BOFS_INODE_DIRECT_EXTENTS]; /* Offset 0x070 - 0x18F */

    /* Multi-Tier Indirect Extent Block References */
    uint64_t indirect_block;     /* Offset 0x190: 1st Tier Indirect Block Pointer (0 if unused) */
    uint64_t double_indirect_block; /* Offset 0x198: 2nd Tier Double Indirect Pointer (0 if unused) */

    /* Reserved for Extended Attributes / Inline Security Descriptors */
    uint8_t  extended_attributes[88]; /* Offset 0x1A0 - 0x1F7 (88 Bytes) */

    /* Inode Validation */
    uint32_t reserved;           /* Offset 0x1F8: Alignment padding (4 Bytes) */
    uint32_t checksum;           /* Offset 0x1FC: IEEE 802.3 CRC32 over bytes 0x000 to 0x1FB (4 Bytes) */
} __attribute__((packed)) bofs_inode_t;

_Static_assert(sizeof(bofs_inode_t) == 512, "BOFS: bofs_inode_t must be exactly 512 bytes");

/* --------------------------------------------------------------------------
 * Structure: Superblock (4,096 Bytes = 1 Block)
 * -------------------------------------------------------------------------- */
typedef struct {
    uint32_t magic;              /* Offset 0x000: 'BOFS' (0x53464F42) */
    uint16_t version_major;      /* Offset 0x004: 1 */
    uint16_t version_minor;      /* Offset 0x006: 0 */
    uint16_t format_revision;    /* Offset 0x008: 1 */
    uint16_t state_flags;        /* Offset 0x00A: BOFS_STATE_* */
    uint32_t feature_compat;     /* Offset 0x00C: Compatible feature mask */
    uint32_t feature_incompat;   /* Offset 0x010: Incompatible feature mask (Must reject if unsupported) */
    uint32_t feature_ro_compat;  /* Offset 0x014: Read-only compatible feature mask */
    uint8_t  uuid[16];           /* Offset 0x018: 128-bit RFC 4122 Volume UUID */
    char     volume_label[64];   /* Offset 0x028: Null-terminated UTF-8 Volume Label */

    uint32_t block_size;         /* Offset 0x068: Fixed 4,096 Bytes */
    uint32_t sector_size;        /* Offset 0x06C: Physical Sector Size (512 or 4096) */
    uint64_t total_blocks;       /* Offset 0x070: Total volume capacity in 4KB blocks */
    uint64_t free_blocks;        /* Offset 0x078: Unallocated block count */
    uint64_t total_inodes;       /* Offset 0x080: Fixed Inode capacity (65,536 in standard V1) */
    uint64_t free_inodes;        /* Offset 0x088: Unallocated Inode count */

    /* Structural Region Geometry Offsets (Measured in 4KB Blocks) */
    uint64_t primary_sb_block;         /* Offset 0x090: Block 0 */
    uint64_t backup_sb_block;          /* Offset 0x098: Block 1 */
    uint64_t journal_start_block;      /* Offset 0x0A0: Block 4 */
    uint64_t journal_block_count;      /* Offset 0x0A8: 8,192 Blocks (32 MB) */
    uint64_t inode_bitmap_start_block; /* Offset 0x0B0: Block 8,196 */
    uint64_t inode_bitmap_block_count; /* Offset 0x0B8: 2 Blocks (covers 65,536 Inodes) */
    uint64_t block_bitmap_start_block; /* Offset 0x0C0: Block 8,198 */
    uint64_t block_bitmap_block_count; /* Offset 0x0C8: Sized to cover total_blocks */
    uint64_t inode_table_start_block;  /* Offset 0x0D0: Fixed Inode Table Base */
    uint64_t inode_table_block_count;  /* Offset 0x0D8: 8,192 Blocks for 65,536 Inodes */
    uint64_t data_pool_start_block;    /* Offset 0x0E0: First general data/index block */
    uint64_t data_pool_block_count;    /* Offset 0x0E8: Total payload blocks */

    /* Well-Known Reserved Inode Assignments */
    uint64_t root_inode_num;           /* Offset 0x0F0: Inode 1 */
    uint64_t journal_inode_num;        /* Offset 0x0F8: Inode 2 */
    uint64_t block_bitmap_inode_num;   /* Offset 0x100: Inode 3 */
    uint64_t inode_bitmap_inode_num;   /* Offset 0x108: Inode 4 */

    /* Mount & Lifecycle Metrics */
    uint64_t mount_count;              /* Offset 0x110: Total times mounted */
    uint64_t generation;               /* Offset 0x118: Monotonic Superblock commit generation */
    uint64_t last_mount_time;          /* Offset 0x120: Nanoseconds since Epoch */
    uint64_t last_write_time;          /* Offset 0x128: Nanoseconds since Epoch */

    /* Reserved for Expansion (Padded to 4,092 Bytes) */
    uint8_t  reserved[3788];           /* Offset 0x130 - 0xFFB (3,788 Bytes) */

    /* Superblock Integrity Checksum */
    uint32_t checksum;                 /* Offset 0xFFC: IEEE 802.3 CRC32 over bytes 0x000 to 0xFFB */
} __attribute__((packed)) bofs_superblock_t;

_Static_assert(sizeof(bofs_superblock_t) == 4096, "BOFS: bofs_superblock_t must be exactly 4096 bytes");

/* --------------------------------------------------------------------------
 * Structure: Directory Entry (Variable Length on Disk, Max 272 Bytes)
 * -------------------------------------------------------------------------- */
#define BOFS_MAX_FILENAME_LEN        255U

/* Fast Directory File Types */
#define BOFS_FT_UNKNOWN              0U
#define BOFS_FT_REG                  1U
#define BOFS_FT_DIR                  2U
#define BOFS_FT_CHR                  3U
#define BOFS_FT_BLK                  4U
#define BOFS_FT_FIFO                 5U
#define BOFS_FT_SOCK                 6U
#define BOFS_FT_SYMLINK              7U
#define BOFS_FT_BOSX                 8U  /* Native ATOMS Binary Object System Executable */

typedef struct {
    uint64_t inode_num;          /* Target Inode Number */
    uint16_t rec_len;            /* Total byte length of this directory entry record */
    uint8_t  name_len;           /* Filename byte length (1 to 255) */
    uint8_t  file_type;          /* BOFS_FT_* */
    uint32_t flags;              /* Reserved / Directory Entry Flags */
    char     name[256];          /* Canonical UTF-8 Filename (Null-Terminated) */
} __attribute__((packed)) bofs_dirent_t;

_Static_assert(sizeof(bofs_dirent_t) == 272, "BOFS: bofs_dirent_t must be exactly 272 bytes");

/* --------------------------------------------------------------------------
 * Structure: Directory B+Tree Node (4,096 Bytes = 1 Block)
 * -------------------------------------------------------------------------- */
#define BOFS_DIR_NODE_LEAF           1U
#define BOFS_DIR_NODE_ROUTER         2U

#define BOFS_HASH_SEED               0x5F424F46535F5631ULL  /* '_BOFS_V1' */

typedef struct {
    uint64_t hash;               /* 64-bit deterministic hash of filename */
    uint64_t child_block;        /* For Router Nodes: Target child block index */
    uint64_t inode_num;          /* For Leaf Nodes: Target Inode number */
    uint16_t rec_len;            /* Record length */
    uint8_t  name_len;           /* Filename length */
    uint8_t  file_type;          /* Cached file type */
    char     name[36];           /* Inline name prefix or truncated key (null-terminated) */
} __attribute__((packed)) bofs_dir_entry_slot_t;

_Static_assert(sizeof(bofs_dir_entry_slot_t) == 64, "BOFS: bofs_dir_entry_slot_t must be exactly 64 bytes");

#define BOFS_DIR_SLOTS_PER_BLOCK     62U  /* 62 * 64 = 3,968 Bytes */

typedef struct {
    uint32_t magic;              /* Offset 0x000: 'BDIR' (0x52494442) */
    uint16_t node_type;          /* Offset 0x004: BOFS_DIR_NODE_LEAF or BOFS_DIR_NODE_ROUTER */
    uint16_t entry_count;        /* Offset 0x006: Current active entries in this node */
    uint32_t tree_level;         /* Offset 0x008: 0 for Leaf Node, 1+ for Router/Internal */
    uint32_t generation;         /* Offset 0x00C: Directory block mutation generation */
    uint64_t parent_block;       /* Offset 0x010: Parent B+Tree block index (0 if root) */
    uint64_t left_sibling_block; /* Offset 0x018: Left sibling block index */
    uint64_t right_sibling_block;/* Offset 0x020: Right sibling block index */
    uint8_t  reserved[84];       /* Offset 0x028: Padding to offset 0x07C (84 bytes header padding) */

    /* Slots: 62 Slots * 64 Bytes = 3,968 Bytes (Offset 0x07C to 0xFFB) */
    bofs_dir_entry_slot_t slots[BOFS_DIR_SLOTS_PER_BLOCK];

    /* Node Integrity Checksum */
    uint32_t checksum;           /* Offset 0xFFC: IEEE 802.3 CRC32 over bytes 0x000 to 0xFFB */
} __attribute__((packed)) bofs_dir_node_t;

_Static_assert(sizeof(bofs_dir_node_t) == 4096, "BOFS: bofs_dir_node_t must be exactly 4096 bytes");

/* --------------------------------------------------------------------------
 * Structure: Write-Ahead Journal Ring (WAL) Structures (4,096 Bytes Each)
 * -------------------------------------------------------------------------- */
typedef struct {
    uint32_t magic;              /* Offset 0x000: 'BJNL' (0x4C4E4A42) */
    uint16_t version;            /* Offset 0x004: 1 */
    uint16_t flags;              /* Offset 0x006: Journal state flags */
    uint32_t block_size;         /* Offset 0x008: 4,096 Bytes */
    uint32_t reserved_hdr;       /* Offset 0x00C: Alignment */
    uint64_t total_blocks;       /* Offset 0x010: Total blocks in journal ring (e.g. 8,192) */
    uint64_t head_block;         /* Offset 0x018: First active transaction block offset */
    uint64_t tail_block;         /* Offset 0x020: Next block offset to allocate */
    uint64_t sequence_number;    /* Offset 0x028: Monotonically incrementing transaction sequence */
    uint64_t last_commit_seq;    /* Offset 0x030: Last transaction committed to disk */

    uint8_t  reserved[4036];     /* Offset 0x038 - 0xFFB */
    uint32_t checksum;           /* Offset 0xFFC: IEEE 802.3 CRC32 over bytes 0x000 to 0xFFB */
} __attribute__((packed)) bofs_journal_header_t;

_Static_assert(sizeof(bofs_journal_header_t) == 4096, "BOFS: bofs_journal_header_t must be exactly 4096 bytes");

#define BOFS_MAX_TXN_BLOCKS          240U

typedef struct {
    uint32_t magic;              /* Offset 0x000: 'BTXN' (0x4E585442) */
    uint32_t record_type;        /* Offset 0x004: 1 = DESCRIPTOR */
    uint64_t transaction_id;     /* Offset 0x008: Unique transaction identifier */
    uint64_t sequence_number;    /* Offset 0x010: Monotonic transaction sequence */
    uint32_t block_count;        /* Offset 0x018: Number of metadata blocks attached (1 to 240) */
    uint32_t flags;              /* Offset 0x01C: Transaction flags */

    /* Target Physical Block Mappings: 240 * 8 = 1,920 Bytes */
    uint64_t target_blocks[BOFS_MAX_TXN_BLOCKS]; /* Offset 0x020 - 0x79F */

    uint8_t  reserved[2136];     /* Offset 0x7A0 - 0xFF7 (2,136 Bytes) */
    uint32_t padding;            /* Offset 0xFF8: Alignment (4 Bytes) */
    uint32_t checksum;           /* Offset 0xFFC: IEEE 802.3 CRC32 over bytes 0x000 to 0xFFB */
} __attribute__((packed)) bofs_journal_desc_t;

_Static_assert(sizeof(bofs_journal_desc_t) == 4096, "BOFS: bofs_journal_desc_t must be exactly 4096 bytes");

typedef struct {
    uint32_t magic;              /* Offset 0x000: 'BCMT' (0x544D4342) */
    uint32_t record_type;        /* Offset 0x004: 2 = COMMIT */
    uint64_t transaction_id;     /* Offset 0x008: Transaction identifier matching descriptor */
    uint64_t sequence_number;    /* Offset 0x010: Monotonic sequence number */
    uint64_t commit_timestamp;   /* Offset 0x018: Nanoseconds since Epoch when committed */

    uint8_t  reserved[4060];     /* Offset 0x020 - 0xFFB */
    uint32_t checksum;           /* Offset 0xFFC: IEEE 802.3 CRC32 over bytes 0x000 to 0xFFB */
} __attribute__((packed)) bofs_journal_commit_t;

_Static_assert(sizeof(bofs_journal_commit_t) == 4096, "BOFS: bofs_journal_commit_t must be exactly 4096 bytes");

#ifdef __cplusplus
}
#endif

#endif /* BOFS_FORMAT_H */
