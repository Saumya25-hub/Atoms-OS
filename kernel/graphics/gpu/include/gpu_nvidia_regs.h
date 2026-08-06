#ifndef BOS_GPU_NVIDIA_REGS_H
#define BOS_GPU_NVIDIA_REGS_H

#include <stdint.h>

/* NVIDIA PCI Identifiers */
#define NVIDIA_PCI_VENDOR_ID                0x10DE

/* Power Management Controller (NV_PMC) */
#define NV_PMC_ENABLE                       0x000200
#define NV_PMC_BOOT_0                       0x000000

/* Framebuffer Memory Controller (NV_PFB) */
#define NV_PFB_CFG                          0x100200
#define NV_PFB_SIZE                         0x100209

/* CRTC Display Controller Registers (NV_PCRTC) */
#define NV_PCRTC_START                      0x600800
#define NV_PCRTC_TIMING                     0x600804
#define NV_PCRTC_RASTER_START               0x60080C
#define NV_PCRTC_CONFIG                     0x600810
#define NV_PCRTC_H_TOTAL                    0x600814
#define NV_PCRTC_V_TOTAL                    0x600818

/* PRAMDAC Controls & Hardware Cursor Registers (NV_PRAMDAC) */
#define NV_PRAMDAC_CURSOR_CTRL              0x680800
#define NV_PRAMDAC_CURSOR_BASE              0x680804
#define NV_PRAMDAC_CURSOR_POS               0x680808
#define NV_PRAMDAC_FP_TG_CONTROL            0x680840
#define NV_PRAMDAC_GENERAL_CONTROL          0x680848

#endif /* BOS_GPU_NVIDIA_REGS_H */
