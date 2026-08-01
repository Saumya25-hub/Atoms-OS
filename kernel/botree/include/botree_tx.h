#ifndef BOTREE_TX_H
#define BOTREE_TX_H

#include "botree_types.h"

// Transaction Engine Public Functions
BDeTxHandle BDe_TransactionCopy(const char* src_path, const char* dest_dir);
BDeTxHandle BDe_TransactionMove(const char* src_path, const char* dest_dir);
BDeTxHandle BDe_TransactionDelete(const char* path, bool send_to_recycle);
int32_t     BDe_TransactionGetProgress(BDeTxHandle tx_id, BDeTxProgress* out_progress);
int32_t     BDe_TransactionCancel(BDeTxHandle tx_id);
int32_t     BDe_TransactionUndo(void);

#endif // BOTREE_TX_H
