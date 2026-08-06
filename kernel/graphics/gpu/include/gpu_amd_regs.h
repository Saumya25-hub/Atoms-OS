#ifndef BOS_GPU_AMD_REGS_H
#define BOS_GPU_AMD_REGS_H

#include <stdint.h>

/* AMD PCI Identifiers */
#define AMD_PCI_VENDOR_ID                   0x1002

/* Display Core (DC) Plane Registers */
#define AMD_D1GRPH_CONTROL                  0x0500
#define AMD_D1GRPH_PRIMARY_SURFACE_ADDRESS  0x050C
#define AMD_D1GRPH_PITCH                    0x0520
#define AMD_D1GRPH_SURFACE_OFFSET           0x0524

/* Hardware Cursor Plane Registers */
#define AMD_D1CUR_CONTROL                   0x0A00
#define AMD_D1CUR_SURFACE_ADDRESS           0x0A04
#define AMD_D1CUR_POSITION                  0x0A08

/* Display CRTC Timing Registers */
#define AMD_CRTC_CONTROL                    0x0600
#define AMD_CRTC_H_TOTAL                    0x0604
#define AMD_CRTC_V_TOTAL                    0x0608
#define AMD_CRTC_STATUS                     0x0614

/* SDMA Engine Ring Registers */
#define AMD_SDMA0_GFX_RB_BASE               0x4000
#define AMD_SDMA0_GFX_RB_CNTL               0x4004
#define AMD_SDMA0_GFX_RB_RPTR               0x4008
#define AMD_SDMA0_GFX_RB_WPTR               0x400C

/* SDMA Opcodes */
#define SDMA_OP_NOP                         0x00
#define SDMA_OP_COPY                        0x01
#define SDMA_OP_FILL                        0x02

/* Memory Controller & GART Registers */
#define AMD_GMC_GART_BASE                   0x2000
#define AMD_GMC_GART_CNTL                   0x2004
#define AMD_GART_PTE_VALID                  0x00000001ULL

#endif /* BOS_GPU_AMD_REGS_H */
