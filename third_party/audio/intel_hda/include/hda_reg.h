/*-
 * Copyright (c) 2006 Stephane E. Fabie
 * Copyright (c) 2008-2012 Alexander Motin <mav@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _THIRD_PARTY_HDA_REG_H_
#define _THIRD_PARTY_HDA_REG_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Intel High Definition Audio Specification (Rev 1.0a)
 * Memory-Mapped I/O Register Offsets (Relative to BAR0)
 */

/* Global Controller Registers */
#define HDA_GCAP            0x0000U /* 16-bit: Global Capabilities */
#define HDA_VMIN            0x0002U /* 8-bit:  Minor Specification Revision */
#define HDA_VMAJ            0x0003U /* 8-bit:  Major Specification Revision */
#define HDA_OUTPAY          0x0004U /* 16-bit: Output Payload Capability */
#define HDA_INPAY           0x0006U /* 16-bit: Input Payload Capability */
#define HDA_GCTL            0x0008U /* 32-bit: Global Control */
#define HDA_WAKEEN          0x000CU /* 16-bit: Wake Enable */
#define HDA_STATESTS        0x000EU /* 16-bit: State Change Status */
#define HDA_GSTS            0x0010U /* 16-bit: Global Status */
#define HDA_INTCTL          0x0020U /* 32-bit: Interrupt Control */
#define HDA_INTSTS          0x0024U /* 32-bit: Interrupt Status */
#define HDA_WALCLK          0x0030U /* 32-bit: Wall Clock Counter */
#define HDA_SSYNC           0x0038U /* 32-bit: Stream Synchronization */

/* Command Output Ring Buffer (CORB) Registers */
#define HDA_CORBLBASE       0x0040U /* 32-bit: CORB Lower Base Address */
#define HDA_CORBUBASE       0x0044U /* 32-bit: CORB Upper Base Address */
#define HDA_CORBWP          0x0048U /* 16-bit: CORB Write Pointer */
#define HDA_CORBRP          0x004AU /* 16-bit: CORB Read Pointer */
#define HDA_CORBCTL         0x004CU /* 8-bit:  CORB Control */
#define HDA_CORBSTS         0x004DU /* 8-bit:  CORB Status */
#define HDA_CORBSIZE        0x004EU /* 8-bit:  CORB Size */

/* Response Input Ring Buffer (RIRB) Registers */
#define HDA_RIRBLBASE       0x0050U /* 32-bit: RIRB Lower Base Address */
#define HDA_RIRBUBASE       0x0054U /* 32-bit: RIRB Upper Base Address */
#define HDA_RIRBWP          0x0058U /* 16-bit: RIRB Write Pointer */
#define HDA_RINTCNT         0x005AU /* 16-bit: Response Interrupt Count */
#define HDA_RIRBCTL         0x005CU /* 8-bit:  RIRB Control */
#define HDA_RIRBSTS         0x005DU /* 8-bit:  RIRB Status */
#define HDA_RIRBSIZE        0x005EU /* 8-bit:  RIRB Size */

/* Immediate Command / Response Registers (Fallback & Diagnostic) */
#define HDA_IC              0x0060U /* 32-bit: Immediate Command */
#define HDA_IR              0x0064U /* 32-bit: Immediate Response */
#define HDA_ICS             0x0068U /* 16-bit: Immediate Command Status */

/* Stream Descriptors Base Offset */
#define HDA_SD_BASE         0x0080U
#define HDA_SD_STRIDE       0x0020U

/* Stream Descriptor Register Offsets (Relative to stream base) */
#define HDA_SD_CTL          0x0000U /* 24-bit: Stream Descriptor Control */
#define HDA_SD_CTL_B2       0x0002U /* Byte 2 of SD_CTL (Stream Tag, etc.) */
#define HDA_SD_STS          0x0003U /* 8-bit:  Stream Descriptor Status */
#define HDA_SD_LPIB         0x0004U /* 32-bit: Link Position In Buffer */
#define HDA_SD_CBL          0x0008U /* 32-bit: Cyclic Buffer Length */
#define HDA_SD_LVI          0x000CU /* 16-bit: Last Valid Index */
#define HDA_SD_FIFOS        0x0010U /* 16-bit: FIFO Size */
#define HDA_SD_FMT          0x0012U /* 16-bit: Stream Format */
#define HDA_SD_BDLPL        0x0018U /* 32-bit: Buffer Descriptor List Lower */
#define HDA_SD_BDLPU        0x001CU /* 32-bit: Buffer Descriptor List Upper */

/* Register Bit Definitions */

/* GCTL */
#define HDA_GCTL_CRST       0x00000001U /* Controller Reset: 1 = Run, 0 = Reset */
#define HDA_GCTL_FCNTRL     0x00000002U /* Flush Control */
#define HDA_GCTL_UNSOL      0x00000100U /* Accept Unsolicited Responses */

/* CORBCTL */
#define HDA_CORBCTL_RUN     0x02U       /* Enable CORB DMA Engine */
#define HDA_CORBCTL_MEIE    0x01U       /* Memory Error Interrupt Enable */

/* CORBRP */
#define HDA_CORBRP_RST      0x8000U     /* Reset CORB Read Pointer */

/* RIRBCTL */
#define HDA_RIRBCTL_RUN     0x02U       /* Enable RIRB DMA Engine */
#define HDA_RIRBCTL_RINT    0x01U       /* Response Interrupt Enable */

/* ICS (Immediate Command Status) */
#define HDA_ICS_BUSY        0x0001U     /* Command busy / in-flight */
#define HDA_ICS_IRV         0x0002U     /* Immediate Result Valid */

/* SD_CTL */
#define HDA_SD_CTL_SRST     0x000001U   /* Stream Reset */
#define HDA_SD_CTL_RUN      0x000002U   /* Stream DMA Run */
#define HDA_SD_CTL_IOCE     0x000004U   /* Interrupt On Completion Enable */
#define HDA_SD_CTL_FEIE     0x000008U   /* FIFO Error Interrupt Enable */
#define HDA_SD_CTL_DEIE     0x000010U   /* Descriptor Error Interrupt Enable */

/* SD_STS */
#define HDA_SD_STS_BCIS     0x04U       /* Buffer Completion Interrupt Status */
#define HDA_SD_STS_FIFOE    0x08U       /* FIFO Error */
#define HDA_SD_STS_DESCE    0x10U       /* Descriptor Error */

/* Stream Format (SD_FMT) Encodings */
#define HDA_FMT_CHAN(c)     (((c) - 1) & 0x0FU)
#define HDA_FMT_BITS_8      (0x0U << 4)
#define HDA_FMT_BITS_16     (0x1U << 4)
#define HDA_FMT_BITS_20     (0x2U << 4)
#define HDA_FMT_BITS_24     (0x3U << 4)
#define HDA_FMT_BITS_32     (0x4U << 4)
#define HDA_FMT_BASE_48K    (0x0U << 14)
#define HDA_FMT_BASE_44K    (0x1U << 14)
#define HDA_FMT_MULT_1X     (0x0U << 11)
#define HDA_FMT_DIV_1X      (0x0U << 8)

/* Standard 48kHz 16-bit Stereo PCM Format Word */
#define HDA_FORMAT_48K_16BIT_STEREO (HDA_FMT_BASE_48K | HDA_FMT_MULT_1X | HDA_FMT_DIV_1X | HDA_FMT_BITS_16 | HDA_FMT_CHAN(2))

/* Buffer Descriptor List (BDL) Entry */
#pragma pack(push, 1)
typedef struct {
    uint32_t addr_low;   /* Buffer physical address (bits 0-31) */
    uint32_t addr_high;  /* Buffer physical address (bits 32-63) */
    uint32_t length;     /* Buffer length in bytes */
    uint32_t flags;      /* Bit 0: Interrupt On Completion (IOC) */
} hda_bdl_entry_t;
#pragma pack(pop)

#define HDA_BDL_IOC 0x00000001U

/* Verb Construction Helper Macros */
#define HDA_VERB_4BIT(codec, node, verb, param) \
    (((uint32_t)(codec) << 28) | ((uint32_t)(node) << 20) | ((uint32_t)(verb) << 8) | ((uint32_t)(param) & 0xFFU))

#define HDA_VERB_12BIT(codec, node, verb12, param8) \
    (((uint32_t)(codec) << 28) | ((uint32_t)(node) << 20) | (((uint32_t)(verb12) & 0xFFFU) << 8) | ((uint32_t)(param8) & 0xFFU))

#ifdef __cplusplus
}
#endif

#endif /* _THIRD_PARTY_HDA_REG_H_ */
