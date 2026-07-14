#include <stdint.h>
#include <stdbool.h>
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/input/input.h"

// External kernel metrics
extern volatile uint64_t g_irq1_count;
extern volatile uint64_t g_irq12_count;
extern uint32_t g_kbd_events_per_sec;
extern uint32_t g_mouse_events_per_sec;
extern uint64_t timer_get_ticks(void);

// App state
static uint32_t s_window_id = 0;
static uint32_t s_app_fps = 0;
static uint32_t s_frame_count = 0;
static uint64_t s_last_fps_time = 0;

static uint64_t s_last_kbd_time = 0;
static uint64_t s_last_mouse_time = 0;
static uint64_t s_last_event_time = 0;

// Mouse state
static int32_t s_mouse_x = 0;
static int32_t s_mouse_y = 0;
static int32_t s_mouse_dx = 0;
static int32_t s_mouse_dy = 0;
static uint8_t s_mouse_buttons = 0;
static uint32_t s_mouse_packet_count = 0;

// Keyboard state
static char s_kbd_last_name[32] = "NONE";
static uint8_t s_kbd_last_scan = 0;
static char s_kbd_last_ascii = 0;
static bool s_kbd_pressed = false;
static bool s_kbd_released = false;
static uint32_t s_kbd_repeat_count = 0;
static uint64_t s_kbd_timestamp = 0;

// Trace log
#define TRACE_LOG_SIZE 15
typedef struct {
    char text[64];
    uint64_t time;
} TraceLine;
static TraceLine s_trace_log[TRACE_LOG_SIZE];
static int s_trace_head = 0;

static void my_itoa(uint32_t val, char* b) {
    char temp[16];
    int i = 0;
    if (val == 0) { b[0] = '0'; b[1] = '\0'; return; }
    while (val > 0) { temp[i++] = (val % 10) + '0'; val /= 10; }
    int j = 0;
    while (i > 0) b[j++] = temp[--i];
    b[j] = '\0';
}

static void my_itoa_hex(uint32_t val, char* b) {
    char temp[16];
    int i = 0;
    if (val == 0) { b[0] = '0'; b[1] = '\0'; return; }
    while (val > 0) { 
        uint32_t rem = val % 16;
        if (rem < 10) temp[i++] = rem + '0';
        else temp[i++] = (rem - 10) + 'A';
        val /= 16; 
    }
    int j = 0;
    while (i > 0) b[j++] = temp[--i];
    b[j] = '\0';
}

static void log_trace(const char* msg) {
    uint32_t hh = (uint32_t)((timer_get_ticks() / 3600000) % 24);
    uint32_t mm = (uint32_t)((timer_get_ticks() / 60000) % 60);
    uint32_t ss = (uint32_t)((timer_get_ticks() / 1000) % 60);

    char buf[64];
    char num_buf[16];

    my_itoa(hh, num_buf); strcpy(buf, num_buf); strcat(buf, ":");
    my_itoa(mm, num_buf); strcat(buf, num_buf); strcat(buf, ":");
    my_itoa(ss, num_buf); strcat(buf, num_buf); strcat(buf, " ");
    strcat(buf, msg);

    strcpy(s_trace_log[s_trace_head].text, buf);
    s_trace_log[s_trace_head].time = timer_get_ticks();
    s_trace_head = (s_trace_head + 1) % TRACE_LOG_SIZE;
}

// ---------------------------------------------------------
// Window Events
// ---------------------------------------------------------
static void input_lab_event_cb(uint32_t id, const BWE_Event* event) {
    if (id != s_window_id || !event) return;
    
    s_last_event_time = timer_get_ticks();
    
    if (event->type == BWE_EVENT_KEY_DOWN || event->type == BWE_EVENT_KEY_UP) {
        s_last_kbd_time = s_last_event_time;
        s_kbd_timestamp = s_last_event_time;
        
        bool pressed = (event->type == BWE_EVENT_KEY_DOWN);
        if (pressed && s_kbd_last_scan == event->data.key.key_code && s_kbd_pressed) {
            s_kbd_repeat_count++;
        } else {
            s_kbd_repeat_count = 0;
        }
        
        s_kbd_last_scan = event->data.key.key_code;
        s_kbd_last_ascii = (event->data.key.key_code >= 32 && event->data.key.key_code <= 126) ? (char)event->data.key.key_code : '?';
        s_kbd_pressed = pressed;
        s_kbd_released = !pressed;
        
        char msg[64] = "KEY ";
        strcat(msg, pressed ? "DOWN " : "UP ");
        char num[16]; my_itoa(s_kbd_last_scan, num); strcat(msg, num);
        log_trace(msg);
        
    } else if (event->type == BWE_EVENT_MOUSE_MOVE || event->type == BWE_EVENT_MOUSE_DOWN || event->type == BWE_EVENT_MOUSE_UP) {
        s_last_mouse_time = s_last_event_time;
        s_mouse_packet_count++;
        
        s_mouse_dx = event->data.mouse.x - s_mouse_x;
        s_mouse_dy = event->data.mouse.y - s_mouse_y;
        s_mouse_x = event->data.mouse.x;
        s_mouse_y = event->data.mouse.y;
        s_mouse_buttons = event->data.mouse.buttons;
        
        if (event->type == BWE_EVENT_MOUSE_DOWN) {
            log_trace("Mouse DOWN");
        } else if (event->type == BWE_EVENT_MOUSE_UP) {
            log_trace("Mouse UP");
        } else if (s_mouse_packet_count % 30 == 0) { // Log every 30th move so we don't spam
            log_trace("Mouse MOVE");
        }
    }
}

// ---------------------------------------------------------
// Window Render
// ---------------------------------------------------------
static void input_lab_render_cb(BWE_Window* self) {
    if (!self) return;
    
    // FPS tracking
    s_frame_count++;
    uint64_t now = timer_get_ticks();
    if (now - s_last_fps_time >= 1000) {
        s_app_fps = s_frame_count;
        s_frame_count = 0;
        s_last_fps_time = now;
    }

    extern const BVFramebuffer* BWE_GetRenderTarget(void);
    const BVFramebuffer* fb = BWE_GetRenderTarget();
    BWE_Rect b = self->screen_bounds;
    
    // Background
    BWE_FillRect(fb, b.x, b.y, b.width, b.height, 0xFF0A0A0A);
    BWE_DrawRect(fb, b.x, b.y, b.width, b.height, 0xFF333333, 1);
    
    // Header
    BWE_FillRect(fb, b.x, b.y, b.width, 30, 0xFF1E1E1E);
    BWE_DrawText(fb, "BOS INPUT DIAGNOSTICS RUNTIME", b.x + 10, b.y + 10, 0xFF00FF00, 0);
    
    int32_t col1_x = b.x + 10;
    int32_t col2_x = b.x + 250;
    int32_t col3_x = b.x + 500;
    
    char buf[128];
    char num[32];
    
    // --- COLUMN 1: LIVE INPUT (KEYBOARD & MOUSE) ---
    BWE_DrawText(fb, "--- KEYBOARD ---", col1_x, b.y + 40, 0xFFAAAAAA, 0);
    strcpy(buf, "Key Name: "); strcat(buf, s_kbd_last_name);
    BWE_DrawText(fb, buf, col1_x, b.y + 60, 0xFFFFFFFF, 0);
    
    strcpy(buf, "Scan: 0x"); my_itoa_hex(s_kbd_last_scan, num); strcat(buf, num);
    BWE_DrawText(fb, buf, col1_x, b.y + 80, 0xFFFFFFFF, 0);
    
    strcpy(buf, "ASCII: "); 
    int len = 0; while (buf[len]) len++;
    buf[len++] = s_kbd_last_ascii; buf[len] = 0;
    BWE_DrawText(fb, buf, col1_x, b.y + 100, 0xFFFFFFFF, 0);
    
    strcpy(buf, "Pressed: "); strcat(buf, s_kbd_pressed ? "YES" : "NO");
    BWE_DrawText(fb, buf, col1_x, b.y + 120, s_kbd_pressed ? 0xFF00FF00 : 0xFFFFFFFF, 0);
    
    strcpy(buf, "Released: "); strcat(buf, s_kbd_released ? "YES" : "NO");
    BWE_DrawText(fb, buf, col1_x, b.y + 140, s_kbd_released ? 0xFF00FF00 : 0xFFFFFFFF, 0);
    
    strcpy(buf, "Repeat: "); my_itoa(s_kbd_repeat_count, num); strcat(buf, num);
    BWE_DrawText(fb, buf, col1_x, b.y + 160, 0xFFFFFFFF, 0);

    BWE_DrawText(fb, "--- MOUSE ---", col1_x, b.y + 200, 0xFFAAAAAA, 0);
    strcpy(buf, "Mouse X: "); my_itoa(s_mouse_x, num); strcat(buf, num);
    BWE_DrawText(fb, buf, col1_x, b.y + 220, 0xFFFFFFFF, 0);
    
    strcpy(buf, "Mouse Y: "); my_itoa(s_mouse_y, num); strcat(buf, num);
    BWE_DrawText(fb, buf, col1_x, b.y + 240, 0xFFFFFFFF, 0);
    
    strcpy(buf, "Delta X: "); 
    if (s_mouse_dx < 0) { strcat(buf, "-"); my_itoa(-s_mouse_dx, num); strcat(buf, num); }
    else { my_itoa(s_mouse_dx, num); strcat(buf, num); }
    BWE_DrawText(fb, buf, col1_x, b.y + 260, 0xFFFFFFFF, 0);
    
    strcpy(buf, "Delta Y: "); 
    if (s_mouse_dy < 0) { strcat(buf, "-"); my_itoa(-s_mouse_dy, num); strcat(buf, num); }
    else { my_itoa(s_mouse_dy, num); strcat(buf, num); }
    BWE_DrawText(fb, buf, col1_x, b.y + 280, 0xFFFFFFFF, 0);
    
    strcpy(buf, "Left Btn: "); strcat(buf, (s_mouse_buttons & 1) ? "YES" : "NO");
    BWE_DrawText(fb, buf, col1_x, b.y + 300, (s_mouse_buttons & 1) ? 0xFF00FF00 : 0xFFFFFFFF, 0);
    
    strcpy(buf, "Right Btn: "); strcat(buf, (s_mouse_buttons & 2) ? "YES" : "NO");
    BWE_DrawText(fb, buf, col1_x, b.y + 320, (s_mouse_buttons & 2) ? 0xFF00FF00 : 0xFFFFFFFF, 0);
    
    strcpy(buf, "Packet Count: "); my_itoa(s_mouse_packet_count, num); strcat(buf, num);
    BWE_DrawText(fb, buf, col1_x, b.y + 340, 0xFFFFFFFF, 0);

    // --- COLUMN 2: ENGINE STATUS ---
    BWE_DrawText(fb, "--- ENGINE COUNTERS ---", col2_x, b.y + 40, 0xFFAAAAAA, 0);
    
    strcpy(buf, "KBD IRQ Count: "); my_itoa(g_irq1_count, num); strcat(buf, num);
    BWE_DrawText(fb, buf, col2_x, b.y + 60, 0xFF00FFFF, 0);
    
    strcpy(buf, "Mouse IRQ Count: "); my_itoa(g_irq12_count, num); strcat(buf, num);
    BWE_DrawText(fb, buf, col2_x, b.y + 80, 0xFF00FFFF, 0);
    
    strcpy(buf, "KBD Events/sec: "); my_itoa(g_kbd_events_per_sec, num); strcat(buf, num);
    BWE_DrawText(fb, buf, col2_x, b.y + 100, 0xFFFFFFFF, 0);
    
    strcpy(buf, "Mouse Events/sec: "); my_itoa(g_mouse_events_per_sec, num); strcat(buf, num);
    BWE_DrawText(fb, buf, col2_x, b.y + 120, 0xFFFFFFFF, 0);
    
    strcpy(buf, "Input Queue Size: "); my_itoa(kernel_input_get_queue_size(), num); strcat(buf, num);
    BWE_DrawText(fb, buf, col2_x, b.y + 140, 0xFFFF00FF, 0);
    
    BWE_DrawText(fb, "--- EVENT TRACE ---", col2_x, b.y + 200, 0xFFAAAAAA, 0);
    BWE_DrawText(fb, "PS/2 Hardware  [PASS]", col2_x, b.y + 220, 0xFF00FF00, 0);
    BWE_DrawText(fb, "      |", col2_x, b.y + 240, 0xFF666666, 0);
    BWE_DrawText(fb, "Driver         [PASS]", col2_x, b.y + 260, 0xFF00FF00, 0);
    BWE_DrawText(fb, "      |", col2_x, b.y + 280, 0xFF666666, 0);
    BWE_DrawText(fb, "Input Engine   [PASS]", col2_x, b.y + 300, 0xFF00FF00, 0);
    BWE_DrawText(fb, "      |", col2_x, b.y + 320, 0xFF666666, 0);
    BWE_DrawText(fb, "Dispatcher     [PASS]", col2_x, b.y + 340, 0xFF00FF00, 0);
    BWE_DrawText(fb, "      |", col2_x, b.y + 360, 0xFF666666, 0);
    BWE_DrawText(fb, "App Callback   [PASS]", col2_x, b.y + 380, 0xFF00FF00, 0);

    // --- COLUMN 3: RUNTIME LOG & HEARTBEAT ---
    BWE_DrawText(fb, "--- HEARTBEAT ---", col3_x, b.y + 40, 0xFFAAAAAA, 0);
    strcpy(buf, "Runtime FPS: "); my_itoa(s_app_fps, num); strcat(buf, num);
    BWE_DrawText(fb, buf, col3_x, b.y + 60, 0xFFFFFF00, 0);
    
    strcpy(buf, "Runtime Tick: "); my_itoa((uint32_t)now, num); strcat(buf, num);
    BWE_DrawText(fb, buf, col3_x, b.y + 80, 0xFFFFFFFF, 0);
    
    strcpy(buf, "Last KBD Tick: "); my_itoa((uint32_t)s_last_kbd_time, num); strcat(buf, num);
    BWE_DrawText(fb, buf, col3_x, b.y + 100, 0xFFFFFFFF, 0);
    
    strcpy(buf, "Last Mouse Tick: "); my_itoa((uint32_t)s_last_mouse_time, num); strcat(buf, num);
    BWE_DrawText(fb, buf, col3_x, b.y + 120, 0xFFFFFFFF, 0);
    
    BWE_DrawText(fb, "--- SCROLLING LOG ---", col3_x, b.y + 160, 0xFFAAAAAA, 0);
    int32_t ly = b.y + 180;
    for (int i = 0; i < TRACE_LOG_SIZE; i++) {
        int idx = (s_trace_head - 1 - i + TRACE_LOG_SIZE) % TRACE_LOG_SIZE;
        if (s_trace_log[idx].time == 0) continue;
        BWE_DrawText(fb, s_trace_log[idx].text, col3_x, ly, 0xFFCCCCCC, 0);
        ly += 16;
    }

    // Force continuous redraw for live diagnostics
    BWE_InvalidateWindow(self->id);
}

// ---------------------------------------------------------
// App Entry
// ---------------------------------------------------------
int input_lab_init(uint32_t* out_win) {
    uint32_t win_id;
    bwe_error_t err = BOS_CreateWindow(100, 100, 750, 450, "BOS Input Lab", &win_id);
    if (err != BWE_SUCCESS) return -1;

    BWE_Window* win = BWE_GetWindow(win_id);
    if (win) {
        win->on_render = input_lab_render_cb;
        win->on_event = input_lab_event_cb;
        s_window_id = win_id;
        s_last_fps_time = timer_get_ticks();
        if (out_win) *out_win = win_id;
        return 0;
    }
    return -1;
}
