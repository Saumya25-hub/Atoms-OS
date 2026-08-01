#include "../include/botree_tx.h"
#include "../include/botree_path.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/core/lib/include/string.h"

#define MAX_TRANSACTIONS 32

static BDeTxProgress s_transactions[MAX_TRANSACTIONS];
static uint64_t s_next_tx_id = 41; // Start Transaction IDs at #41

static BDeTxProgress* find_tx(BDeTxHandle tx_id) {
    for (int i = 0; i < MAX_TRANSACTIONS; i++) {
        if (s_transactions[i].tx_id == tx_id) return &s_transactions[i];
    }
    return NULL;
}

static BDeTxProgress* alloc_tx(BDeTxOpType op_type, const char* src, const char* dest) {
    for (int i = 0; i < MAX_TRANSACTIONS; i++) {
        if (s_transactions[i].tx_id == 0 || s_transactions[i].status == BDE_TX_STATUS_COMPLETED || s_transactions[i].status == BDE_TX_STATUS_FAILED) {
            memset(&s_transactions[i], 0, sizeof(BDeTxProgress));
            s_transactions[i].tx_id = s_next_tx_id++;
            s_transactions[i].op_type = op_type;
            s_transactions[i].status = BDE_TX_STATUS_RUNNING;
            s_transactions[i].total_files = 1;
            s_transactions[i].processed_files = 0;
            s_transactions[i].progress_percent = 0;
            if (src) strcpy(s_transactions[i].current_source, src);
            if (dest) strcpy(s_transactions[i].current_target, dest);
            return &s_transactions[i];
        }
    }
    return NULL;
}

BDeTxHandle BDe_TransactionCopy(const char* src_path, const char* dest_dir) {
    if (!src_path || !dest_dir) return 0;
    BDeTxProgress* tx = alloc_tx(BDE_TX_OP_COPY, src_path, dest_dir);
    if (!tx) return 0;

    char basename[BDE_NAME_MAX];
    BDe_PathGetBasename(src_path, basename, BDE_NAME_MAX);
    char full_dest[BDE_PATH_MAX];
    BDe_PathJoin(dest_dir, basename, full_dest, BDE_PATH_MAX);

    int src_fd = vfs_open(src_path);
    if (src_fd < 0) {
        tx->status = BDE_TX_STATUS_FAILED;
        return tx->tx_id;
    }

    int dest_fd = vfs_create(full_dest);
    if (dest_fd < 0) {
        vfs_close(src_fd);
        tx->status = BDE_TX_STATUS_FAILED;
        return tx->tx_id;
    }

    static uint8_t buf[4096];
    int read_bytes = 0;
    while ((read_bytes = vfs_read(src_fd, buf, sizeof(buf))) > 0) {
        vfs_write(dest_fd, buf, read_bytes);
        tx->processed_bytes += read_bytes;
    }

    vfs_close(src_fd);
    vfs_close(dest_fd);

    tx->processed_files = 1;
    tx->progress_percent = 100;
    tx->status = BDE_TX_STATUS_COMPLETED;
    return tx->tx_id;
}

BDeTxHandle BDe_TransactionMove(const char* src_path, const char* dest_dir) {
    if (!src_path || !dest_dir) return 0;
    BDeTxProgress* tx = alloc_tx(BDE_TX_OP_MOVE, src_path, dest_dir);
    if (!tx) return 0;

    char basename[BDE_NAME_MAX];
    BDe_PathGetBasename(src_path, basename, BDE_NAME_MAX);
    char full_dest[BDE_PATH_MAX];
    BDe_PathJoin(dest_dir, basename, full_dest, BDE_PATH_MAX);

    if (vfs_rename(src_path, full_dest) == 0) {
        tx->processed_files = 1;
        tx->progress_percent = 100;
        tx->status = BDE_TX_STATUS_COMPLETED;
    } else {
        // Fallback: Copy + Delete
        if (BDe_TransactionCopy(src_path, dest_dir) != 0) {
            vfs_delete(src_path);
            tx->processed_files = 1;
            tx->progress_percent = 100;
            tx->status = BDE_TX_STATUS_COMPLETED;
        } else {
            tx->status = BDE_TX_STATUS_FAILED;
        }
    }
    return tx->tx_id;
}

BDeTxHandle BDe_TransactionDelete(const char* path, bool send_to_recycle) {
    if (!path) return 0;
    (void)send_to_recycle;

    BDeTxProgress* tx = alloc_tx(BDE_TX_OP_DELETE, path, NULL);
    if (!tx) return 0;

    if (vfs_delete(path) == 0) {
        tx->processed_files = 1;
        tx->progress_percent = 100;
        tx->status = BDE_TX_STATUS_COMPLETED;
    } else {
        tx->status = BDE_TX_STATUS_FAILED;
    }
    return tx->tx_id;
}

int32_t BDe_TransactionGetProgress(BDeTxHandle tx_id, BDeTxProgress* out_progress) {
    if (!out_progress) return -1;
    BDeTxProgress* tx = find_tx(tx_id);
    if (!tx) return -1;
    memcpy(out_progress, tx, sizeof(BDeTxProgress));
    return 0;
}

int32_t BDe_TransactionCancel(BDeTxHandle tx_id) {
    BDeTxProgress* tx = find_tx(tx_id);
    if (!tx) return -1;
    tx->status = BDE_TX_STATUS_CANCELLED;
    return 0;
}

int32_t BDe_TransactionUndo(void) {
    // Undo transaction stack
    return 0;
}
