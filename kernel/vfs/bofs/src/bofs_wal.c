#include "kernel/vfs/bofs/include/bofs_wal.h"
#include "kernel/vfs/bofs/include/bofs_validator.h"
#include "kernel/core/lib/include/string.h"

/* --------------------------------------------------------------------------
 * Internal Helper: Block to LBA Translation
 * -------------------------------------------------------------------------- */
static inline bool bofs_wal_block_to_lba(const bofs_wal_t* wal, uint64_t rel_block_idx, uint64_t* out_lba) {
    if (!wal || !wal->dev || wal->spb == 0) return false;
    uint64_t abs_block = wal->sb.journal_start_block + rel_block_idx;
    if (abs_block > (UINT64_MAX / wal->spb)) return false;
    *out_lba = abs_block * (uint64_t)wal->spb;
    return true;
}

/* --------------------------------------------------------------------------
 * Internal Helper: Ring Pointer Wraparound
 * Journal Header is at relative block 0.
 * Usable ring blocks are in range [1, total_blocks - 1].
 * -------------------------------------------------------------------------- */
static inline uint64_t bofs_wal_next_ring_block(uint64_t blk, uint64_t total_blocks) {
    blk++;
    if (blk >= total_blocks) {
        blk = 1;
    }
    return blk;
}

/* --------------------------------------------------------------------------
 * Public API: bofs_wal_free_blocks
 * -------------------------------------------------------------------------- */
uint64_t bofs_wal_free_blocks(const bofs_wal_t* wal) {
    if (!wal || wal->jh.total_blocks <= 2) return 0;
    uint64_t total_ring = wal->jh.total_blocks - 1; /* Excludes relative block 0 (Header) */
    uint64_t head = wal->jh.head_block;
    uint64_t tail = wal->jh.tail_block;

    if (head < 1 || head >= wal->jh.total_blocks) head = 1;
    if (tail < 1 || tail >= wal->jh.total_blocks) tail = 1;

    uint64_t used;
    if (tail >= head) {
        used = tail - head;
    } else {
        used = total_ring - (head - tail);
    }

    /* Keep 1 guard block to distinguish full ring from empty ring */
    if (used + 1 >= total_ring) return 0;
    return total_ring - used - 1;
}

/* --------------------------------------------------------------------------
 * Public API: bofs_wal_format
 * -------------------------------------------------------------------------- */
int bofs_wal_format(BlockDevice* dev, const bofs_superblock_t* sb) {
    if (!dev || !sb) return BOFS_ERR_WAL_INVALID_PARAM;
    if (dev->sector_size == 0) return BOFS_ERR_WAL_INVALID_PARAM;
    if (dev->read_only) return BOFS_ERR_WAL_IO;

    uint32_t spb = BOFS_BLOCK_SIZE / (uint32_t)dev->sector_size;
    if (spb == 0) return BOFS_ERR_WAL_INVALID_PARAM;

    bofs_journal_header_t jh;
    memset(&jh, 0, sizeof(bofs_journal_header_t));
    jh.magic = BOFS_JOURNAL_MAGIC;
    jh.version = 1;
    jh.flags = 0;
    jh.block_size = BOFS_BLOCK_SIZE;
    jh.total_blocks = sb->journal_block_count;
    jh.head_block = 1;
    jh.tail_block = 1;
    jh.sequence_number = 1;
    jh.last_commit_seq = 0;
    jh.checksum = bofs_crc32(&jh, offsetof(bofs_journal_header_t, checksum));

    uint64_t lba = sb->journal_start_block * spb;
    if (!dev->write(dev, lba, spb, &jh)) {
        return BOFS_ERR_WAL_IO;
    }

    if (dev->flush && !dev->flush(dev)) {
        return BOFS_ERR_WAL_IO;
    }

    return BOFS_WAL_OK;
}

/* --------------------------------------------------------------------------
 * Public API: bofs_wal_init
 * -------------------------------------------------------------------------- */
int bofs_wal_init(bofs_wal_t* wal, BlockDevice* dev, const bofs_superblock_t* sb) {
    if (!wal || !dev || !sb) return BOFS_ERR_WAL_INVALID_PARAM;
    if (dev->sector_size == 0) return BOFS_ERR_WAL_INVALID_PARAM;

    memset(wal, 0, sizeof(bofs_wal_t));
    wal->dev = dev;
    wal->sb = *sb;
    wal->spb = BOFS_BLOCK_SIZE / (uint32_t)dev->sector_size;
    wal->journal_start_lba_base = sb->journal_start_block * wal->spb;
    wal->mounted = false;
    wal->active_tx.state = BOFS_TX_STATE_INACTIVE;

    return BOFS_WAL_OK;
}

/* --------------------------------------------------------------------------
 * Public API: bofs_wal_mount
 * -------------------------------------------------------------------------- */
int bofs_wal_mount(bofs_wal_t* wal, BlockDevice* dev, const bofs_superblock_t* sb, bofs_wal_recovery_stats_t* stats) {
    int init_res = bofs_wal_init(wal, dev, sb);
    if (init_res != BOFS_WAL_OK) return init_res;

    /* Read Journal Header at relative block 0 */
    uint64_t lba;
    if (!bofs_wal_block_to_lba(wal, 0, &lba)) return BOFS_ERR_WAL_IO;

    if (!wal->dev->read(wal->dev, lba, wal->spb, &wal->jh)) {
        return BOFS_ERR_WAL_IO;
    }

    /* Validate Header */
    int val_res = bofs_validate_journal_header(&wal->jh);
    if (val_res != BOFS_VALID_OK) {
        return BOFS_ERR_WAL_CORRUPT_HEADER;
    }

    /* Ring Bounds Check */
    if (wal->jh.head_block == 0 || wal->jh.head_block >= wal->jh.total_blocks ||
        wal->jh.tail_block == 0 || wal->jh.tail_block >= wal->jh.total_blocks) {
        return BOFS_ERR_WAL_CORRUPT_HEADER;
    }

    /* Run mount-time recovery if there are uncheckpointed transactions */
    int rec_res = bofs_wal_recover(wal, stats);
    if (rec_res != BOFS_WAL_OK) {
        return rec_res;
    }

    wal->mounted = true;
    return BOFS_WAL_OK;
}

/* --------------------------------------------------------------------------
 * Public API: bofs_wal_recover
 * Deterministic, idempotent recovery scanner and replayer.
 * -------------------------------------------------------------------------- */
int bofs_wal_recover(bofs_wal_t* wal, bofs_wal_recovery_stats_t* stats) {
    if (!wal || !wal->dev) return BOFS_ERR_WAL_INVALID_PARAM;

    if (stats) {
        memset(stats, 0, sizeof(bofs_wal_recovery_stats_t));
    }

    /* Re-read and validate journal header from disk */
    uint64_t hdr_lba;
    if (!bofs_wal_block_to_lba(wal, 0, &hdr_lba)) return BOFS_ERR_WAL_IO;
    if (!wal->dev->read(wal->dev, hdr_lba, wal->spb, &wal->jh)) {
        return BOFS_ERR_WAL_IO;
    }
    if (bofs_validate_journal_header(&wal->jh) != BOFS_VALID_OK) {
        return BOFS_ERR_WAL_CORRUPT_HEADER;
    }

    uint64_t curr = wal->jh.head_block;
    uint64_t limit = wal->jh.tail_block;

    /* Empty journal ring -> nothing to recover */
    if (curr == limit) {
        return BOFS_WAL_OK;
    }

    static bofs_journal_desc_t   s_rec_desc __attribute__((aligned(64)));
    static bofs_journal_commit_t s_rec_commit __attribute__((aligned(64)));
    static uint8_t               s_rec_payload[BOFS_BLOCK_SIZE] __attribute__((aligned(64)));

    bool replayed_any = false;
    uint64_t last_replayed_seq = wal->jh.last_commit_seq;

    for (uint64_t step = 0; step < wal->jh.total_blocks; step++) {
        /* 1. Read record at current ring cursor */
        uint64_t desc_lba;
        if (!bofs_wal_block_to_lba(wal, curr, &desc_lba)) break;
        if (!wal->dev->read(wal->dev, desc_lba, wal->spb, &s_rec_desc)) break;

        /* If block does not begin with BTXN magic, end of active transactions reached */
        if (s_rec_desc.magic != BOFS_TXN_DESC_MAGIC) break;

        /* If CRC invalid or corrupted descriptor, record corruption and halt scan */
        if (bofs_validate_journal_desc(&s_rec_desc) != BOFS_VALID_OK ||
            s_rec_desc.block_count == 0 ||
            s_rec_desc.block_count > BOFS_WAL_MAX_RECORD_BLOCKS) {
            if (stats) stats->corrupted_records_detected++;
            break;
        }

        /* If sequence number is not newer than last checkpointed, stale record from older cycle */
        if (s_rec_desc.sequence_number <= last_replayed_seq) break;

        if (stats) stats->total_scanned_records++;

        /* 2. Locate and inspect corresponding commit record */
        uint64_t cmt_cursor = curr;
        for (uint32_t i = 0; i <= s_rec_desc.block_count; i++) {
            cmt_cursor = bofs_wal_next_ring_block(cmt_cursor, wal->jh.total_blocks);
        }

        uint64_t cmt_lba;
        if (!bofs_wal_block_to_lba(wal, cmt_cursor, &cmt_lba)) break;
        if (!wal->dev->read(wal->dev, cmt_lba, wal->spb, &s_rec_commit)) break;

        /* 3. Validate commit record */
        if (s_rec_commit.magic != BOFS_TXN_COMMIT_MAGIC ||
            bofs_validate_journal_commit(&s_rec_commit) != BOFS_VALID_OK ||
            s_rec_commit.transaction_id != s_rec_desc.transaction_id ||
            s_rec_commit.sequence_number != s_rec_desc.sequence_number) {
            /* Transaction is INCOMPLETE (crashed before commit was durable) */
            if (stats) stats->incomplete_txns_discarded++;
            break;
        }

        /* 4. Transaction is COMMITTED: Replay all attached metadata blocks */
        uint64_t attached_cursor = curr;
        for (uint32_t i = 0; i < s_rec_desc.block_count; i++) {
            attached_cursor = bofs_wal_next_ring_block(attached_cursor, wal->jh.total_blocks);

            uint64_t payload_lba;
            if (!bofs_wal_block_to_lba(wal, attached_cursor, &payload_lba)) {
                return BOFS_ERR_WAL_IO;
            }
            if (!wal->dev->read(wal->dev, payload_lba, wal->spb, s_rec_payload)) {
                return BOFS_ERR_WAL_IO;
            }

            /* Apply replayed block directly to target physical filesystem block */
            uint64_t target_lba = s_rec_desc.target_blocks[i] * (uint64_t)wal->spb;
            if (!wal->dev->write(wal->dev, target_lba, wal->spb, s_rec_payload)) {
                return BOFS_ERR_WAL_REPLAY_FAILED;
            }

            if (stats) stats->total_blocks_replayed++;
        }

        replayed_any = true;
        if (stats) stats->committed_txns_replayed++;
        last_replayed_seq = s_rec_desc.sequence_number;

        /* Advance cursor past commit record */
        curr = bofs_wal_next_ring_block(cmt_cursor, wal->jh.total_blocks);
    }

    /* Ensure all replayed blocks are durable */
    if (replayed_any && wal->dev->flush) {
        if (!wal->dev->flush(wal->dev)) {
            return BOFS_ERR_WAL_IO;
        }
    }

    /* Advance journal head and tail to mark recovered transactions as checkpointed */
    wal->jh.head_block = curr;
    wal->jh.tail_block = curr;
    wal->jh.last_commit_seq = last_replayed_seq;
    wal->jh.checksum = bofs_crc32(&wal->jh, offsetof(bofs_journal_header_t, checksum));

    if (!wal->dev->write(wal->dev, hdr_lba, wal->spb, &wal->jh)) {
        return BOFS_ERR_WAL_IO;
    }
    if (wal->dev->flush && !wal->dev->flush(wal->dev)) {
        return BOFS_ERR_WAL_IO;
    }

    return BOFS_WAL_OK;
}

/* --------------------------------------------------------------------------
 * Public API: bofs_tx_begin
 * -------------------------------------------------------------------------- */
int bofs_tx_begin(bofs_wal_t* wal, bofs_tx_t** out_tx) {
    if (!wal || !out_tx) return BOFS_ERR_WAL_INVALID_PARAM;
    if (wal->active_tx.state == BOFS_TX_STATE_ACTIVE) {
        return BOFS_ERR_WAL_ALREADY_COMMITTED;
    }

    bofs_tx_t* tx = &wal->active_tx;
    memset(tx, 0, sizeof(bofs_tx_t));
    tx->tx_id = wal->jh.sequence_number;
    tx->seq_num = wal->jh.sequence_number;
    wal->jh.sequence_number++;
    tx->block_count = 0;
    tx->state = BOFS_TX_STATE_ACTIVE;

    *out_tx = tx;
    return BOFS_WAL_OK;
}

/* --------------------------------------------------------------------------
 * Public API: bofs_tx_record_block
 * -------------------------------------------------------------------------- */
int bofs_tx_record_block(bofs_wal_t* wal, bofs_tx_t* tx, uint64_t target_block, const void* block_data) {
    if (!wal || !tx || !block_data) return BOFS_ERR_WAL_INVALID_PARAM;
    if (tx->state != BOFS_TX_STATE_ACTIVE) return BOFS_ERR_WAL_NOT_ACTIVE;

    /* Check if target_block was already recorded in this transaction (coalesce) */
    for (uint32_t i = 0; i < tx->block_count; i++) {
        if (tx->recorded_blocks[i].target_block == target_block) {
            memcpy(tx->recorded_blocks[i].data, block_data, BOFS_BLOCK_SIZE);
            return BOFS_WAL_OK;
        }
    }

    if (tx->block_count >= BOFS_WAL_MAX_RECORD_BLOCKS) {
        return BOFS_ERR_WAL_TXN_TOO_LARGE;
    }

    tx->recorded_blocks[tx->block_count].target_block = target_block;
    memcpy(tx->recorded_blocks[tx->block_count].data, block_data, BOFS_BLOCK_SIZE);
    tx->block_count++;

    return BOFS_WAL_OK;
}

/* --------------------------------------------------------------------------
 * Public API: bofs_tx_abort
 * -------------------------------------------------------------------------- */
int bofs_tx_abort(bofs_wal_t* wal, bofs_tx_t* tx) {
    if (!wal || !tx) return BOFS_ERR_WAL_INVALID_PARAM;
    tx->state = BOFS_TX_STATE_ABORTED;
    tx->block_count = 0;
    return BOFS_WAL_OK;
}

/* --------------------------------------------------------------------------
 * Public API: bofs_tx_commit
 * -------------------------------------------------------------------------- */
int bofs_tx_commit(bofs_wal_t* wal, bofs_tx_t* tx) {
    if (!wal || !tx || !wal->dev) return BOFS_ERR_WAL_INVALID_PARAM;
    if (tx->state != BOFS_TX_STATE_ACTIVE) return BOFS_ERR_WAL_NOT_ACTIVE;

    /* Zero-block transaction is a no-op commit */
    if (tx->block_count == 0) {
        tx->state = BOFS_TX_STATE_COMMITTED;
        return BOFS_WAL_OK;
    }

    /* Check journal capacity: 1 descriptor + N attached blocks + 1 commit record */
    uint64_t needed_blocks = (uint64_t)tx->block_count + 2;
    if (bofs_wal_free_blocks(wal) < needed_blocks) {
        return BOFS_ERR_WAL_JOURNAL_FULL;
    }

    /* 1. Build Journal Descriptor */
    static bofs_journal_desc_t s_desc __attribute__((aligned(64)));
    memset(&s_desc, 0, sizeof(bofs_journal_desc_t));
    s_desc.magic = BOFS_TXN_DESC_MAGIC;
    s_desc.record_type = 1;
    s_desc.transaction_id = tx->tx_id;
    s_desc.sequence_number = tx->seq_num;
    s_desc.block_count = tx->block_count;
    s_desc.flags = 0;

    for (uint32_t i = 0; i < tx->block_count; i++) {
        s_desc.target_blocks[i] = tx->recorded_blocks[i].target_block;
    }
    s_desc.checksum = bofs_crc32(&s_desc, offsetof(bofs_journal_desc_t, checksum));

    /* Write descriptor to journal ring */
    uint64_t desc_rel_block = wal->jh.tail_block;
    uint64_t desc_lba;
    if (!bofs_wal_block_to_lba(wal, desc_rel_block, &desc_lba)) return BOFS_ERR_WAL_IO;
    if (!wal->dev->write(wal->dev, desc_lba, wal->spb, &s_desc)) {
        return BOFS_ERR_WAL_IO;
    }

    uint64_t next_tail = bofs_wal_next_ring_block(desc_rel_block, wal->jh.total_blocks);

    /* 2. Write attached metadata blocks to journal ring */
    for (uint32_t i = 0; i < tx->block_count; i++) {
        uint64_t blk_lba;
        if (!bofs_wal_block_to_lba(wal, next_tail, &blk_lba)) return BOFS_ERR_WAL_IO;
        if (!wal->dev->write(wal->dev, blk_lba, wal->spb, tx->recorded_blocks[i].data)) {
            return BOFS_ERR_WAL_IO;
        }
        next_tail = bofs_wal_next_ring_block(next_tail, wal->jh.total_blocks);
    }

    /* Flush journal payload blocks before commit record */
    if (wal->dev->flush && !wal->dev->flush(wal->dev)) {
        return BOFS_ERR_WAL_IO;
    }

    /* 3. Build and write Journal Commit Record */
    static bofs_journal_commit_t s_commit __attribute__((aligned(64)));
    memset(&s_commit, 0, sizeof(bofs_journal_commit_t));
    s_commit.magic = BOFS_TXN_COMMIT_MAGIC;
    s_commit.record_type = 2;
    s_commit.transaction_id = tx->tx_id;
    s_commit.sequence_number = tx->seq_num;
    s_commit.commit_timestamp = 0;
    s_commit.checksum = bofs_crc32(&s_commit, offsetof(bofs_journal_commit_t, checksum));

    uint64_t cmt_lba;
    if (!bofs_wal_block_to_lba(wal, next_tail, &cmt_lba)) return BOFS_ERR_WAL_IO;
    if (!wal->dev->write(wal->dev, cmt_lba, wal->spb, &s_commit)) {
        return BOFS_ERR_WAL_IO;
    }

    next_tail = bofs_wal_next_ring_block(next_tail, wal->jh.total_blocks);

    /* 4. FLUSH COMMIT RECORD: AUTHORITATIVE COMMIT POINT */
    if (wal->dev->flush && !wal->dev->flush(wal->dev)) {
        return BOFS_ERR_WAL_IO;
    }

    /* The transaction is officially durable and committed! */
    tx->state = BOFS_TX_STATE_COMMITTED;
    wal->jh.tail_block = next_tail;
    wal->jh.last_commit_seq = tx->seq_num;

    /* 5. Apply metadata blocks to target physical filesystem blocks */
    for (uint32_t i = 0; i < tx->block_count; i++) {
        uint64_t target_lba = tx->recorded_blocks[i].target_block * (uint64_t)wal->spb;
        if (!wal->dev->write(wal->dev, target_lba, wal->spb, tx->recorded_blocks[i].data)) {
            return BOFS_ERR_WAL_IO;
        }
    }

    /* 6. Flush applied filesystem metadata */
    if (wal->dev->flush && !wal->dev->flush(wal->dev)) {
        return BOFS_ERR_WAL_IO;
    }

    /* 7. Checkpoint: Advance head to mark transaction applied and reclaim ring */
    wal->jh.head_block = wal->jh.tail_block;
    wal->jh.checksum = bofs_crc32(&wal->jh, offsetof(bofs_journal_header_t, checksum));

    uint64_t hdr_lba;
    if (!bofs_wal_block_to_lba(wal, 0, &hdr_lba)) return BOFS_ERR_WAL_IO;
    if (!wal->dev->write(wal->dev, hdr_lba, wal->spb, &wal->jh)) {
        return BOFS_ERR_WAL_IO;
    }

    if (wal->dev->flush && !wal->dev->flush(wal->dev)) {
        return BOFS_ERR_WAL_IO;
    }

    tx->state = BOFS_TX_STATE_APPLIED;
    return BOFS_WAL_OK;
}

/* --------------------------------------------------------------------------
 * Public API: bofs_wal_checkpoint
 * -------------------------------------------------------------------------- */
int bofs_wal_checkpoint(bofs_wal_t* wal) {
    if (!wal || !wal->dev) return BOFS_ERR_WAL_INVALID_PARAM;

    wal->jh.head_block = wal->jh.tail_block;
    wal->jh.checksum = bofs_crc32(&wal->jh, offsetof(bofs_journal_header_t, checksum));

    uint64_t hdr_lba;
    if (!bofs_wal_block_to_lba(wal, 0, &hdr_lba)) return BOFS_ERR_WAL_IO;
    if (!wal->dev->write(wal->dev, hdr_lba, wal->spb, &wal->jh)) {
        return BOFS_ERR_WAL_IO;
    }

    if (wal->dev->flush && !wal->dev->flush(wal->dev)) {
        return BOFS_ERR_WAL_IO;
    }

    return BOFS_WAL_OK;
}
