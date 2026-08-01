#include "../include/brt_api.h"

static uint32_t s_next_tx_id = 1000;

BRTTxHandle BRT_StartTransaction(BRTRuntime* rt, BRTTxType type) {
    if (!rt || !rt->active) return 0;
    (void)type;
    rt->active_tx = s_next_tx_id++;
    return rt->active_tx;
}

int32_t BRT_CommitTransaction(BRTTxHandle tx) {
    if (tx == 0) return -1;
    return 0;
}

int32_t BRT_RollbackTransaction(BRTTxHandle tx) {
    if (tx == 0) return -1;
    return 0;
}
