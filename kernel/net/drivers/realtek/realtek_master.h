/* kernel/net/drivers/realtek/realtek_master.h - Realtek Multi-Family Master Engine Header */
#ifndef SIGNATURES_REALTEK_MASTER_H
#define SIGNATURES_REALTEK_MASTER_H

#include <kernel/net/net_framework.h>

#define REALTEK_VENDOR_ID 0x10EC

#define REALTEK_DEV_RTL8168 0x8168
#define REALTEK_DEV_RTL8111 0x8111
#define REALTEK_DEV_RTL8125 0x8125

void realtek_master_init(void);
bool realtek_master_probe(PCIDevice* pdev);

#endif /* SIGNATURES_REALTEK_MASTER_H */
