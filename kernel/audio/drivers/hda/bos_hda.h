/*
 * ATOMS OS — Intel High Definition Audio (HDA) Driver
 * Clean Native BOS Adapter & Hardware Abstraction Interface
 *
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Hardware Interface Constants Adapted from FreeBSD snd_hda (BSD-2-Clause)
 */

#ifndef BOS_HDA_H
#define BOS_HDA_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "third_party/audio/intel_hda/include/hda_reg.h"
#include "third_party/audio/intel_hda/include/hda_codec.h"
#include "kernel/core/pci/pci.h"
#include "kernel/audio/hal/audio_hal.h"

#define BOS_HDA_MAX_CODECS        4
#define BOS_HDA_DMA_BUFFER_SIZE   65536U  /* 64 KB Ring Buffer */
#define BOS_HDA_BDL_NUM_ENTRIES   4U      /* 4 x 16 KB Buffer Segments */
#define BOS_HDA_DEFAULT_STREAM_ID 1U      /* Stream Tag 1 */

/* HDA Codec Information */
typedef struct {
    uint8_t  cad;                /* Codec Address (0 - 14) */
    uint16_t vendor_id;          /* PCI Vendor ID */
    uint16_t device_id;          /* PCI Device ID */
    uint8_t  afg_node;           /* Audio Function Group Node ID */
    uint8_t  dac_node;           /* Primary Output Converter (DAC) */
    uint8_t  pin_node;           /* Primary Output Pin Complex */
    bool     is_realtek;         /* Flag for Realtek ALC Codec Family */
    char     name[32];           /* Human-readable Codec Identifier */
} bos_hda_codec_t;

/* HDA Controller Hardware Context */
typedef struct {
    PCIDevice       pci_dev;
    uint64_t        mmio_phys;
    uint64_t        mmio_virt;
    uint32_t        mmio_size;
    
    /* Stream capabilities from GCAP */
    uint8_t         num_iss;     /* Input Streams */
    uint8_t         num_oss;     /* Output Streams */
    uint8_t         num_bss;     /* Bidirectional Streams */

    /* CORB / RIRB Command Buffers */
    uint32_t*       corb_virt;
    uint64_t        corb_phys;
    uint32_t        corb_entries;
    uint16_t        corb_wp;

    uint64_t*       rirb_virt;
    uint64_t        rirb_phys;
    uint32_t        rirb_entries;
    uint16_t        rirb_rp;

    /* Output Stream State */
    uint32_t        stream_reg_offset;
    uint8_t         stream_tag;
    void*           dma_buffer_virt;
    uint64_t        dma_buffer_phys;
    uint32_t        dma_buffer_size;
    hda_bdl_entry_t* bdl_virt;
    uint64_t        bdl_phys;
    uint32_t        bdl_entries;
    volatile uint32_t write_pos;
    bool            stream_active;
    bool            stream_paused;

    /* Discovered Codec */
    bos_hda_codec_t active_codec;
    bool            codec_ready;
    bool            initialized;
} bos_hda_controller_t;

/* Global Controller Instance Access */
bos_hda_controller_t* bos_hda_get_controller(void);

/* Low-level Controller Functions */
bool bos_hda_controller_init(PCIDevice* dev);
void bos_hda_controller_shutdown(void);
uint32_t bos_hda_send_verb(uint8_t cad, uint8_t node, uint32_t verb, uint32_t param);

/* Codec Enumeration & Routing */
bool bos_hda_codec_discover_and_configure(bos_hda_controller_t* ctrl, bos_hda_codec_t* out_codec);
void bos_hda_codec_set_volume(bos_hda_controller_t* ctrl, uint8_t left, uint8_t right);

/* Stream Playback & DMA Management */
bool bos_hda_stream_init(bos_hda_controller_t* ctrl);
bool bos_hda_stream_start(uint32_t sample_rate, uint8_t channels, uint8_t bit_depth);
void bos_hda_stream_stop(void);
void bos_hda_stream_pause(void);
void bos_hda_stream_resume(void);
bool bos_hda_stream_allocate_dma(size_t requested_size, void** virt_addr, uint32_t* phys_addr);
void bos_hda_stream_update_pointers(uint32_t frames_written);
uint32_t bos_hda_stream_get_position(void);
void bos_hda_stream_set_volume(uint8_t left, uint8_t right);

/* Driver Registration */
void hda_driver_register(void);

#endif /* BOS_HDA_H */
