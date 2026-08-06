#ifndef BOS_DPDP_H
#define BOS_DPDP_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    DPDP_STATUS_PASS = 0,
    DPDP_STATUS_WARN = 1,
    DPDP_STATUS_FAIL = 2,
    DPDP_STATUS_SKIP = 3
} dpdp_status_t;

typedef struct {
    dpdp_status_t status;
    uint32_t     draw_calls;
    uint32_t     ui_elements;
    uint64_t     pixels_generated;
} dpdp_stage01_app_t;

typedef struct {
    dpdp_status_t status;
    uint32_t     surfaces;
    uint32_t     dirty_regions;
    uint32_t     crc;
} dpdp_stage02_boimage_t;

typedef struct {
    dpdp_status_t status;
    uint32_t     layers;
    uint32_t     blend_time_us;
    uint32_t     output_crc;
} dpdp_stage03_compositor_t;

typedef struct {
    dpdp_status_t status;
    uint64_t     address;
    uint32_t     pitch;
    uint32_t     size;
    uint64_t     non_black_pixels;
    uint32_t     crc;
} dpdp_stage04_backbuffer_t;

typedef struct {
    dpdp_status_t src_status;
    dpdp_status_t dst_status;
    uint32_t     bytes_copied;
    uint32_t     copy_time_us;
} dpdp_stage05_memcpy_t;

typedef struct {
    dpdp_status_t present_status;
    uint32_t     surface_handle;
    bool         fence_ready;
} dpdp_stage06_gpu_hal_t;

typedef struct {
    dpdp_status_t status;
    uint32_t     fifo_writes;
    bool         update_cmd_sent;
    bool         sync_complete;
} dpdp_stage07_vmware_driver_t;

typedef struct {
    dpdp_status_t status;
    uint64_t     physical_address;
    uint32_t     crc;
    uint32_t     first_pixel;
    uint32_t     last_pixel;
} dpdp_stage08_framebuffer_t;

typedef struct {
    dpdp_status_t status;
    uint32_t     display_buffer_page;
    bool         scanout_active;
    uint32_t     bytes_per_line;
} dpdp_stage09_scanout_t;

typedef struct {
    dpdp_status_t status;
    uint64_t     visible_pixels;
    uint64_t     black_pixels;
    bool         last_refresh_ok;
} dpdp_stage10_monitor_t;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t enable;
    uint32_t sync;
    uint32_t busy;
    uint32_t bytes_per_line;
    uint32_t fifo_next_cmd;
    uint32_t fifo_stop;
} dpdp_register_watch_t;

typedef struct {
    uint32_t draw_ui_us;
    uint32_t boimage_finish_us;
    uint32_t compositor_finish_us;
    uint32_t memcpy_finish_us;
    uint32_t present_us;
    uint32_t fifo_update_us;
    uint32_t sync_us;
    uint32_t monitor_refresh_us;
} dpdp_timeline_t;

typedef struct {
    uint32_t frame_id;
    uint32_t time_ms;
    char     gpu_name[32];
    char     backend[16];
    char     display_name[16];
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    uint32_t refresh_rate;

    dpdp_stage01_app_t           stage01;
    dpdp_stage02_boimage_t       stage02;
    dpdp_stage03_compositor_t    stage03;
    dpdp_stage04_backbuffer_t    stage04;
    dpdp_stage05_memcpy_t        stage05;
    dpdp_stage06_gpu_hal_t       stage06;
    dpdp_stage07_vmware_driver_t stage07;
    dpdp_stage08_framebuffer_t   stage08;
    dpdp_stage09_scanout_t       stage09;
    dpdp_stage10_monitor_t       stage10;

    dpdp_register_watch_t        regs;
    dpdp_timeline_t              timeline;

    uint32_t                     failed_stage_id;
    char                         root_cause[128];
    uint32_t                     confidence_percent;
} dpdp_frame_autopsy_t;

void dpdp_init(dpdp_frame_autopsy_t* autopsy);
void dpdp_record_sample_frame(dpdp_frame_autopsy_t* autopsy);
void dpdp_compile_autopsy(dpdp_frame_autopsy_t* autopsy);
void dpdp_print_flight_recorder(const dpdp_frame_autopsy_t* autopsy);

#endif /* BOS_DPDP_H */
