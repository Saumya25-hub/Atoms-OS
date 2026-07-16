
static void bos_canvas_render(BWE_Window* win) {
    extern const BVFramebuffer* BWE_GetRenderTarget(void);
    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (!fb || !win->control_data.canvas.pixel_buffer) return;

    uint32_t bw = win->control_data.canvas.buffer_w;
    uint32_t bh = win->control_data.canvas.buffer_h;
    const uint32_t* src = win->control_data.canvas.pixel_buffer;

    int32_t start_x = win->screen_bounds.x;
    int32_t start_y = win->screen_bounds.y;
    
    // Draw shadow if not borderless
    if (!(win->flags & 0x0020)) { // BWE_WINDOW_BORDERLESS = 0x20
        start_x += 5;
        start_y += 35;
    }

    extern bool BWE_GetClip(BWE_Rect* out_rect);
    BWE_Rect clip;
    bool has_clip = BWE_GetClip(&clip);

    for (uint32_t y = 0; y < bh; y++) {
        for (uint32_t x = 0; x < bw; x++) {
            int32_t sx = start_x + (int32_t)x;
            int32_t sy = start_y + (int32_t)y;

            if (sx >= 0 && sx < (int32_t)fb->width && sy >= 0 && sy < (int32_t)fb->height) {
                if (has_clip) {
                    if (sx >= clip.x && sx < clip.x + clip.width && sy >= clip.y && sy < clip.y + clip.height) {
                        fb->buffer[sy * fb->width + sx] = src[y * bw + x];
                    }
                } else {
                    fb->buffer[sy * fb->width + sx] = src[y * bw + x];
                }
            }
        }
    }
}

bwe_error_t BOS_SurfacePresent(uint32_t window_id, const uint32_t* pixels, uint32_t w, uint32_t h) {
    BWE_Window* win = BWE_GetWindow(window_id);
    if (!win) return BWE0001;

    extern void* kmalloc(uint32_t size);
    extern void  kfree(void* ptr);

    if (win->control_data.canvas.buffer_w != w || win->control_data.canvas.buffer_h != h || !win->control_data.canvas.pixel_buffer) {
        if (win->control_data.canvas.pixel_buffer) {
            kfree(win->control_data.canvas.pixel_buffer);
        }
        win->control_data.canvas.pixel_buffer = (uint32_t*)kmalloc(w * h * 4);
        win->control_data.canvas.buffer_w = w;
        win->control_data.canvas.buffer_h = h;
    }

    if (!win->control_data.canvas.pixel_buffer) {
        return BWE0004; // ERROR ALLOC
    }

    uint32_t* dst = win->control_data.canvas.pixel_buffer;
    for (uint32_t i = 0; i < w * h; i++) {
        dst[i] = pixels[i];
    }

    if (w == 640 || w == 320) {
        static uint32_t trace_frame = 0;
        trace_frame++;
        if (trace_frame == 1 || trace_frame == 2 || trace_frame == 3 || trace_frame == 10 || trace_frame == 100 || trace_frame == 500) {
            uint32_t checksum = 0;
            for (uint32_t i = 0; i < w * h; i++) {
                checksum ^= dst[i]; // XOR checksum
                checksum += dst[i]; // ADD checksum
            }
            extern void display_print(const char*);
            extern void display_print_dec(uint32_t);
            extern void display_print_hex(uint32_t);
            display_print("\n--- PHASE 11 CANVAS AUTOPSY (FRAME "); display_print_dec(trace_frame); display_print(") ---\n");
            display_print("Canvas Pointer: 0x"); display_print_hex((uint32_t)(uint64_t)dst); display_print("\n");
            display_print("Width: "); display_print_dec(w); display_print("\n");
            display_print("Height: "); display_print_dec(h); display_print("\n");
            display_print("Pitch: "); display_print_dec(w * 4); display_print("\n");
            display_print("First Pixel: 0x"); display_print_hex(dst[0]); display_print("\n");
            display_print("Middle Pixel: 0x"); display_print_hex(dst[(h/2)*w + (w/2)]); display_print("\n");
            display_print("Last Pixel: 0x"); display_print_hex(dst[(h-1)*w + (w-1)]); display_print("\n");
            display_print("Checksum: 0x"); display_print_hex(checksum); display_print("\n");
            display_print("----------------------------------------\n");
        }
    }

    win->on_render = bos_canvas_render;
    win->is_dirty = true;

    return BWE_SUCCESS;
}
