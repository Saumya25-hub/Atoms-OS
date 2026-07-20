/**
 * @file vram_copy.c
 * @brief BSPE Partial VRAM Copy Engine Implementation
 * @status Step 11 Production Implementation
 * 
 * @section PURPOSE
 * Implements the BSPE Partial VRAM Copy Engine. Replaces full framebuffer copies
 * with dirty-rectangle scanline transfers while maintaining 100% backward compatibility
 * via an emergency fallback to Legacy SwapFull.
 * 
 * @section RULES_ENFORCED
 * - Zero heap allocations (static memory only).
 * - Zero modifications to source staging framebuffers.
 * - Supports both Legacy Full Copy and Partial Damage Copy modes via bspe_use_partial_present.
 * - Safe bounding box clipping against screen boundaries.
 * - Idempotent scanline transfers safely handle overlapping rectangles.
 */

#include "vram_copy.h"
#include "bspe_present.h"
#include "bovisual/Include/bovisual_types.h"
#include "bovisual/Include/graphics.h"

/* Required runtime flag controlling presentation mode (Default: false -> Legacy Full Copy) */
bool bspe_use_partial_present = true;

/* Static telemetry tracking structure */
static BSPE_CopyTelemetry g_copy_telemetry = {0};

/* External system RAM backbuffer accessor from bovisual/Graphics/graphics.c */
extern void* BOVISUAL_Graphics_GetBuffer(void);

void BSPE_VRAM_GetCopyTelemetry(BSPE_CopyTelemetry* out_telemetry) {
    if (!out_telemetry) return;
    *out_telemetry = g_copy_telemetry;
}

void BSPE_VRAM_ResetCopyTelemetry(void) {
    g_copy_telemetry.full_copy_count = 0;
    g_copy_telemetry.partial_copy_count = 0;
    g_copy_telemetry.total_bytes_copied = 0;
    g_copy_telemetry.total_rects_copied = 0;
    g_copy_telemetry.average_bytes_per_frame = 0;
    g_copy_telemetry.largest_rect_area = 0;
    g_copy_telemetry.smallest_rect_area = 0;
}

/**
 * @brief Internal core scanline copy engine.
 */
static BSPE_Error bspe_vram_copy_damaged_internal(const BOGE_StagingFrame* frame,
                                                  const uint8_t* src_buffer,
                                                  uint8_t* dst_buffer,
                                                  uint32_t pitch,
                                                  bool record_telemetry) {
    if (!frame || !src_buffer || !dst_buffer) {
        return BSPE_ERR_NULL_POINTER;
    }
    if (frame->width == 0 || frame->height == 0) {
        return BSPE_ERR_INVALID_STATE;
    }

    uint32_t max_buffer_size = frame->height * pitch;

    if (record_telemetry) {
        g_copy_telemetry.partial_copy_count++;
    }

    for (uint32_t i = 0; i < frame->dirty_count; i++) {
        BOGE_Rect r = frame->dirty_rects[i];

        /* 1. Clip rectangle against screen boundaries */
        int32_t x1 = r.x;
        int32_t y1 = r.y;
        int32_t x2 = r.x + (int32_t)r.width;
        int32_t y2 = r.y + (int32_t)r.height;

        if (x1 < 0) x1 = 0;
        if (y1 < 0) y1 = 0;
        if (x2 > (int32_t)frame->width)  x2 = (int32_t)frame->width;
        if (y2 > (int32_t)frame->height) y2 = (int32_t)frame->height;

        /* 2. Skip offscreen or empty rectangles */
        if (x1 >= x2 || y1 >= y2) {
            continue;
        }

        uint32_t clip_w = (uint32_t)(x2 - x1);
        uint32_t clip_h = (uint32_t)(y2 - y1);
        uint32_t row_bytes = clip_w * 4; /* Assuming 32bpp ARGB */

        /* 3. Copy ONLY damaged scanlines row-by-row safely */
        for (int32_t y = y1; y < y2; y++) {
            uint32_t offset = (uint32_t)y * pitch + (uint32_t)x1 * 4;
            if (offset + row_bytes > max_buffer_size) continue;
            const uint8_t* src_row = src_buffer + offset;
            uint8_t*       dst_row = dst_buffer + offset;

            /* Fast 64-bit SIMD-style integer block copy for row */
            uint32_t count64 = row_bytes / 8;
            const uint64_t* s64 = (const uint64_t*)src_row;
            uint64_t*       d64 = (uint64_t*)dst_row;
            for (uint32_t k = 0; k < count64; k++) {
                d64[k] = s64[k];
            }
            uint32_t rem = row_bytes % 8;
            if (rem) {
                const uint8_t* s8 = src_row + (count64 * 8);
                uint8_t*       d8 = dst_row + (count64 * 8);
                for (uint32_t k = 0; k < rem; k++) {
                    d8[k] = s8[k];
                }
            }
        }

        /* 4. Update Telemetry metrics */
        if (record_telemetry) {
            uint32_t area = clip_w * clip_h;
            g_copy_telemetry.total_bytes_copied += (uint64_t)(clip_h * row_bytes);
            g_copy_telemetry.total_rects_copied += 1;

            if (area > g_copy_telemetry.largest_rect_area) {
                g_copy_telemetry.largest_rect_area = area;
            }
            if (g_copy_telemetry.smallest_rect_area == 0 || area < g_copy_telemetry.smallest_rect_area) {
                g_copy_telemetry.smallest_rect_area = area;
            }
        }
    }

    if (record_telemetry) {
        uint64_t total_copies = g_copy_telemetry.full_copy_count + g_copy_telemetry.partial_copy_count;
        g_copy_telemetry.average_bytes_per_frame = (uint32_t)(g_copy_telemetry.total_bytes_copied / (total_copies ? total_copies : 1));
    }

    return BSPE_OK;
}

BSPE_Error BSPE_VRAM_CopyDamaged(const BOGE_StagingFrame* frame) {
    if (!frame || !frame->buffer_virtual_address) {
        return BSPE_ERR_NULL_POINTER;
    }
    if (frame->width == 0 || frame->height == 0) {
        return BSPE_ERR_INVALID_STATE;
    }

    /* If damage_count == 0, there are no changes to the screen. Fast-path return. */
    if (frame->dirty_count == 0) {
        return BSPE_OK;
    }

    /* Fallback condition check:
     * If bspe_use_partial_present == false
     * Automatically execute LegacySwapFullBackend() */
    if (!bspe_use_partial_present) {
        BOVISUAL_Graphics_LegacySwapFull_Backend(frame->buffer_virtual_address);

        /* Record Telemetry for Full Copy */
        g_copy_telemetry.full_copy_count++;
        uint64_t bytes_copied = (uint64_t)frame->height * frame->pitch;
        g_copy_telemetry.total_bytes_copied += bytes_copied;
        g_copy_telemetry.total_rects_copied += 1;

        uint64_t total_copies = g_copy_telemetry.full_copy_count + g_copy_telemetry.partial_copy_count;
        g_copy_telemetry.average_bytes_per_frame = (uint32_t)(g_copy_telemetry.total_bytes_copied / (total_copies ? total_copies : 1));

        uint32_t area = frame->width * frame->height;
        if (area > g_copy_telemetry.largest_rect_area) {
            g_copy_telemetry.largest_rect_area = area;
        }
        if (g_copy_telemetry.smallest_rect_area == 0 || area < g_copy_telemetry.smallest_rect_area) {
            g_copy_telemetry.smallest_rect_area = area;
        }
        return BSPE_OK;
    }

    /* Partial Damage Copy Mode */
    const BVFramebuffer* hw_fb = (const BVFramebuffer*)frame->buffer_virtual_address;
    if (!hw_fb || !hw_fb->buffer) {
        return BSPE_ERR_NULL_POINTER;
    }

    const uint8_t* src_buffer = (const uint8_t*)BOVISUAL_Graphics_GetBuffer();
    if (!src_buffer) {
        /* If system RAM backbuffer is unavailable, emergency fallback to legacy full copy */
        BOVISUAL_Graphics_LegacySwapFull_Backend(frame->buffer_virtual_address);
        g_copy_telemetry.full_copy_count++;
        return BSPE_OK;
    }

    uint8_t* dst_buffer = (uint8_t*)hw_fb->buffer;
    uint32_t pitch = hw_fb->pitch ? hw_fb->pitch : frame->pitch;

    return bspe_vram_copy_damaged_internal(frame, src_buffer, dst_buffer, pitch, true);
}

extern uint8_t io_in8(uint16_t port);
extern void io_out8(uint16_t port, uint8_t data);
static void local_serial_print(const char* str) {
    while (*str) {
        while ((io_in8(0x3F8 + 5) & 0x20) == 0);
        io_out8(0x3F8, *str++);
    }
}
static void local_serial_print_dec(uint32_t val) {
    char buf[16];
    int i = 14;
    buf[15] = '\0';
    if (val == 0) buf[i--] = '0';
    while (val > 0) {
        buf[i--] = '0' + (val % 10);
        val /= 10;
    }
    local_serial_print(&buf[i + 1]);
}
static void local_serial_print_hex(uint64_t val) {
    char buf[17];
    int i = 15;
    buf[16] = '\0';
    if (val == 0) buf[i--] = '0';
    while (val > 0) {
        uint8_t rem = val % 16;
        buf[i--] = (rem < 10) ? '0' + rem : 'A' + (rem - 10);
        val /= 16;
    }
    local_serial_print(&buf[i + 1]);
}

BSPE_Error BSPE_VRAM_CopyEffectiveDamage(const BOGE_StagingFrame* frame, const BOGE_Rect* effective_rects, uint32_t effective_count) {
    if (!frame || !frame->buffer_virtual_address || !effective_rects) {
        return BSPE_ERR_NULL_POINTER;
    }
    if (frame->width == 0 || frame->height == 0) {
        return BSPE_ERR_INVALID_STATE;
    }

    if (effective_count == 0) {
        return BSPE_OK;
    }

    if (!bspe_use_partial_present) {
        BOVISUAL_Graphics_LegacySwapFull_Backend(frame->buffer_virtual_address);
        g_copy_telemetry.full_copy_count++;
        uint64_t bytes_copied = (uint64_t)frame->height * frame->pitch;
        g_copy_telemetry.total_bytes_copied += bytes_copied;
        g_copy_telemetry.total_rects_copied += 1;
        uint64_t total_copies = g_copy_telemetry.full_copy_count + g_copy_telemetry.partial_copy_count;
        g_copy_telemetry.average_bytes_per_frame = (uint32_t)(g_copy_telemetry.total_bytes_copied / (total_copies ? total_copies : 1));
        extern uint32_t g_bspe_telemetry_legacy_fallbacks;
        extern uint32_t g_bspe_telemetry_full_presents;
        extern uint32_t g_bspe_telemetry_partial_presents;
        extern uint32_t g_bspe_telemetry_fallback_reason_vram;
        g_bspe_telemetry_legacy_fallbacks++;
        g_bspe_telemetry_full_presents++;
        g_bspe_telemetry_partial_presents--;
        g_bspe_telemetry_fallback_reason_vram++;
        return BSPE_OK;
    }

    const BVFramebuffer* hw_fb = (const BVFramebuffer*)frame->buffer_virtual_address;
    if (!hw_fb || !hw_fb->buffer) {
        return BSPE_ERR_NULL_POINTER;
    }

    const uint8_t* src_buffer = (const uint8_t*)BOVISUAL_Graphics_GetBuffer();
    if (!src_buffer) {
        BOVISUAL_Graphics_LegacySwapFull_Backend(frame->buffer_virtual_address);
        g_copy_telemetry.full_copy_count++;
        extern uint32_t g_bspe_telemetry_legacy_fallbacks;
        extern uint32_t g_bspe_telemetry_full_presents;
        extern uint32_t g_bspe_telemetry_partial_presents;
        extern uint32_t g_bspe_telemetry_fallback_reason_vram;
        g_bspe_telemetry_legacy_fallbacks++;
        g_bspe_telemetry_full_presents++;
        g_bspe_telemetry_partial_presents--;
        g_bspe_telemetry_fallback_reason_vram++;
        return BSPE_OK;
    }

    uint8_t* dst_buffer = (uint8_t*)hw_fb->buffer;
    uint32_t pitch = hw_fb->pitch ? hw_fb->pitch : frame->pitch;
    uint32_t max_buffer_size = frame->height * pitch;

    static bool first_frame_logged = false;
    if (!first_frame_logged) {
        local_serial_print("\n[AUDIT] FIRST FRAME PRESENTATION TRACE\n");
        local_serial_print("src pointer : 0x"); local_serial_print_hex((uint64_t)src_buffer); local_serial_print("\n");
        local_serial_print("dst pointer : 0x"); local_serial_print_hex((uint64_t)dst_buffer); local_serial_print("\n");
        local_serial_print("copy width  : "); local_serial_print_dec(frame->width); local_serial_print("\n");
        local_serial_print("copy height : "); local_serial_print_dec(frame->height); local_serial_print("\n");
        local_serial_print("src_pitch   : "); local_serial_print_dec(frame->pitch); local_serial_print("\n");
        local_serial_print("dst_pitch   : "); local_serial_print_dec(pitch); local_serial_print("\n");
        local_serial_print("bpp         : 4\n");
        local_serial_print("row advance : "); local_serial_print_dec(pitch); local_serial_print("\n");
        first_frame_logged = true;
    }

    // extern void inst_print_event(const char*);
    // inst_print_event("VRAM Copy Begin");

    g_copy_telemetry.partial_copy_count++;

    for (uint32_t i = 0; i < effective_count; i++) {
        BOGE_Rect r = effective_rects[i];
        int32_t x1 = r.x < 0 ? 0 : r.x;
        int32_t y1 = r.y < 0 ? 0 : r.y;
        int32_t x2 = (int32_t)(r.x + r.width) > (int32_t)frame->width ? (int32_t)frame->width : (int32_t)(r.x + r.width);
        int32_t y2 = (int32_t)(r.y + r.height) > (int32_t)frame->height ? (int32_t)frame->height : (int32_t)(r.y + r.height);

        if (x1 >= x2 || y1 >= y2) continue;

        uint32_t clip_w = (uint32_t)(x2 - x1);
        uint32_t clip_h = (uint32_t)(y2 - y1);
        uint32_t row_bytes = clip_w * 4;

        for (int32_t y = y1; y < y2; y++) {
            uint32_t offset = (uint32_t)y * pitch + (uint32_t)x1 * 4;
            if (offset + row_bytes > max_buffer_size) continue;
            const uint8_t* src_row = src_buffer + offset;
            uint8_t*       dst_row = dst_buffer + offset;

            uint32_t count64 = row_bytes / 8;
            const uint64_t* s64 = (const uint64_t*)src_row;
            uint64_t*       d64 = (uint64_t*)dst_row;
            for (uint32_t k = 0; k < count64; k++) d64[k] = s64[k];
            uint32_t rem = row_bytes % 8;
            if (rem) {
                const uint8_t* s8 = src_row + (count64 * 8);
                uint8_t*       d8 = dst_row + (count64 * 8);
                for (uint32_t k = 0; k < rem; k++) d8[k] = s8[k];
            }
        }

        uint32_t area = clip_w * clip_h;
        g_copy_telemetry.total_bytes_copied += (uint64_t)(clip_h * row_bytes);
        g_copy_telemetry.total_rects_copied += 1;
        if (area > g_copy_telemetry.largest_rect_area) g_copy_telemetry.largest_rect_area = area;
        if (g_copy_telemetry.smallest_rect_area == 0 || area < g_copy_telemetry.smallest_rect_area) g_copy_telemetry.smallest_rect_area = area;
    }
    
    // inst_print_event("VRAM Copy End");

    uint64_t total_copies = g_copy_telemetry.full_copy_count + g_copy_telemetry.partial_copy_count;
    g_copy_telemetry.average_bytes_per_frame = (uint32_t)(g_copy_telemetry.total_bytes_copied / (total_copies ? total_copies : 1));

    return BSPE_OK;
}

/* --- Verification Stress Test Suite --- */

/* Static 128x128 scratch test grids (16,384 pixels = 65,536 bytes each) */
#define TEST_GRID_SIZE 128
static uint32_t s_test_src_grid[TEST_GRID_SIZE * TEST_GRID_SIZE];
static uint32_t s_test_dst_legacy[TEST_GRID_SIZE * TEST_GRID_SIZE];
static uint32_t s_test_dst_partial[TEST_GRID_SIZE * TEST_GRID_SIZE];

static bool verify_grid_equality(void) {
    for (uint32_t i = 0; i < (TEST_GRID_SIZE * TEST_GRID_SIZE); i++) {
        if (s_test_dst_legacy[i] != s_test_dst_partial[i]) {
            return false;
        }
    }
    return true;
}

bool BSPE_VRAM_RunStressTest(void) {
    /* Initialize random/pattern source grid */
    for (uint32_t i = 0; i < (TEST_GRID_SIZE * TEST_GRID_SIZE); i++) {
        s_test_src_grid[i] = 0xFF000000 | (i * 12345 + 6789);
    }

    BOGE_StagingFrame test_frame;
    test_frame.frame_id = 100;
    test_frame.buffer_virtual_address = (void*)s_test_dst_partial;
    test_frame.width = TEST_GRID_SIZE;
    test_frame.height = TEST_GRID_SIZE;
    test_frame.pitch = TEST_GRID_SIZE * 4;

    uint32_t pitch = TEST_GRID_SIZE * 4;
    bool all_passed = true;

    /* Test 1: 1 Rectangle */
    for (uint32_t i = 0; i < (TEST_GRID_SIZE * TEST_GRID_SIZE); i++) {
        s_test_dst_legacy[i] = 0xFF000000;
        s_test_dst_partial[i] = 0xFF000000;
    }
    test_frame.dirty_rects[0] = (BOGE_Rect){10, 10, 50, 50};
    test_frame.dirty_count = 1;
    /* Legacy reference: copy rect manually to legacy grid */
    for (int y = 10; y < 60; y++) {
        for (int x = 10; x < 60; x++) s_test_dst_legacy[y * TEST_GRID_SIZE + x] = s_test_src_grid[y * TEST_GRID_SIZE + x];
    }
    bspe_vram_copy_damaged_internal(&test_frame, (const uint8_t*)s_test_src_grid, (uint8_t*)s_test_dst_partial, pitch, false);
    if (!verify_grid_equality()) all_passed = false;

    /* Test 2: 10 Rectangles */
    for (uint32_t i = 0; i < (TEST_GRID_SIZE * TEST_GRID_SIZE); i++) {
        s_test_dst_legacy[i] = 0xFF000000;
        s_test_dst_partial[i] = 0xFF000000;
    }
    test_frame.dirty_count = 10;
    for (int r = 0; r < 10; r++) {
        int x = (r * 11) % 100;
        int y = (r * 13) % 100;
        test_frame.dirty_rects[r] = (BOGE_Rect){x, y, 20, 20};
        for (int py = y; py < y + 20 && py < TEST_GRID_SIZE; py++) {
            for (int px = x; px < x + 20 && px < TEST_GRID_SIZE; px++) {
                s_test_dst_legacy[py * TEST_GRID_SIZE + px] = s_test_src_grid[py * TEST_GRID_SIZE + px];
            }
        }
    }
    bspe_vram_copy_damaged_internal(&test_frame, (const uint8_t*)s_test_src_grid, (uint8_t*)s_test_dst_partial, pitch, false);
    if (!verify_grid_equality()) all_passed = false;

    /* Test 3: 32 Rectangles */
    for (uint32_t i = 0; i < (TEST_GRID_SIZE * TEST_GRID_SIZE); i++) {
        s_test_dst_legacy[i] = 0xFF000000;
        s_test_dst_partial[i] = 0xFF000000;
    }
    test_frame.dirty_count = 32;
    for (int r = 0; r < 32; r++) {
        int x = (r * 7) % 110;
        int y = (r * 17) % 110;
        test_frame.dirty_rects[r] = (BOGE_Rect){x, y, 15, 15};
        for (int py = y; py < y + 15 && py < TEST_GRID_SIZE; py++) {
            for (int px = x; px < x + 15 && px < TEST_GRID_SIZE; px++) {
                s_test_dst_legacy[py * TEST_GRID_SIZE + px] = s_test_src_grid[py * TEST_GRID_SIZE + px];
            }
        }
    }
    bspe_vram_copy_damaged_internal(&test_frame, (const uint8_t*)s_test_src_grid, (uint8_t*)s_test_dst_partial, pitch, false);
    if (!verify_grid_equality()) all_passed = false;

    /* Test 4: Overlapping Rectangles */
    for (uint32_t i = 0; i < (TEST_GRID_SIZE * TEST_GRID_SIZE); i++) {
        s_test_dst_legacy[i] = 0xFF000000;
        s_test_dst_partial[i] = 0xFF000000;
    }
    test_frame.dirty_count = 2;
    test_frame.dirty_rects[0] = (BOGE_Rect){20, 20, 40, 40};
    test_frame.dirty_rects[1] = (BOGE_Rect){30, 30, 40, 40};
    for (int py = 20; py < 60; py++) {
        for (int px = 20; px < 60; px++) s_test_dst_legacy[py * TEST_GRID_SIZE + px] = s_test_src_grid[py * TEST_GRID_SIZE + px];
    }
    for (int py = 30; py < 70; py++) {
        for (int px = 30; px < 70; px++) s_test_dst_legacy[py * TEST_GRID_SIZE + px] = s_test_src_grid[py * TEST_GRID_SIZE + px];
    }
    bspe_vram_copy_damaged_internal(&test_frame, (const uint8_t*)s_test_src_grid, (uint8_t*)s_test_dst_partial, pitch, false);
    if (!verify_grid_equality()) all_passed = false;

    /* Test 5: Edge Clipping */
    for (uint32_t i = 0; i < (TEST_GRID_SIZE * TEST_GRID_SIZE); i++) {
        s_test_dst_legacy[i] = 0xFF000000;
        s_test_dst_partial[i] = 0xFF000000;
    }
    test_frame.dirty_count = 2;
    test_frame.dirty_rects[0] = (BOGE_Rect){-10, -10, 30, 30};
    test_frame.dirty_rects[1] = (BOGE_Rect){110, 110, 40, 40};
    for (int py = 0; py < 20; py++) {
        for (int px = 0; px < 20; px++) s_test_dst_legacy[py * TEST_GRID_SIZE + px] = s_test_src_grid[py * TEST_GRID_SIZE + px];
    }
    for (int py = 110; py < 128; py++) {
        for (int px = 110; px < 128; px++) s_test_dst_legacy[py * TEST_GRID_SIZE + px] = s_test_src_grid[py * TEST_GRID_SIZE + px];
    }
    bspe_vram_copy_damaged_internal(&test_frame, (const uint8_t*)s_test_src_grid, (uint8_t*)s_test_dst_partial, pitch, false);
    if (!verify_grid_equality()) all_passed = false;

    /* Test 6: Offscreen Clipping */
    for (uint32_t i = 0; i < (TEST_GRID_SIZE * TEST_GRID_SIZE); i++) {
        s_test_dst_legacy[i] = 0xFF000000;
        s_test_dst_partial[i] = 0xFF000000;
    }
    test_frame.dirty_count = 2;
    test_frame.dirty_rects[0] = (BOGE_Rect){-50, -50, 20, 20};
    test_frame.dirty_rects[1] = (BOGE_Rect){200, 200, 30, 30};
    /* Legacy grid remains unchanged (all offscreen) */
    bspe_vram_copy_damaged_internal(&test_frame, (const uint8_t*)s_test_src_grid, (uint8_t*)s_test_dst_partial, pitch, false);
    if (!verify_grid_equality()) all_passed = false;

    /* Test 7: Fullscreen Rectangle (Compare against full memcpy) */
    for (uint32_t i = 0; i < (TEST_GRID_SIZE * TEST_GRID_SIZE); i++) {
        s_test_dst_legacy[i] = s_test_src_grid[i];
        s_test_dst_partial[i] = 0xFF000000;
    }
    test_frame.dirty_count = 1;
    test_frame.dirty_rects[0] = (BOGE_Rect){0, 0, TEST_GRID_SIZE, TEST_GRID_SIZE};
    bspe_vram_copy_damaged_internal(&test_frame, (const uint8_t*)s_test_src_grid, (uint8_t*)s_test_dst_partial, pitch, false);
    if (!verify_grid_equality()) all_passed = false;

    /* Test 8: Empty Rectangle */
    for (uint32_t i = 0; i < (TEST_GRID_SIZE * TEST_GRID_SIZE); i++) {
        s_test_dst_legacy[i] = 0xFF000000;
        s_test_dst_partial[i] = 0xFF000000;
    }
    test_frame.dirty_count = 2;
    test_frame.dirty_rects[0] = (BOGE_Rect){10, 10, 0, 50};
    test_frame.dirty_rects[1] = (BOGE_Rect){20, 20, 50, 0};
    /* Legacy grid remains unchanged */
    bspe_vram_copy_damaged_internal(&test_frame, (const uint8_t*)s_test_src_grid, (uint8_t*)s_test_dst_partial, pitch, false);
    if (!verify_grid_equality()) all_passed = false;

    /* Test 9: Random Stress (32 mixed rects) */
    for (uint32_t i = 0; i < (TEST_GRID_SIZE * TEST_GRID_SIZE); i++) {
        s_test_dst_legacy[i] = 0xFF000000;
        s_test_dst_partial[i] = 0xFF000000;
    }
    test_frame.dirty_count = 32;
    for (int r = 0; r < 32; r++) {
        int x = (r * 19 - 20) % 140;
        int y = (r * 23 - 20) % 140;
        int w = (r * 5) % 40;
        int h = (r * 3) % 40;
        test_frame.dirty_rects[r] = (BOGE_Rect){x, y, w, h};

        int x1 = x < 0 ? 0 : x;
        int y1 = y < 0 ? 0 : y;
        int x2 = x + w > TEST_GRID_SIZE ? TEST_GRID_SIZE : x + w;
        int y2 = y + h > TEST_GRID_SIZE ? TEST_GRID_SIZE : y + h;
        if (x1 < x2 && y1 < y2) {
            for (int py = y1; py < y2; py++) {
                for (int px = x1; px < x2; px++) {
                    s_test_dst_legacy[py * TEST_GRID_SIZE + px] = s_test_src_grid[py * TEST_GRID_SIZE + px];
                }
            }
        }
    }
    bspe_vram_copy_damaged_internal(&test_frame, (const uint8_t*)s_test_src_grid, (uint8_t*)s_test_dst_partial, pitch, false);
    if (!verify_grid_equality()) all_passed = false;

    return all_passed;
}
