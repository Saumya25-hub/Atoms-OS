#ifndef BOS_GPU_INTEL_REGS_H
#define BOS_GPU_INTEL_REGS_H

#include <stdint.h>

/* Intel PCI Identifiers */
#define INTEL_PCI_VENDOR_ID                 0x8086

/* Power Well & Clock Control Registers */
#define INTEL_PWR_WELL_CTL                  0x45400
#define INTEL_CDCLK_CTL                     0x46000
#define INTEL_DPLL_CTRL1                    0x6C058
#define INTEL_LCPLL_CTL                     0x130040

/* Display Pipe & Timing Registers */
#define INTEL_HTOTAL_A                      0x60000
#define INTEL_HBLANK_A                      0x60004
#define INTEL_HSYNC_A                       0x60008
#define INTEL_VTOTAL_A                      0x6000C
#define INTEL_VBLANK_A                      0x60010
#define INTEL_VSYNC_A                       0x60014
#define INTEL_PIPECONF_A                    0x70008
#define INTEL_PIPESRC_A                     0x6001C

/* Universal Primary Plane 1 Registers (Skylake / Gen9+) */
#define INTEL_PLANE_CTL_1_A                 0x70180
#define INTEL_PLANE_STRIDE_1_A              0x70188
#define INTEL_PLANE_POS_1_A                 0x7018C
#define INTEL_PLANE_SIZE_1_A                0x70190
#define INTEL_PLANE_SURF_1_A                0x7019C
#define INTEL_PLANE_KEYVAL_1_A              0x70194

/* Legacy Display Plane A Registers (Gen4 - Gen8) */
#define INTEL_DSPACNTR                      0x70180
#define INTEL_DSPAADDR                      0x70184
#define INTEL_DSPASTRIDE                    0x70188
#define INTEL_DSPASURF                      0x7019C

/* Hardware Cursor Plane A Registers */
#define INTEL_CURACNTR                      0x70080
#define INTEL_CURABASE                      0x70084
#define INTEL_CURAPOS                       0x70088

/* Display Interrupt Registers */
#define INTEL_DEISR                         0x44000
#define INTEL_DEIMR                         0x44004
#define INTEL_DEIIR                         0x44008
#define INTEL_DEIER                         0x4400C

/* Ring Engine MMIO Register Ranges */
#define INTEL_RCS_TAIL                      0x02030
#define INTEL_RCS_HEAD                      0x02034
#define INTEL_RCS_START                     0x02038
#define INTEL_RCS_CTL                       0x0203C

#define INTEL_BCS_TAIL                      0x22030
#define INTEL_BCS_HEAD                      0x22034
#define INTEL_BCS_START                     0x22038
#define INTEL_BCS_CTL                       0x2203C

/* GGTT Memory Offset in BAR0 GTTMMADR */
#define INTEL_GGTT_PTE_BASE                 0x800000   /* 8 MB MMIO Offset */
#define INTEL_GGTT_PTE_VALID                0x1ULL
#define INTEL_GGTT_PAGE_SIZE                4096

/* PCI Configuration Stolen Memory Register */
#define INTEL_PCI_BSM                       0x5C

#endif /* BOS_GPU_INTEL_REGS_H */
