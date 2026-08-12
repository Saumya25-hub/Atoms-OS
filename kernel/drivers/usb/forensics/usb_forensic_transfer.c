#include "usb_forensic_center.h"
#include "kernel/core/lib/include/string.h"

void usb_forensic_log_control_transfer(uint8_t req_type, uint8_t req, uint16_t val, uint16_t idx, uint16_t len) {
    ForensicControlTransferPanel* p = &g_forensic_center.control_transfer;
    p->bmRequestType = req_type;
    p->bRequest = req;
    p->wValue = val;
    p->wIndex = idx;
    p->wLength = len;

    if (req_type & 0x80) {
        strcpy(p->direction, "IN");
    } else {
        strcpy(p->direction, "OUT");
    }

    uint32_t trt = 0;
    if (len > 0) trt = (req_type & 0x80) ? 3 : 2;

    p->setup_dw0 = req_type | (req << 8) | (val << 16);
    p->setup_dw1 = idx | (len << 16);
    p->setup_dw2 = 8;
    p->setup_dw3 = (2 << 10) | (trt << 16) | (1 << 6) | 1;

    p->data_dw0 = (uint32_t)(g_forensic_center.dma.dma_phys & 0xFFFFFFFF);
    p->data_dw1 = (uint32_t)((g_forensic_center.dma.dma_phys >> 32) & 0xFFFFFFFF);
    p->data_dw2 = len & 0x1FFFF;
    p->data_dw3 = (3 << 1) | ((req_type & 0x80) ? (1 << 16) : 0) | (1 << 2) | 1;

    uint32_t status_dir = (trt == 0 || trt == 2) ? (1 << 16) : 0;
    p->status_dw0 = 0;
    p->status_dw1 = 0;
    p->status_dw2 = 0;
    p->status_dw3 = (4 << 10) | status_dir | (1 << 5) | 1;
}
