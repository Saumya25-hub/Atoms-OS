#ifndef BOFS_WAL_H
#define BOFS_WAL_H

#include "bofs_format.h"
#include "kernel/vfs/vfs_legacy/storage/include/block_device.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Return & Error Codes
 * -------------------------------------------------------------------------- */
#define BOFS_WAL_OK                     0
#define BOFS_ERR_WAL_INVALID_PARAM     -1   /* NULL or invalid parameter */
#define BOFS_ERR_WAL_IO                -2   /* BlockDevice read/write/flush failure */
#define BOFS_ERR_WAL_CORRUPT_HEADER    -3   /* Journal header magic/CRC invalid */
#define BOFS_ERR_WAL_CORRUPT_RECORD    -4   /* Journal record structure corrupted */
#define BOFS_ERR_WAL_CRC_MISMATCH      -5   /* CRC32 integrity check failed */
#define BOFS_ERR_WAL_TXN_TOO_LARGE     -6   /* Transaction block limit exceeded */
#define BOFS_ERR_WAL_JOURNAL_FULL      -7   /* Journal ring capacity exhausted */
#define BOFS_ERR_WAL_NOT_ACTIVE        -8   /* No active transaction in context */
#define BOFS_ERR_WAL_ALREADY_COMMITTED -9   /* Transaction already committed */
#define BOFS_ERR_WAL_ABORTED           -10  /* Transaction was aborted */
#define BOFS_ERR_WAL_REPLAY_FAILED     -11  /* Error applying replayed blocks */

/* Maximum in-memory metadata blocks buffered per single transaction */
#define BOFS_WAL_MAX_RECORD_BLOCKS      32U

/* Transaction State Flags */
typedef enum {
    BOFS_TX_STATE_INACTIVE = 0,
    BOFS_TX_STATE_ACTIVE,
    BOFS_TX_STATE_COMMITTED,
    BOFS_TX_STATE_ABORTED,
    BOFS_TX_STATE_APPLIED
} bofs_tx_state_t;

/* Single Buffered Transaction Metadata Block */
typedef struct {
    uint64_t target_block;
    uint8_t  data[BOFS_BLOCK_SIZE] __attribute__((aligned(64)));
} bofs_tx_block_buf_t;

/* In-Memory Transaction Context */
typedef struct {
    uint64_t        tx_id;
    uint64_t        seq_num;
    uint32_t        block_count;
    bofs_tx_state_t state;
    bofs_tx_block_buf_t recorded_blocks[BOFS_WAL_MAX_RECORD_BLOCKS];
} bofs_tx_t;

/* Mount-Time Recovery Statistics */
typedef struct {
    uint64_t total_scanned_records;
    uint64_t committed_txns_replayed;
    uint64_t incomplete_txns_discarded;
    uint64_t corrupted_records_detected;
    uint64_t total_blocks_replayed;
} bofs_wal_recovery_stats_t;

/* --------------------------------------------------------------------------
 * Write-Ahead Logging (WAL) Engine Context
 * -------------------------------------------------------------------------- */
typedef struct {
    BlockDevice*           dev;
    bofs_superblock_t      sb;
    bofs_journal_header_t  jh;
    uint64_t               journal_start_lba_base;
    uint32_t               spb; /* Sectors per 4KB Block */
    bool                   mounted;
    bofs_tx_t              active_tx;
} bofs_wal_t;

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */
int      bofs_wal_init(bofs_wal_t* wal, BlockDevice* dev, const bofs_superblock_t* sb);
int      bofs_wal_format(BlockDevice* dev, const bofs_superblock_t* sb);
int      bofs_wal_mount(bofs_wal_t* wal, BlockDevice* dev, const bofs_superblock_t* sb, bofs_wal_recovery_stats_t* stats);
int      bofs_wal_recover(bofs_wal_t* wal, bofs_wal_recovery_stats_t* stats);

int      bofs_tx_begin(bofs_wal_t* wal, bofs_tx_t** out_tx);
int      bofs_tx_record_block(bofs_wal_t* wal, bofs_tx_t* tx, uint64_t target_block, const void* block_data);
int      bofs_tx_commit(bofs_wal_t* wal, bofs_tx_t* tx);
int      bofs_tx_abort(bofs_wal_t* wal, bofs_tx_t* tx);

int      bofs_wal_checkpoint(bofs_wal_t* wal);
uint64_t bofs_wal_free_blocks(const bofs_wal_t* wal);

#ifdef __cplusplus
}
#endif

#endif /* BOFS_WAL_H */
