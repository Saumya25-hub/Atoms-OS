#ifndef BOFS_VALIDATOR_H
#define BOFS_VALIDATOR_H

#include "bofs_format.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Return Error Codes */
#define BOFS_VALID_OK                 0
#define BOFS_ERR_INVALID_MAGIC       -1
#define BOFS_ERR_UNSUPPORTED_VERSION -2
#define BOFS_ERR_UNSUPPORTED_FEATURE -3
#define BOFS_ERR_CHECKSUM_MISMATCH   -4
#define BOFS_ERR_GEOMETRY_OVERLAP    -5
#define BOFS_ERR_INTEGER_OVERFLOW    -6
#define BOFS_ERR_IMAGE_TRUNCATED     -7
#define BOFS_ERR_OUT_OF_BOUNDS       -8
#define BOFS_ERR_INVALID_ENTRY_COUNT -9
#define BOFS_ERR_CORRUPTED_INODE     -10
#define BOFS_ERR_CORRUPTED_DIR_NODE  -11
#define BOFS_ERR_CORRUPTED_JOURNAL   -12

/* Hash & CRC Functions */
uint32_t bofs_crc32(const void* data, size_t length);
uint64_t bofs_hash(const char* name, uint8_t len);

/* Geometry Calculation & Validation */
int bofs_calc_geometry(uint64_t total_sectors, uint32_t sector_size, bool compact_mode, bofs_superblock_t* out_sb);
int bofs_validate_geometry(const bofs_superblock_t* sb, uint64_t volume_block_capacity);

/* Core Structure Validation */
int bofs_validate_superblock(const bofs_superblock_t* sb, uint64_t volume_block_capacity);
int bofs_validate_inode(const bofs_inode_t* inode, uint64_t expected_inode_num);
int bofs_validate_dir_node(const bofs_dir_node_t* node);
int bofs_validate_journal_header(const bofs_journal_header_t* jh);
int bofs_validate_journal_desc(const bofs_journal_desc_t* desc);
int bofs_validate_journal_commit(const bofs_journal_commit_t* commit);

/* Serialization Helpers */
void bofs_init_superblock(bofs_superblock_t* sb, const uint8_t uuid[16], const char* label);
void bofs_init_root_inode(bofs_inode_t* inode);
void bofs_init_journal_header(bofs_journal_header_t* jh, uint64_t total_journal_blocks);
void bofs_init_dir_node(bofs_dir_node_t* node, uint16_t node_type, uint32_t level);

#ifdef __cplusplus
}
#endif

#endif /* BOFS_VALIDATOR_H */
