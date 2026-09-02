#ifndef AIPDEBUG_H
#define AIPDEBUG_H

#include "kernel/debug/aipdebug/aipdebug_schema.h"

void aipd_init(uint16_t profile_mask);
uint16_t aipd_tx_begin(AIPDTransactionType tx_type, uint32_t target_addr);
void aipd_tx_record(uint16_t tx_id, AIPDSubsystem subsys, AIPDComponent comp,
                    AIPDEventType evt, AIPDTruthClass truth,
                    uint32_t addr, uint32_t before, uint32_t after,
                    uint16_t status);
void aipd_tx_end(uint16_t tx_id, AIPDTransactionResult result);
void aipd_flush(void);
uint32_t aipd_get_dropped_count(void);
void aipd_detect_cpu_brand(char out_brand[49]);

#endif /* AIPDEBUG_H */
