#ifndef NTFS_H
#define NTFS_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/vfs/vfs_legacy/storage/include/block_device.h"

// On-disk NTFS Boot Sector / Bios Parameter Block (512 bytes)
#pragma pack(push, 1)
typedef struct {
    uint8_t  jump[3];                  // 0x00: Jump instruction
    char     oem_id[8];                // 0x03: OEM identifier ("NTFS    ")
    uint16_t bytes_per_sector;        // 0x0B: Bytes per sector (512, 1024, 2048, 4096)
    uint8_t  sectors_per_cluster;     // 0x0D: Sectors per cluster
    uint16_t reserved_sectors;        // 0x0E: Reserved sectors (0 in NTFS)
    uint8_t  zero1[3];                // 0x10: Always 0
    uint16_t zero2;                   // 0x13: Always 0
    uint8_t  media_descriptor;        // 0x15: Media descriptor (e.g. 0xF8)
    uint16_t zero3;                   // 0x16: Always 0
    uint16_t sectors_per_track;       // 0x18: Sectors per track
    uint16_t heads;                   // 0x1A: Number of heads
    uint32_t hidden_sectors;          // 0x1C: Hidden sectors
    uint32_t zero4;                   // 0x20: Always 0
    uint32_t zero5;                   // 0x24: Always 0 or 0x80000000
    uint64_t total_sectors;           // 0x28: Total sectors in volume
    uint64_t mft_cluster;             // 0x30: Starting LCN of $MFT
    uint64_t mft_mirr_cluster;        // 0x38: Starting LCN of $MFTMirr
    int8_t   clusters_per_mft_record; // 0x40: FILE record size encoding
    uint8_t  reserved1[3];            // 0x41: Reserved
    int8_t   clusters_per_index_buffer;// 0x44: Index buffer size encoding
    uint8_t  reserved2[3];            // 0x45: Reserved
    uint64_t volume_serial_number;    // 0x48: Volume serial number
    uint32_t checksum;                // 0x50: Boot sector checksum
    uint8_t  boot_code[426];          // 0x54: Bootstrap code
    uint16_t boot_sector_signature;   // 0x01FE: Boot sector magic (0xAA55)
} NTFS_BootSector;
#pragma pack(pop)

// In-Memory Extent Representation (Phase 3)
typedef struct {
    uint64_t vcn_start;
    uint64_t cluster_count;
    int64_t  lcn_start; // -1 if sparse, else physical LCN
    bool     is_sparse;
} NTFS_Extent;

typedef struct {
    uint32_t     extent_count;
    uint32_t     capacity;
    NTFS_Extent* extents;
    uint64_t     total_clusters;
    uint64_t     data_size_bytes;
} NTFS_ExtentMap;

// Phase 4: Sector Read Cache Subsystem
#define NTFS_CACHE_SIZE 64

typedef struct {
    uint64_t lba;
    uint8_t  data[512];
    bool     valid;
    uint32_t access_count;
} NTFS_CacheEntry;

typedef struct {
    NTFS_CacheEntry entries[NTFS_CACHE_SIZE];
    uint64_t hits;
    uint64_t misses;
    uint64_t evictions;
} NTFS_ReadCache;

// Phase 7B: MFT Record Cache Subsystem
#define NTFS_MFT_CACHE_SIZE 32

typedef struct {
    uint32_t record_number;
    uint8_t* record_buffer; // Heap allocated buffer of size file_record_size
    uint32_t record_size;
    uint16_t usa_offset;
    uint16_t usa_count;
    uint64_t lsn;
    uint16_t sequence_number;
    uint16_t hard_link_count;
    uint16_t first_attribute_offset;
    uint16_t flags;
    uint32_t bytes_in_use;
    uint32_t bytes_allocated;
    uint64_t base_file_record;
    uint16_t next_attribute_id;
    bool     valid;
    uint32_t access_count;
} NTFS_MFTCacheEntry;

typedef struct {
    NTFS_MFTCacheEntry entries[NTFS_MFT_CACHE_SIZE];
    uint64_t hits;
    uint64_t misses;
    uint64_t evictions;
} NTFS_MFTCache;

// Phase 7E: Path / Directory Lookup Acceleration Cache Subsystem
#define NTFS_PATH_CACHE_SIZE 16

typedef struct {
    char     path[256];
    uint32_t record_number;
    bool     valid;
    uint32_t access_count;
} NTFS_PathCacheEntry;

typedef struct {
    NTFS_PathCacheEntry entries[NTFS_PATH_CACHE_SIZE];
    uint64_t hits;
    uint64_t misses;
    uint64_t evictions;
} NTFS_PathCache;

// Phase 7F: Production Observability & Performance Diagnostics
typedef struct {
    uint64_t device_reads;
    uint64_t device_sectors_read;
    uint64_t sector_cache_hits;
    uint64_t sector_cache_misses;
    uint64_t sector_cache_evictions;
    uint64_t mft_cache_hits;
    uint64_t mft_cache_misses;
    uint64_t mft_cache_evictions;
    uint64_t path_cache_hits;
    uint64_t path_cache_misses;
    uint64_t path_cache_evictions;
    uint64_t sparse_bytes_synthesized;
    uint64_t read_ahead_triggers;
    uint64_t prefetched_sectors;
    uint64_t coalesced_reads;
    uint64_t bytes_returned_vfs;
} NTFS_PerfStats;

// In-memory NTFS Volume Context (Phase 1, 3, 4, 5, 6 & 7)
typedef struct {
    bool         mounted;
    BlockDevice* device;
    NTFS_BootSector bpb;

    uint32_t bytes_per_sector;
    uint32_t sectors_per_cluster;
    uint32_t bytes_per_cluster;

    uint64_t total_sectors;
    uint64_t total_clusters;
    uint64_t volume_size_bytes;

    uint64_t mft_lcn;
    uint64_t mft_byte_offset;

    uint64_t mft_mirr_lcn;
    uint64_t mft_mirr_byte_offset;

    uint32_t file_record_size;
    uint32_t index_buffer_size;

    uint64_t volume_serial_number;

    // Phase 3: Primary $MFT::$DATA Extent Map for MFT Record Resolution
    NTFS_ExtentMap mft_extent_map;

    // Phase 4: Volume Read Cache
    NTFS_ReadCache cache;

    // Phase 7B: MFT Record Cache
    NTFS_MFTCache mft_cache;

    // Phase 7E: Path Lookup Cache
    NTFS_PathCache path_cache;

    // Phase 7F: Performance Statistics
    NTFS_PerfStats stats;
} NTFS_VOLUME;

// On-disk NTFS FILE Record Header (48 bytes for NTFS 3.1+)
#pragma pack(push, 1)
typedef struct {
    char     magic[4];               // 0x00: "FILE" or "BAAD"
    uint16_t usa_offset;             // 0x04: Offset to Update Sequence Array
    uint16_t usa_count;              // 0x06: Size of USA in 2-byte words
    uint64_t lsn;                    // 0x08: Logfile Sequence Number
    uint16_t sequence_number;        // 0x10: Sequence number
    uint16_t hard_link_count;        // 0x12: Hard link count
    uint16_t first_attribute_offset; // 0x14: Offset to first attribute header
    uint16_t flags;                  // 0x16: Flags (0x0001 = InUse, 0x0002 = Directory)
    uint32_t bytes_in_use;           // 0x18: Real / used size of record
    uint32_t bytes_allocated;        // 0x1C: Allocated size of record
    uint64_t base_file_record;       // 0x20: Base file record reference
    uint16_t next_attribute_id;      // 0x28: Next attribute ID
    uint16_t align;                  // 0x2A: Alignment / reserved
    uint32_t record_number;          // 0x2C: MFT Record Number
} NTFS_FileRecordHeader;
#pragma pack(pop)

// FILE Record Flags
#define NTFS_FILE_IN_USE     0x0001
#define NTFS_FILE_DIRECTORY  0x0002

// Record Trust State Enums
typedef enum {
    NTFS_RECORD_STATE_RAW = 0,
    NTFS_RECORD_STATE_FIXUP_APPLIED,
    NTFS_RECORD_STATE_VALIDATED
} ntfs_record_state_t;

typedef enum {
    NTFS_RECORD_SRC_PRIMARY = 0,
    NTFS_RECORD_SRC_MIRROR
} ntfs_record_source_t;

// Validated In-Memory FILE Record Structure (Phase 2 Output)
typedef struct {
    ntfs_record_state_t  state;
    ntfs_record_source_t source;
    uint32_t             record_number;
    uint64_t             lba_offset;
    uint32_t             record_size;

    // Header Fields
    uint16_t usa_offset;
    uint16_t usa_count;
    uint64_t lsn;
    uint16_t sequence_number;
    uint16_t hard_link_count;
    uint16_t first_attribute_offset;
    uint16_t flags;
    uint32_t bytes_in_use;
    uint32_t bytes_allocated;
    uint64_t base_file_record;
    uint16_t next_attribute_id;

    // Owned Mutable Record Buffer (Fixups applied in-place)
    uint8_t* buffer;
} NTFS_FileRecord;

// ===========================================================================
// PHASE 3 — ATTRIBUTE ENGINE STRUCTURES & CONSTANTS
// ===========================================================================

#define NTFS_ATTR_STANDARD_INFORMATION 0x10
#define NTFS_ATTR_ATTRIBUTE_LIST       0x20
#define NTFS_ATTR_FILE_NAME            0x30
#define NTFS_ATTR_OBJECT_ID            0x40
#define NTFS_ATTR_SECURITY_DESCRIPTOR  0x50
#define NTFS_ATTR_VOLUME_NAME          0x60
#define NTFS_ATTR_VOLUME_INFORMATION   0x70
#define NTFS_ATTR_DATA                 0x80
#define NTFS_ATTR_INDEX_ROOT           0x90
#define NTFS_ATTR_INDEX_ALLOCATION     0xA0
#define NTFS_ATTR_BITMAP               0xB0
#define NTFS_ATTR_END                  0xFFFFFFFF

// Attribute Flags
#define NTFS_ATTR_FLAG_COMPRESSED      0x0001
#define NTFS_ATTR_FLAG_ENCRYPTED       0x4000
#define NTFS_ATTR_FLAG_SPARSE          0x8000

#pragma pack(push, 1)
typedef struct {
    uint32_t type;               // 0x00: Attribute type
    uint32_t length;             // 0x04: Total record length
    uint8_t  non_resident;       // 0x08: 0 = Resident, 1 = Non-resident
    uint8_t  name_length;        // 0x09: Name length in UTF-16 chars
    uint16_t name_offset;        // 0x0A: Offset to name
    uint16_t flags;              // 0x0C: Attribute flags
    uint16_t attribute_id;       // 0x0E: Attribute ID
} NTFS_AttributeHeader;

typedef struct {
    uint32_t value_length;       // 0x10: Length of attribute value
    uint16_t value_offset;       // 0x14: Offset to attribute value
    uint8_t  indexed_flag;       // 0x16: Indexed flag
    uint8_t  padding;            // 0x17: Padding
} NTFS_ResidentAttributeHeader;

typedef struct {
    uint64_t starting_vcn;           // 0x10: Starting VCN
    uint64_t last_vcn;               // 0x18: Last VCN (inclusive)
    uint16_t mapping_pairs_offset;   // 0x20: Offset to data runs
    uint16_t compression_unit;       // 0x22: Compression unit
    uint32_t padding;                // 0x24: Padding
    uint64_t allocated_size;         // 0x28: Allocated size on disk
    uint64_t data_size;              // 0x30: Real / data size
    uint64_t initialized_size;       // 0x38: Initialized size
} NTFS_NonResidentAttributeHeader;

typedef struct {
    uint32_t type;                 // 0x00: Attribute type
    uint16_t length;               // 0x04: Entry length
    uint8_t  name_length;          // 0x06: Name length
    uint8_t  name_offset;          // 0x07: Name offset
    uint64_t starting_vcn;         // 0x08: Starting VCN
    uint64_t base_file_reference;  // 0x10: Lower 48 bits = MFT record num
    uint16_t attribute_id;         // 0x18: Attribute ID
} NTFS_AttributeListEntry;
#pragma pack(pop)

// Parsed In-Memory Attribute Metadata View
typedef struct {
    uint32_t type;
    uint32_t length;
    bool     non_resident;
    uint16_t flags;
    uint16_t attribute_id;

    // Resident fields
    uint32_t resident_value_length;
    uint32_t resident_value_offset;

    // Non-resident fields
    uint64_t starting_vcn;
    uint64_t last_vcn;
    uint32_t mapping_pairs_offset;
    uint64_t allocated_size;
    uint64_t data_size;
    uint64_t initialized_size;

    // Pointer into record buffer
    const uint8_t* raw_attr_ptr;
} NTFS_Attribute;

// ===========================================================================
// PHASE 4 — FILE READ ENGINE STRUCTURES
// ===========================================================================

typedef struct {
    NTFS_VOLUME*     vol;
    uint32_t         record_number;
    NTFS_FileRecord* record;

    bool             is_directory;
    bool             has_data;
    bool             non_resident;
    bool             is_compressed;
    bool             is_encrypted;

    uint64_t         data_size;        // Authoritative logical file size
    uint64_t         allocated_size;
    uint64_t         initialized_size;

    // Resident stream payload
    const uint8_t*   resident_data;
    uint32_t         resident_len;

    // Non-resident stream extent map
    NTFS_ExtentMap   extent_map;

    // Phase 7D: Sequential Read-Ahead State
    uint64_t         last_read_offset;
    uint32_t         sequential_read_count;
} NTFS_File;

// ===========================================================================
// PHASE 5 — DIRECTORY & INDEX ENGINE STRUCTURES
// ===========================================================================

#define NTFS_ROOT_RECORD_NUM           5
#define NTFS_INDEX_ENTRY_HAS_SUBNODE   0x0001
#define NTFS_INDEX_ENTRY_LAST          0x0002

#pragma pack(push, 1)
typedef struct {
    uint32_t attribute_type;     // 0x00: Attribute type indexed (0x30 for $FILE_NAME)
    uint32_t collation_rule;     // 0x04: Collation rule
    uint32_t index_buffer_size;  // 0x08: Bytes per index block
    uint8_t  clusters_per_block; // 0x0C: Clusters per index block
    uint8_t  padding[3];         // 0x0D: Alignment
} NTFS_IndexRootHeader;

typedef struct {
    uint32_t entries_offset;     // 0x00: Offset to first entry relative to index header start
    uint32_t total_size;         // 0x04: Total byte size of entries including header
    uint32_t allocated_size;     // 0x08: Allocated byte size of entries
    uint8_t  flags;              // 0x0C: 0x01 = Has Large Index ($INDEX_ALLOCATION present)
    uint8_t  padding[3];
} NTFS_IndexHeader;

typedef struct {
    uint64_t file_reference;     // 0x00: Target MFT file ref (lower 48 bits = rec_num, upper 16 bits = seq)
    uint16_t length;             // 0x08: Total size of entry
    uint16_t key_length;         // 0x0A: Key size ($FILE_NAME attribute size)
    uint16_t flags;              // 0x0C: Flags (0x01 = Has Child VCN, 0x02 = End Entry)
    uint16_t reserved;           // 0x0E: Alignment
} NTFS_IndexEntry;

typedef struct {
    uint64_t parent_directory;   // 0x00: Parent MFT reference
    uint64_t creation_time;      // 0x08
    uint64_t modification_time;  // 0x10
    uint64_t mft_change_time;    // 0x18
    uint64_t access_time;        // 0x20
    uint64_t allocated_size;     // 0x28
    uint64_t real_size;          // 0x30
    uint32_t file_flags;         // 0x38
    uint32_t reparse_tag;        // 0x3C
    uint8_t  filename_len;       // 0x40: Name length in UTF-16 characters
    uint8_t  namespace;          // 0x41: 0=POSIX, 1=Win32, 2=DOS, 3=Win32+DOS
    uint16_t filename[1];        // 0x42: UTF-16 character array
} NTFS_FileNameAttr;

typedef struct {
    char     magic[4];           // 0x00: "INDX"
    uint16_t usa_offset;         // 0x04: USA array offset
    uint16_t usa_count;          // 0x06: USA array count
    uint64_t lsn;                // 0x08: Logfile sequence number
    uint64_t index_block_vcn;    // 0x10: VCN of this index block
    NTFS_IndexHeader index_hdr;  // 0x18: Index header
} NTFS_IndexBlockHeader;
#pragma pack(pop)

typedef struct {
    char     name[256];
    uint32_t record_number;
    uint16_t sequence_number;
    bool     is_directory;
    uint64_t file_size;
    uint8_t  name_space;
} NTFS_DirEntry;

// Phase 1 NTFS API
void      ntfs_init(void);
VFS_Node* ntfs_mount(BlockDevice* device);
int       ntfs_unmount(VFS_Node* mount_node);

// Core Phase 1 Validation & Geometry Helpers
bool ntfs_validate_bpb(const NTFS_BootSector* bpb, uint64_t device_sector_count, const char** out_err_reason);
bool ntfs_decode_record_size(int8_t encoded, uint32_t bytes_per_cluster, uint32_t bytes_per_sector, uint32_t* out_size);
void ntfs_dump_diagnostics(const NTFS_VOLUME* vol, const char* stage, bool success, const char* failure_reason);

// Phase 2 MFT Core Engine API
NTFS_FileRecord* ntfs_mft_read_record(NTFS_VOLUME* vol, uint32_t record_number);
void             ntfs_mft_free_record(NTFS_FileRecord* record);
bool             ntfs_mft_apply_fixup(uint8_t* buffer, uint32_t record_size, uint32_t bytes_per_sector, const char** out_err);
bool             ntfs_mft_validate_record(const uint8_t* buffer, uint32_t record_size, const char** out_err);
void             ntfs_mft_dump_diagnostics(const NTFS_FileRecord* record, const char* stage, bool success, const char* err_reason);

// Phase 3 Attribute Engine API
bool ntfs_attr_find(const NTFS_FileRecord* rec, uint32_t attr_type, const char* name, NTFS_Attribute* out_attr);
bool ntfs_attr_get_resident_value(const NTFS_FileRecord* rec, const NTFS_Attribute* attr, const void** out_data, uint32_t* out_len);
bool ntfs_decode_data_runs(const uint8_t* runlist, uint32_t runlist_len, uint64_t starting_vcn, NTFS_ExtentMap* out_map, const char** out_err);
bool ntfs_extent_map_lookup(const NTFS_ExtentMap* map, uint64_t vcn, NTFS_Extent* out_extent);
void ntfs_extent_map_free(NTFS_ExtentMap* map);
bool ntfs_bootstrap_mft_extent_map(NTFS_VOLUME* vol);

// Phase 4 File Read Engine API
NTFS_File* ntfs_file_open_by_record(NTFS_VOLUME* vol, uint32_t record_number);
void       ntfs_file_close(NTFS_File* file);
int64_t    ntfs_file_read(NTFS_File* file, uint64_t offset, void* buffer, uint64_t len);

void       ntfs_cache_init(NTFS_ReadCache* cache);
void       ntfs_cache_flush(NTFS_ReadCache* cache);
bool       ntfs_read_sector_cached(NTFS_VOLUME* vol, uint64_t lba, void* buffer);

// Phase 5 Directory & Index Engine API
bool       ntfs_dir_lookup_entry(NTFS_VOLUME* vol, const NTFS_FileRecord* dir_rec, const char* name, uint64_t* out_file_ref);
bool       ntfs_dir_enum(NTFS_VOLUME* vol, const NTFS_FileRecord* dir_rec, NTFS_DirEntry** out_entries, uint32_t* out_count);
bool       ntfs_resolve_path(NTFS_VOLUME* vol, const char* path, uint32_t* out_record_num);
NTFS_File* ntfs_open_file_by_path(NTFS_VOLUME* vol, const char* path);

// Phase 7 Cache & Performance Diagnostics API
void       ntfs_mft_cache_init(NTFS_MFTCache* cache);
void       ntfs_mft_cache_flush(NTFS_MFTCache* cache);
void       ntfs_path_cache_init(NTFS_PathCache* cache);
void       ntfs_path_cache_flush(NTFS_PathCache* cache);
void       ntfs_dump_performance_stats(const NTFS_VOLUME* vol);

// Phase 6 Production VFS Driver API & Callbacks
extern FilesystemDriver ntfs_fs_driver;

// Diagnostic & Certification Test Suite
void ntfs_run_tests(void);

#endif // NTFS_H
