#include "../Include/graphics.h"
#include "../../kernel/graphics/BSPE/include/bspe.h"
#include "kernel/debug/step14_telemetry.h"
#include "kernel/debug/desktop_diag.h"
#include <stddef.h>

static BVFramebuffer g_active_fb = {0};
static bool g_graphics_ready = false;

// Phase 5: Damage Tracking
static int32_t damage_x1 = 99999;
static int32_t damage_y1 = 99999;
static int32_t damage_x2 = -1;
static int32_t damage_y2 = -1;

// Global Hardware-style Clipping
static BVRect g_clip_rect = {0, 0, 0, 0};
static bool g_clip_enabled = false;

void BOVISUAL_Graphics_SetClipRect(BVRect clip) {
    if (clip.x < 0) { clip.width += clip.x; clip.x = 0; }
    if (clip.y < 0) { clip.height += clip.y; clip.y = 0; }
    if (clip.x + clip.width > (int32_t)g_active_fb.width) clip.width = g_active_fb.width - clip.x;
    if (clip.y + clip.height > (int32_t)g_active_fb.height) clip.height = g_active_fb.height - clip.y;
    if (clip.width < 0) clip.width = 0;
    if (clip.height < 0) clip.height = 0;
    g_clip_rect = clip;
    g_clip_enabled = true;
}

void BOVISUAL_Graphics_ClearClipRect(void) {
    g_clip_enabled = false;
}

static uint32_t g_bovisual_ram_buffer[2560 * 1600] __attribute__((aligned(16)));

// Called by Core during initialization
void internal_graphics_init(const BVFramebuffer* fb) {
    if (fb) {
        g_active_fb = *fb;
        g_active_fb.buffer = (BOVISUAL_Color*)g_bovisual_ram_buffer;
        g_graphics_ready = true;
        BSPE_Config cfg;
        cfg.display_width = fb->width;
        cfg.display_height = fb->height;
        cfg.buffer_count = 2;
        cfg.enable_vsync = false;
        BSPE_Initialize(&cfg);
    }
}

// Called by Core during shutdown
void internal_graphics_shutdown(void) {
    BSPE_Shutdown();
    g_active_fb.buffer = NULL;
    g_graphics_ready = false;
}

void BOVISUAL_Graphics_PutPixel(int32_t x, int32_t y, BOVISUAL_Color color) {
    if (!g_graphics_ready || !g_active_fb.buffer) return;

    if (x < 0 || x >= (int32_t)g_active_fb.width || y < 0 || y >= (int32_t)g_active_fb.height) {
        return; // Clip to screen bounds
    }
    
    if (g_clip_enabled) {
        if (x < g_clip_rect.x || x >= g_clip_rect.x + g_clip_rect.width ||
            y < g_clip_rect.y || y >= g_clip_rect.y + g_clip_rect.height) {
            return; // Out of clip rect
        }
    }

    uint8_t* row_ptr = (uint8_t*)g_active_fb.buffer + (y * g_active_fb.pitch);
    ((BOVISUAL_Color*)row_ptr)[x] = color;
}

BOVISUAL_Color BOVISUAL_Graphics_ReadPixel(int32_t x, int32_t y) {
    if (!g_graphics_ready || !g_active_fb.buffer) return 0;

    if (x < 0 || x >= (int32_t)g_active_fb.width || y < 0 || y >= (int32_t)g_active_fb.height) {
        return 0; // Out of bounds returns 0 (Transparent/Black)
    }

    uint8_t* row_ptr = (uint8_t*)g_active_fb.buffer + (y * g_active_fb.pitch);
    return ((BOVISUAL_Color*)row_ptr)[x];
}

void BOVISUAL_Graphics_Clear(BOVISUAL_Color color) {
    if (!g_graphics_ready || !g_active_fb.buffer) return;

    if (g_clip_enabled) {
        BOVISUAL_Graphics_Fill(g_clip_rect.x, g_clip_rect.y, g_clip_rect.width, g_clip_rect.height, color);
        return;
    }

    for (int32_t row = 0; row < (int32_t)g_active_fb.height; row++) {
        uint8_t* row_ptr = (uint8_t*)g_active_fb.buffer + (row * g_active_fb.pitch);
        BOVISUAL_Color* pixel_ptr = (BOVISUAL_Color*)row_ptr;
        for (int32_t col = 0; col < (int32_t)g_active_fb.width; col++) {
            pixel_ptr[col] = color;
        }
    }
}

void BOVISUAL_Graphics_Fill_Internal(int32_t x, int32_t y, int32_t width, int32_t height, BOVISUAL_Color color) {
    if (!g_graphics_ready || !g_active_fb.buffer || width <= 0 || height <= 0) return;

    int32_t cx1 = x;
    int32_t cy1 = y;
    int32_t cx2 = x + width;
    int32_t cy2 = y + height;
    
    // Absolute clipping against screen bounds
    if (cx1 < 0) cx1 = 0;
    if (cy1 < 0) cy1 = 0;
    if (cx2 > (int32_t)g_active_fb.width) cx2 = (int32_t)g_active_fb.width;
    if (cy2 > (int32_t)g_active_fb.height) cy2 = (int32_t)g_active_fb.height;

    // Global Hardware Clipping
    if (g_clip_enabled) {
        if (cx1 < g_clip_rect.x) cx1 = g_clip_rect.x;
        if (cy1 < g_clip_rect.y) cy1 = g_clip_rect.y;
        if (cx2 > g_clip_rect.x + g_clip_rect.width) cx2 = g_clip_rect.x + g_clip_rect.width;
        if (cy2 > g_clip_rect.y + g_clip_rect.height) cy2 = g_clip_rect.y + g_clip_rect.height;
    }

    if (cx1 >= cx2 || cy1 >= cy2) return;

    int32_t draw_width = cx2 - cx1;
    int32_t draw_height = cy2 - cy1;

    for (int32_t row = 0; row < draw_height; row++) {
        uint8_t* row_ptr = (uint8_t*)g_active_fb.buffer + ((cy1 + row) * g_active_fb.pitch);
        BOVISUAL_Color* pixel_ptr = (BOVISUAL_Color*)row_ptr;
        for (int32_t col = 0; col < draw_width; col++) {
            pixel_ptr[cx1 + col] = color;
        }
    }
}

void BOVISUAL_Graphics_Fill(int32_t x, int32_t y, int32_t width, int32_t height, BOVISUAL_Color color) {
    BOVISUAL_Graphics_Fill_Internal(x, y, width, height, color);
}

void BOVISUAL_Graphics_AddDamage(int32_t x, int32_t y, int32_t width, int32_t height) {
    if (x < damage_x1) damage_x1 = x;
    if (y < damage_y1) damage_y1 = y;
    if (x + width - 1 > damage_x2) damage_x2 = x + width - 1;
    if (y + height - 1 > damage_y2) damage_y2 = y + height - 1;
}

void BOVISUAL_Graphics_ResetDamage(void) {
    damage_x1 = 99999;
    damage_y1 = 99999;
    damage_x2 = -1;
    damage_y2 = -1;
}

void BOVISUAL_Graphics_SwapBuffers(const BVFramebuffer* hw_fb) {
    if (!g_graphics_ready || !g_active_fb.buffer || !hw_fb || !hw_fb->buffer) return;
    
    extern bool AGDTE_IsInitialized(void);
    if (AGDTE_IsInitialized()) {
        /* Phase 4.1 Presentation Ownership Integration:
         * When AGDTE is active, all legacy SwapBuffers calls MUST NOT independently copy to Page 0.
         * Redirect through BOVISUAL_Graphics_SwapFull using the authoritative VBE back page.
         */
        extern BVFramebuffer* vbe_get_back_page_ptr(void);
        BVFramebuffer* back_fb = vbe_get_back_page_ptr();
        BOVISUAL_Graphics_SwapFull(back_fb);
        BOVISUAL_Graphics_ResetDamage();
        return;
    }
    
    // If no damage, skip swap entirely
    if (damage_x1 > damage_x2 || damage_y1 > damage_y2) return;
    
    // Clip damage rect to screen boundaries
    if (damage_x1 < 0) damage_x1 = 0;
    if (damage_y1 < 0) damage_y1 = 0;
    if (damage_x2 >= (int32_t)g_active_fb.width) damage_x2 = g_active_fb.width - 1;
    if (damage_y2 >= (int32_t)g_active_fb.height) damage_y2 = g_active_fb.height - 1;
    
    int32_t w = damage_x2 - damage_x1 + 1;
    int32_t h = damage_y2 - damage_y1 + 1;
    if (w <= 0 || h <= 0) return;
    
    // Fast 64-bit copy ONLY IF damage covers full screen and stride matches
    if (w == (int32_t)g_active_fb.width && h == (int32_t)g_active_fb.height && g_active_fb.pitch == hw_fb->pitch) {
        uint32_t total_bytes = g_active_fb.height * g_active_fb.pitch;
        uint64_t* src = (uint64_t*)g_active_fb.buffer;
        uint64_t* dst = (uint64_t*)hw_fb->buffer;
        uint32_t count = total_bytes / 8;
        uint32_t count8 = count / 8;
        for (uint32_t i = 0; i < count8; i++) {
            dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
            dst[4] = src[4]; dst[5] = src[5]; dst[6] = src[6]; dst[7] = src[7];
            dst += 8; src += 8;
        }
        for (uint32_t i = 0; i < (count % 8); i++) {
            dst[i] = src[i];
        }
    } else {
        // 64-bit optimized row-by-row copy for PARTIAL damage regions
        for (int32_t row = damage_y1; row <= damage_y2; row++) {
            uint8_t* src_row = (uint8_t*)g_active_fb.buffer + (row * g_active_fb.pitch);
            uint8_t* dst_row = (uint8_t*)hw_fb->buffer + (row * hw_fb->pitch);
            
            // 64-bit copy: 2 pixels per write (32bpp * 2 = 64 bits)
            uint64_t* src64 = (uint64_t*)((uint32_t*)src_row + damage_x1);
            uint64_t* dst64 = (uint64_t*)((uint32_t*)dst_row + damage_x1);
            int32_t pairs = w / 2;
            for (int32_t p = 0; p < pairs; p++) {
                dst64[p] = src64[p];
            }
            // Handle odd trailing pixel
            if (w & 1) {
                uint32_t* src32 = (uint32_t*)src_row + damage_x1 + (pairs * 2);
                uint32_t* dst32 = (uint32_t*)dst_row + damage_x1 + (pairs * 2);
                *dst32 = *src32;
            }
        }
    }
    
    __asm__ volatile("sfence" ::: "memory");
    BOVISUAL_Graphics_ResetDamage();
}

/* Internal presentation backend invoked by BSPE_PresentFrame in Step 10 */
void BOVISUAL_Graphics_LegacySwapFull_Backend(const BVFramebuffer* hw_fb) {
    static bool s_swap_logged = false;
    if (!g_graphics_ready || !g_active_fb.buffer || !hw_fb || !hw_fb->buffer) {
        if (!s_swap_logged) {
            diag_puts("[VRAM_DIAG] LegacySwapFull_Backend: ABORTED! g_ready=");
            diag_put_dec(g_graphics_ready ? 1 : 0);
            diag_puts(" active_fb.buf=0x"); diag_put_hex64((uint64_t)g_active_fb.buffer);
            diag_puts(" hw_fb=0x"); diag_put_hex64((uint64_t)hw_fb);
            diag_puts(" hw_buf=0x"); diag_put_hex64(hw_fb ? (uint64_t)hw_fb->buffer : 0);
            diag_puts("\r\n");
            s_swap_logged = true;
        }
        return;
    }

    if (!s_swap_logged) {
        uint32_t* s32 = (uint32_t*)g_active_fb.buffer;
        uint32_t* d32 = (uint32_t*)hw_fb->buffer;
        diag_puts("[VRAM_DIAG] LegacySwapFull_Backend: START! src=0x");
        diag_put_hex64((uint64_t)g_active_fb.buffer);
        diag_puts(" ("); diag_put_dec(g_active_fb.width); diag_puts("x"); diag_put_dec(g_active_fb.height);
        diag_puts(" p="); diag_put_dec(g_active_fb.pitch);
        diag_puts(") -> dst=0x"); diag_put_hex64((uint64_t)hw_fb->buffer);
        diag_puts(" ("); diag_put_dec(hw_fb->width); diag_puts("x"); diag_put_dec(hw_fb->height);
        diag_puts(" p="); diag_put_dec(hw_fb->pitch); diag_puts(")\r\n");
        diag_puts("[VRAM_DIAG] src[0..3]=");
        diag_put_hex32(s32[0]); diag_puts(" ");
        diag_put_hex32(s32[1]); diag_puts(" ");
        diag_put_hex32(s32[2]); diag_puts(" ");
        diag_put_hex32(s32[3]); diag_puts(" | dst_BEFORE[0..3]=");
        diag_put_hex32(d32[0]); diag_puts(" ");
        diag_put_hex32(d32[1]); diag_puts(" ");
        diag_put_hex32(d32[2]); diag_puts(" ");
        diag_put_hex32(d32[3]); diag_puts("\r\n");
    }
    
    // Fast 64-bit copy ONLY IF stride matches!
    if (g_active_fb.width == hw_fb->width && g_active_fb.height == hw_fb->height && g_active_fb.pitch == hw_fb->pitch) {
        uint32_t total_bytes = g_active_fb.height * g_active_fb.pitch;
        uint64_t* src64 = (uint64_t*)g_active_fb.buffer;
        uint64_t* dst64 = (uint64_t*)hw_fb->buffer;
        uint32_t count64 = total_bytes / 8;
        uint32_t count8 = count64 / 8;
        for (uint32_t i = 0; i < count8; i++) {
            dst64[0] = src64[0]; dst64[1] = src64[1]; dst64[2] = src64[2]; dst64[3] = src64[3];
            dst64[4] = src64[4]; dst64[5] = src64[5]; dst64[6] = src64[6]; dst64[7] = src64[7];
            dst64 += 8; src64 += 8;
        }
        for (uint32_t i = 0; i < (count64 % 8); i++) {
            dst64[i] = src64[i];
        }
        
        uint32_t rem = total_bytes % 8;
        if (rem) {
            uint8_t* src8 = (uint8_t*)g_active_fb.buffer + (count64 * 8);
            uint8_t* dst8 = (uint8_t*)hw_fb->buffer + (count64 * 8);
            for (uint32_t i = 0; i < rem; i++) dst8[i] = src8[i];
        }
    } else {
        // Safe slow copy line by line
        for (uint32_t row = 0; row < g_active_fb.height; row++) {
            uint32_t* src_row = (uint32_t*)((uint8_t*)g_active_fb.buffer + (row * g_active_fb.pitch));
            uint32_t* dst_row = (uint32_t*)((uint8_t*)hw_fb->buffer + (row * hw_fb->pitch));
            for (uint32_t col = 0; col < g_active_fb.width; col++) {
                dst_row[col] = src_row[col];
            }
        }
    }
    __asm__ volatile("sfence" ::: "memory");

    if (!s_swap_logged) {
        uint32_t* d32 = (uint32_t*)hw_fb->buffer;
        diag_puts("[VRAM_DIAG] dst_AFTER[0..3]=");
        diag_put_hex32(d32[0]); diag_puts(" ");
        diag_put_hex32(d32[1]); diag_puts(" ");
        diag_put_hex32(d32[2]); diag_puts(" ");
        diag_put_hex32(d32[3]); diag_puts(" | COMPLETE!\r\n");
        s_swap_logged = true;
    }
}

/* Official BSPE Presentation Entry Point Adapter (Step 10) */
void BOVISUAL_Graphics_SwapFull(const BVFramebuffer* hw_fb) {
    if (!hw_fb) return;

    /* Execution Context Firewall: Strictly forbid VRAM presentation from IRQ context (IF=0) */
    uint64_t rflags;
    __asm__ volatile("pushfq; popq %0" : "=r"(rflags));
    if ((rflags & (1ULL << 9)) == 0) {
        extern void com1_puts(const char* s);
        com1_puts("[BCM][SECURITY] PRESENTATION BLOCKED: IF=0 in BOVISUAL_Graphics_SwapFull\r\n");
        return;
    }

    static uint32_t s_legacy_frame_id = 0;
    BOGE_StagingFrame staging_frame;
    staging_frame.frame_id = ++s_legacy_frame_id;
    staging_frame.buffer_virtual_address = (void*)hw_fb;
    staging_frame.width = hw_fb->width;
    staging_frame.height = hw_fb->height;
    staging_frame.pitch = hw_fb->pitch;
    /* Phase 2: Enable BSPE Partial VRAM Copying by forwarding compositor damage */
    extern bool bspe_use_partial_present;
    bspe_use_partial_present = true;
    
    typedef struct { int32_t x; int32_t y; int32_t width; int32_t height; } BWE_DirtyRectStub;
    extern BWE_DirtyRectStub g_dirty_rects[];
    extern uint32_t g_dirty_rect_count;
    
    if (g_dirty_rect_count > 0 && g_dirty_rect_count <= 32) {
        staging_frame.dirty_count = g_dirty_rect_count;
        for (uint32_t i = 0; i < g_dirty_rect_count; i++) {
            staging_frame.dirty_rects[i].x = g_dirty_rects[i].x;
            staging_frame.dirty_rects[i].y = g_dirty_rects[i].y;
            staging_frame.dirty_rects[i].width = (uint32_t)g_dirty_rects[i].width;
            staging_frame.dirty_rects[i].height = (uint32_t)g_dirty_rects[i].height;
        }
    } else {
        staging_frame.dirty_count = 0; /* Fallback if no dirty rects reported */
    }
    
    /* STEP 14 TEMPORARY INSTRUMENTATION */
    uint64_t start_tsc = step14_rdtsc();
    /* END STEP 14 */

    extern int AGDTE_Presenter_PresentBridgeBSPE(const BOGE_StagingFrame* boge_frame, uint32_t display_id);
    AGDTE_Presenter_PresentBridgeBSPE(&staging_frame, 0);

    /* STEP 14 TEMPORARY INSTRUMENTATION */
    uint64_t end_tsc = step14_rdtsc();
    uint32_t duration_us = step14_cycles_to_us(end_tsc - start_tsc);
    uint32_t bytes = hw_fb->height * hw_fb->pitch;
    step14_log_swapfull(staging_frame.dirty_count, duration_us, bytes);
    /* END STEP 14 */

    extern void bos_profiler_record_mem_copy(uint64_t bytes, bool is_vram);
    bos_profiler_record_mem_copy((uint64_t)bytes, true);
}
void BOVISUAL_Graphics_SwapRect(const BVFramebuffer* hw_fb, BVRect rect) {
    if (!g_graphics_ready || !g_active_fb.buffer || !hw_fb || !hw_fb->buffer || rect.width <= 0 || rect.height <= 0) return;
    
    extern bool AGDTE_IsInitialized(void);
    if (AGDTE_IsInitialized()) {
        extern BVFramebuffer* vbe_get_back_page_ptr(void);
        BVFramebuffer* back_fb_ptr = vbe_get_back_page_ptr();
        BOVISUAL_Graphics_SwapFull(back_fb_ptr);
        return;
    }
    
    int32_t cx1 = rect.x;
    int32_t cy1 = rect.y;
    int32_t cx2 = rect.x + rect.width;
    int32_t cy2 = rect.y + rect.height;
    
    // Clip damage rect to screen boundaries safely
    if (cx1 < 0) cx1 = 0;
    if (cy1 < 0) cy1 = 0;
    if (cx2 > (int32_t)g_active_fb.width) cx2 = (int32_t)g_active_fb.width;
    if (cy2 > (int32_t)g_active_fb.height) cy2 = (int32_t)g_active_fb.height;
    
    if (cx1 >= cx2 || cy1 >= cy2) return;
    
    rect.width = cx2 - cx1;
    rect.height = cy2 - cy1;
    rect.x = cx1;
    rect.y = cy1;

    // Safety check! Ensure hw_fb bounds match
    if (rect.x + rect.width > (int32_t)hw_fb->width) rect.width = hw_fb->width - rect.x;
    if (rect.y + rect.height > (int32_t)hw_fb->height) rect.height = hw_fb->height - rect.y;

    if (rect.width <= 0 || rect.height <= 0) return;
    
    // Copy only the dirty rectangle row by row safely using independent pitches
    for (int32_t row = 0; row < rect.height; row++) {
        uint8_t* src_row = (uint8_t*)g_active_fb.buffer + ((rect.y + row) * g_active_fb.pitch);
        uint8_t* dst_row = (uint8_t*)hw_fb->buffer + ((rect.y + row) * hw_fb->pitch);
        
        uint32_t* src = (uint32_t*)src_row + rect.x;
        uint32_t* dst = (uint32_t*)dst_row + rect.x;
        
        for (int32_t col = 0; col < rect.width; col++) {
            dst[col] = src[col];
        }
    }
}

void* BOVISUAL_Graphics_GetBuffer(void) {
    if (g_active_fb.buffer) return g_active_fb.buffer;
    return (void*)g_bovisual_ram_buffer;
}

uint32_t BOVISUAL_Graphics_GetPitch(void) {
    if (g_active_fb.pitch > 0) return g_active_fb.pitch;
    extern uint32_t g_kernel_screen_width;
    if (g_kernel_screen_width > 0) return g_kernel_screen_width * 4;
    return 1024 * 4;
}

uint32_t BOVISUAL_Graphics_GetWidth(void) {
    if (g_active_fb.width > 0) return g_active_fb.width;
    extern uint32_t g_kernel_screen_width;
    if (g_kernel_screen_width > 0) return g_kernel_screen_width;
    return 1024;
}

uint32_t BOVISUAL_Graphics_GetHeight(void) {
    if (g_active_fb.height > 0) return g_active_fb.height;
    extern uint32_t g_kernel_screen_height;
    if (g_kernel_screen_height > 0) return g_kernel_screen_height;
    return 768;
}
