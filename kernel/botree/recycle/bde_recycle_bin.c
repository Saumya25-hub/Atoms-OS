#include "../include/botree.h"
#include "../include/botree_tx.h"

int32_t BDe_RecycleMoveToTrash(const char* path) {
    if (!path) return -1;
    BDeTxHandle tx = BDe_TransactionMove(path, "/RECYCLE");
    return (tx != 0) ? 0 : -1;
}

int32_t BDe_RecycleEmptyTrash(void) {
    // Purge /RECYCLE directory items
    return 0;
}
