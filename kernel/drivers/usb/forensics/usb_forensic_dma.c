#include "usb_forensic_center.h"
#include "kernel/core/lib/include/string.h"

void usb_forensic_log_dma(uint64_t phys, uint64_t virt, uint32_t len, const void* data) {
    ForensicDmaPanel* dma = &g_forensic_center.dma;
    dma->dma_phys = phys;
    dma->dma_virt = virt;
    dma->length = len;
    dma->cache_flush_pass = true;
    dma->cache_invalidate_pass = true;

    memset(dma->raw_dma, 0, sizeof(dma->raw_dma));
    if (data && len > 0) {
        uint32_t copy_len = len > 64 ? 64 : len;
        memcpy(dma->raw_dma, data, copy_len);
    }
}
