#ifndef BOS_EDID_H
#define BOS_EDID_H

#include "kernel/graphics/display/include/bos_display.h"

/* 18-Byte Detailed Timing Descriptor (DTD) */
typedef struct __attribute__((packed)) {
    uint16_t pixel_clock_10khz;
    uint8_t  hactive_lo;
    uint8_t  hblank_lo;
    uint8_t  hactive_hblank_hi;
    uint8_t  vactive_lo;
    uint8_t  vblank_lo;
    uint8_t  vactive_vblank_hi;
    uint8_t  hsync_offset_lo;
    uint8_t  hsync_width_lo;
    uint8_t  vsync_offset_width_lo;
    uint8_t  sync_hi;
    uint8_t  h_size_mm_lo;
    uint8_t  v_size_mm_lo;
    uint8_t  size_mm_hi;
    uint8_t  h_border;
    uint8_t  v_border;
    uint8_t  flags;
} edid_dtd_t;

/* 128-Byte Base EDID 1.4 Structure */
typedef struct __attribute__((packed)) {
    uint8_t  header[8];              /* 0x00 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0x00 */
    uint16_t manufacturer_id;        /* Compressed ASCII 3-char PNP Code */
    uint16_t product_code;
    uint32_t serial_number;
    uint8_t  mfg_week;
    uint8_t  mfg_year;               /* Year offset from 1990 */
    uint8_t  edid_version;           /* 1 */
    uint8_t  edid_revision;          /* 4 */
    uint8_t  video_input_params;
    uint8_t  h_screen_size_cm;
    uint8_t  v_screen_size_cm;
    uint8_t  gamma;
    uint8_t  feature_support;
    uint8_t  color_characteristics[10];
    uint8_t  established_timings[3];
    uint8_t  standard_timings[16];
    edid_dtd_t dtd[4];               /* 4 x 18-Byte Detailed Timing Descriptors */
    uint8_t  extension_flag;         /* Number of extension blocks */
    uint8_t  checksum;
} edid_base_t;

/* CEA-861 Extension Block (Tag 0x02) */
typedef struct __attribute__((packed)) {
    uint8_t tag;                     /* 0x02 */
    uint8_t revision;                /* 0x03 */
    uint8_t dtd_offset;
    uint8_t native_formats;
    uint8_t data_block_collection[123];
    uint8_t checksum;
} edid_cea_extension_t;

/* Extracted Monitor Meta Information */
typedef struct {
    char     pnp_id[4];              /* E.g., "DEL", "LGD", "SAM" */
    uint16_t product_id;
    uint32_t serial;
    uint32_t mfg_year;
    uint32_t width_mm;
    uint32_t height_mm;
    char     monitor_name[32];
    uint32_t native_width;
    uint32_t native_height;
    uint32_t native_refresh;
    bool     has_cea_extension;
    bool     valid;
} edid_info_t;

bos_display_status_t edid_parse_base(const uint8_t* raw_128, edid_info_t* out_info);
bos_display_status_t edid_parse_cea(const uint8_t* raw_cea, edid_info_t* inout_info);

#endif /* BOS_EDID_H */
