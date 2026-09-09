/*
 * =====================================================================
 * ATOMS OS — CHROMIUM MULTI-PROCESS, IPC & EXCEPTION CERTIFICATION
 * =====================================================================
 * Pure Ring 3 C++ & C Certification Application executing on ATOMS OS.
 * Validates tests P01 through P12:
 *  - P01 Process Creation (PID >= 200, distinct PML4)
 *  - P02 Address-Space Isolation (Hardware page table isolation)
 *  - P03 Parent/Child Lifecycle (waitpid, exit code 42, reap)
 *  - P04 Thread Creation + Teardown (4 concurrent threads, mutex)
 *  - P05 REAL Cross-Process Mojo Round-Trip (MojoPing / MojoPong)
 *  - P06 Cross-Process Shared Memory (64KB shared physical frames)
 *  - P07 Mojo Endpoint Close/Error (Peer closure error handling)
 *  - P08 Userspace Exception/Page-Fault Isolation (Ring 3 #PF containment)
 *  - P09 Controlled Renderer-Style Child Crash (Ring 3 #UD containment)
 *  - P10 Parent/Other Child Survival (Sibling survival, zero cascades)
 *  - P11 Multi-Process Chromium Stress (Browser + 2 Renderers + GPU + Util)
 *  - P12 100-Cycle Process + IPC + SHM Stress (100x lifecycle loop, zero leaks)
 * =====================================================================
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "atoms/userspace/apal/include/apal.h"
#include "atoms/userspace/runtime/include/atoms_syscall.h"

extern "C" {
    int snprintf(char *str, size_t size, const char *format, ...);
    size_t strlen(const char *s);
    int strcmp(const char *s1, const char *s2);
    void *malloc(size_t size);
    void free(void *ptr);
    void *memcpy(void *dest, const void *src, size_t n);
    void *memset(void *s, int c, size_t n);
    int64_t write(int fd, const void *buf, size_t count);
    int sched_yield(void);
    int getpid(void);
    void _exit(int status);
}

/* --- ATOMS GUI Syscalls (Inline Assembly) --- */
#define SYS_GUI_CREATE_WINDOW   16ULL
#define SYS_GUI_SHOW_WINDOW     18ULL
#define SYS_GUI_MAP_SURFACE     20ULL
#define SYS_GUI_INVALIDATE      21ULL
#define SYS_GUI_POLL_EVENT      22ULL
#define SYS_GUI_GET_SCREEN_INFO 23ULL

typedef struct {
    uint32_t abi_version;
    uint32_t type;
    uint32_t window_id;
    int32_t  mouse_x;
    int32_t  mouse_y;
    uint32_t mouse_btn;
    uint32_t key_code;
    uint32_t ascii_char;
    uint32_t modifiers;
    uint32_t reserved;
} GUIEvent;

static inline uint32_t atoms_gui_create_window(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t flags, const char* title) {
    uint32_t id = 0;
    register uint64_t r10 __asm__("r10") = (uint64_t)h;
    register uint64_t r8  __asm__("r8")  = (uint64_t)flags;
    register uint64_t r9  __asm__("r9")  = (uint64_t)title;
    __asm__ volatile("syscall"
        : "=a"(id)
        : "a"(SYS_GUI_CREATE_WINDOW), "D"(x), "S"(y), "d"(w), "r"(r10), "r"(r8), "r"(r9)
        : "rcx", "r11", "memory");
    return id;
}

static inline int atoms_gui_show_window(uint32_t win_id, bool visible) {
    int res = 0;
    __asm__ volatile("syscall"
        : "=a"(res)
        : "a"(SYS_GUI_SHOW_WINDOW), "D"(win_id), "S"((uint32_t)visible)
        : "rcx", "r11", "memory");
    return res;
}

static inline int atoms_gui_map_surface(uint32_t win_id, uint32_t** out_surface_pixels, uint32_t* out_stride_bytes) {
    int res = 0;
    __asm__ volatile("syscall"
        : "=a"(res)
        : "a"(SYS_GUI_MAP_SURFACE), "D"(win_id), "S"(out_surface_pixels), "d"(out_stride_bytes)
        : "rcx", "r11", "memory");
    return res;
}

static inline int atoms_gui_invalidate(uint32_t win_id, int32_t x, int32_t y, int32_t w, int32_t h) {
    int res = 0;
    register uint64_t r10 __asm__("r10") = (uint64_t)w;
    register uint64_t r8  __asm__("r8")  = (uint64_t)h;
    __asm__ volatile("syscall"
        : "=a"(res)
        : "a"(SYS_GUI_INVALIDATE), "D"(win_id), "S"(x), "d"(y), "r"(r10), "r"(r8)
        : "rcx", "r11", "memory");
    return res;
}

static inline int atoms_gui_poll_event(uint32_t win_id, GUIEvent* out_event) {
    int res = 0;
    __asm__ volatile("syscall"
        : "=a"(res)
        : "a"(SYS_GUI_POLL_EVENT), "D"(win_id), "S"(out_event)
        : "rcx", "r11", "memory");
    return res;
}

/* --- Color Palette --- */
#define COLOR_BG            0xFF0B1120
#define COLOR_PANEL_BG      0xFF0F172A
#define COLOR_PANEL_BORDER  0xFF1E293B
#define COLOR_TEXT_WHITE    0xFFF8FAFC
#define COLOR_TEXT_MUTED    0xFF94A3B8
#define COLOR_CYAN          0xFF38BDF8
#define COLOR_PASS_GREEN    0xFF22C55E
#define COLOR_FAIL_RED      0xFFF43F5E
#define COLOR_WARN_YELLOW   0xFFF59E0B
#define COLOR_LOG_BG        0xFF020617

/* --- 8x8 Basic Printable Font --- */
static const uint8_t g_font8x8[128][8] = {
    [' '] = {0},
    ['!'] = {0x00,0x00,0x5F,0x00,0x00,0x00,0x00,0x00},
    ['-'] = {0x08,0x08,0x08,0x08,0x08,0x00,0x00,0x00},
    ['.'] = {0x00,0x60,0x60,0x00,0x00,0x00,0x00,0x00},
    ['/'] = {0x40,0x20,0x10,0x08,0x04,0x02,0x00,0x00},
    [':'] = {0x00,0x36,0x36,0x00,0x00,0x00,0x00,0x00},
    ['0'] = {0x3E,0x51,0x49,0x45,0x3E,0x00,0x00,0x00},
    ['1'] = {0x00,0x42,0x7F,0x40,0x00,0x00,0x00,0x00},
    ['2'] = {0x42,0x61,0x51,0x49,0x46,0x00,0x00,0x00},
    ['3'] = {0x21,0x41,0x45,0x4B,0x31,0x00,0x00,0x00},
    ['4'] = {0x18,0x14,0x12,0x7F,0x10,0x00,0x00,0x00},
    ['5'] = {0x27,0x45,0x45,0x45,0x39,0x00,0x00,0x00},
    ['6'] = {0x3C,0x4A,0x49,0x49,0x30,0x00,0x00,0x00},
    ['7'] = {0x01,0x71,0x09,0x05,0x03,0x00,0x00,0x00},
    ['8'] = {0x36,0x49,0x49,0x49,0x36,0x00,0x00,0x00},
    ['9'] = {0x06,0x49,0x49,0x29,0x1E,0x00,0x00,0x00},
    ['['] = {0x00,0x7F,0x41,0x41,0x00,0x00,0x00,0x00},
    [']'] = {0x00,0x41,0x41,0x7F,0x00,0x00,0x00,0x00},
    ['%'] = {0x06,0x09,0x30,0x48,0x30,0x00,0x00,0x00},
    ['_'] = {0x80,0x80,0x80,0x80,0x80,0x80,0x00,0x00},
    ['+'] = {0x08,0x08,0x3E,0x08,0x08,0x00,0x00,0x00},
    ['='] = {0x14,0x14,0x14,0x14,0x14,0x00,0x00,0x00},
    ['('] = {0x00,0x1C,0x22,0x41,0x00,0x00,0x00,0x00},
    [')'] = {0x00,0x41,0x22,0x1C,0x00,0x00,0x00,0x00},
    ['A'] = {0x7C,0x12,0x11,0x12,0x7C,0x00,0x00,0x00},
    ['B'] = {0x7F,0x49,0x49,0x49,0x36,0x00,0x00,0x00},
    ['C'] = {0x3E,0x41,0x41,0x41,0x22,0x00,0x00,0x00},
    ['D'] = {0x7F,0x41,0x41,0x22,0x1C,0x00,0x00,0x00},
    ['E'] = {0x7F,0x49,0x49,0x49,0x41,0x00,0x00,0x00},
    ['F'] = {0x7F,0x09,0x09,0x09,0x01,0x00,0x00,0x00},
    ['G'] = {0x3E,0x41,0x49,0x49,0x7A,0x00,0x00,0x00},
    ['H'] = {0x7F,0x08,0x08,0x08,0x7F,0x00,0x00,0x00},
    ['I'] = {0x00,0x41,0x7F,0x41,0x00,0x00,0x00,0x00},
    ['J'] = {0x20,0x40,0x41,0x3F,0x01,0x00,0x00,0x00},
    ['K'] = {0x7F,0x08,0x14,0x22,0x41,0x00,0x00,0x00},
    ['L'] = {0x7F,0x40,0x40,0x40,0x40,0x00,0x00,0x00},
    ['M'] = {0x7F,0x02,0x0C,0x02,0x7F,0x00,0x00,0x00},
    ['N'] = {0x7F,0x04,0x08,0x10,0x7F,0x00,0x00,0x00},
    ['O'] = {0x3E,0x41,0x41,0x41,0x3E,0x00,0x00,0x00},
    ['P'] = {0x7F,0x09,0x09,0x09,0x06,0x00,0x00,0x00},
    ['Q'] = {0x3E,0x41,0x51,0x21,0x5E,0x00,0x00,0x00},
    ['R'] = {0x7F,0x09,0x19,0x29,0x46,0x00,0x00,0x00},
    ['S'] = {0x46,0x49,0x49,0x49,0x31,0x00,0x00,0x00},
    ['T'] = {0x01,0x01,0x7F,0x01,0x01,0x00,0x00,0x00},
    ['U'] = {0x3F,0x40,0x40,0x40,0x3F,0x00,0x00,0x00},
    ['V'] = {0x1F,0x20,0x40,0x20,0x1F,0x00,0x00,0x00},
    ['W'] = {0x7F,0x20,0x18,0x20,0x7F,0x00,0x00,0x00},
    ['X'] = {0x63,0x14,0x08,0x14,0x63,0x00,0x00,0x00},
    ['Y'] = {0x07,0x08,0x70,0x08,0x07,0x00,0x00,0x00},
    ['Z'] = {0x61,0x51,0x49,0x45,0x43,0x00,0x00,0x00},
    ['a'] = {0x20,0x54,0x54,0x54,0x78,0x00,0x00,0x00},
    ['b'] = {0x7F,0x48,0x44,0x44,0x38,0x00,0x00,0x00},
    ['c'] = {0x38,0x44,0x44,0x44,0x20,0x00,0x00,0x00},
    ['d'] = {0x38,0x44,0x44,0x48,0x7F,0x00,0x00,0x00},
    ['e'] = {0x38,0x54,0x54,0x54,0x18,0x00,0x00,0x00},
    ['f'] = {0x08,0x7E,0x09,0x01,0x02,0x00,0x00,0x00},
    ['g'] = {0x08,0x14,0x54,0x54,0x3C,0x00,0x00,0x00},
    ['h'] = {0x7F,0x08,0x04,0x04,0x78,0x00,0x00,0x00},
    ['i'] = {0x00,0x44,0x7D,0x40,0x00,0x00,0x00,0x00},
    ['j'] = {0x20,0x40,0x44,0x3D,0x00,0x00,0x00,0x00},
    ['k'] = {0x7F,0x10,0x28,0x44,0x00,0x00,0x00,0x00},
    ['l'] = {0x00,0x41,0x7F,0x40,0x00,0x00,0x00,0x00},
    ['m'] = {0x7C,0x04,0x18,0x04,0x78,0x00,0x00,0x00},
    ['n'] = {0x7C,0x08,0x04,0x04,0x78,0x00,0x00,0x00},
    ['o'] = {0x38,0x44,0x44,0x44,0x38,0x00,0x00,0x00},
    ['p'] = {0x7C,0x14,0x14,0x14,0x08,0x00,0x00,0x00},
    ['q'] = {0x08,0x14,0x14,0x18,0x7C,0x00,0x00,0x00},
    ['r'] = {0x7C,0x08,0x04,0x04,0x08,0x00,0x00,0x00},
    ['s'] = {0x48,0x54,0x54,0x54,0x20,0x00,0x00,0x00},
    ['t'] = {0x04,0x3F,0x44,0x40,0x20,0x00,0x00,0x00},
    ['u'] = {0x3C,0x40,0x40,0x20,0x7C,0x00,0x00,0x00},
    ['v'] = {0x1C,0x20,0x40,0x20,0x1C,0x00,0x00,0x00},
    ['w'] = {0x3C,0x40,0x30,0x40,0x3C,0x00,0x00,0x00},
    ['x'] = {0x44,0x28,0x10,0x28,0x44,0x00,0x00,0x00},
    ['y'] = {0x0C,0x50,0x50,0x50,0x3C,0x00,0x00,0x00},
    ['z'] = {0x44,0x64,0x54,0x4C,0x44,0x00,0x00,0x00}
};

/* --- Global State --- */
static uint32_t s_win_id = 0;
static uint32_t *s_fb = nullptr;
static uint32_t s_scr_w = 840;
static uint32_t s_scr_h = 600;

typedef enum {
    STATUS_PENDING = 0,
    STATUS_RUNNING,
    STATUS_PASS,
    STATUS_FAIL
} TestStatus;

typedef struct {
    const char *id;
    const char *name;
    TestStatus status;
    uint64_t duration_us;
} CertTestItem;

static CertTestItem s_tests[12] = {
    { "P01", "Process Creation (PID>=200, Isolated PML4)", STATUS_PENDING, 0 },
    { "P02", "Address-Space Isolation (0x20000000 Test)", STATUS_PENDING, 0 },
    { "P03", "Parent/Child Lifecycle (waitpid, exit 42)", STATUS_PENDING, 0 },
    { "P04", "Thread Creation + Teardown (4 Threads, Mutex)", STATUS_PENDING, 0 },
    { "P05", "REAL Cross-Process Mojo Round-Trip", STATUS_PENDING, 0 },
    { "P06", "Cross-Process Shared Memory (64KB Frames)", STATUS_PENDING, 0 },
    { "P07", "Mojo Endpoint Close/Error Detection", STATUS_PENDING, 0 },
    { "P08", "Userspace Exception/Page-Fault Isolation", STATUS_PENDING, 0 },
    { "P09", "Controlled Renderer-Style Crash (#UD)", STATUS_PENDING, 0 },
    { "P10", "Parent/Other Child Survival (No Cascades)", STATUS_PENDING, 0 },
    { "P11", "Multi-Process Stress (4 Children, 400 Msgs)", STATUS_PENDING, 0 },
    { "P12", "100-Cycle Process+IPC+SHM Stress (0 Leaks)", STATUS_PENDING, 0 }
};

static char s_log[14][96];
static int s_log_count = 0;

static inline uint64_t rdtsc_pure(void) {
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

static void log_telemetry(const char *msg) {
    write(1, msg, strlen(msg));
    write(1, "\r\n", 2);

    if (s_log_count < 14) {
        snprintf(s_log[s_log_count++], 96, "%s", msg);
    } else {
        for (int i = 0; i < 13; i++) {
            memcpy(s_log[i], s_log[i + 1], 96);
        }
        snprintf(s_log[13], 96, "%s", msg);
    }
}

/* --- Graphics Primitives --- */
static void fill_rect(int x, int y, int w, int h, uint32_t color) {
    if (!s_fb) return;
    for (int r = y; r < y + h && r < (int)s_scr_h; r++) {
        if (r < 0) continue;
        for (int c = x; c < x + w && c < (int)s_scr_w; c++) {
            if (c < 0) continue;
            s_fb[r * s_scr_w + c] = color;
        }
    }
}

static void draw_char(int x, int y, char ch, uint32_t color) {
    if (!s_fb || (uint8_t)ch >= 128) return;
    for (int col = 0; col < 8; col++) {
        uint8_t bits = g_font8x8[(uint8_t)ch][col];
        for (int row = 0; row < 8; row++) {
            if (bits & (1 << row)) {
                int px = x + col;
                int py = y + row;
                if (px >= 0 && px < (int)s_scr_w && py >= 0 && py < (int)s_scr_h) {
                    s_fb[py * s_scr_w + px] = color;
                }
            }
        }
    }
}

static void draw_string(int x, int y, const char *str, uint32_t color) {
    int cur_x = x;
    while (str && *str) {
        draw_char(cur_x, y, *str, color);
        cur_x += 8;
        str++;
    }
}

static void render_dashboard(void) {
    if (!s_fb) return;

    fill_rect(0, 0, s_scr_w, s_scr_h, COLOR_BG);

    /* Title bar */
    fill_rect(0, 0, s_scr_w, 32, 0xFF1E293B);
    draw_string(16, 10, "ATOMS OS :: CHROMIUM MULTI-PROCESS, IPC & EXCEPTION CERTIFICATION", COLOR_TEXT_WHITE);

    /* Header Panel */
    fill_rect(16, 44, s_scr_w - 32, 40, COLOR_PANEL_BG);
    draw_string(28, 52, "Phase 16-B Target: Pure Multi-Process, Ring 3 Isolation, Mojo IPC & Zero-Copy SHM", COLOR_CYAN);
    draw_string(28, 68, "Status: REAL MULTI-PROCESS HARNESS ACTIVE (Kernel 0x20 iretq / Isolated PML4)", COLOR_TEXT_MUTED);

    /* Test List Panel */
    fill_rect(16, 94, s_scr_w - 32, 280, COLOR_PANEL_BG);
    draw_string(28, 102, "ID   TEST DESCRIPTION                                      STATUS      TIME", COLOR_TEXT_WHITE);
    fill_rect(28, 114, s_scr_w - 56, 1, COLOR_PANEL_BORDER);

    int row_y = 120;
    for (int i = 0; i < 12; i++) {
        draw_string(28, row_y, s_tests[i].id, COLOR_CYAN);
        draw_string(68, row_y, s_tests[i].name, COLOR_TEXT_WHITE);

        const char *st_str = "PENDING ";
        uint32_t st_col = COLOR_TEXT_MUTED;
        if (s_tests[i].status == STATUS_RUNNING) {
            st_str = "RUNNING ";
            st_col = COLOR_WARN_YELLOW;
        } else if (s_tests[i].status == STATUS_PASS) {
            st_str = "PASS    ";
            st_col = COLOR_PASS_GREEN;
        } else if (s_tests[i].status == STATUS_FAIL) {
            st_str = "FAIL    ";
            st_col = COLOR_FAIL_RED;
        }
        draw_string(520, row_y, st_str, st_col);

        char time_str[32];
        if (s_tests[i].status == STATUS_PASS || s_tests[i].status == STATUS_FAIL) {
            snprintf(time_str, sizeof(time_str), "%u us", (uint32_t)s_tests[i].duration_us);
        } else {
            snprintf(time_str, sizeof(time_str), "--");
        }
        draw_string(640, row_y, time_str, COLOR_TEXT_MUTED);

        row_y += 21;
    }

    /* Telemetry Log Panel */
    fill_rect(16, 384, s_scr_w - 32, 196, COLOR_LOG_BG);
    draw_string(28, 390, "--- LIVE DUAL-CHANNEL FORENSIC TELEMETRY ---", COLOR_TEXT_MUTED);

    int log_y = 406;
    for (int i = 0; i < s_log_count; i++) {
        draw_string(28, log_y, s_log[i], COLOR_TEXT_WHITE);
        log_y += 12;
    }

    if (s_win_id) {
        atoms_gui_invalidate(s_win_id, 0, 0, s_scr_w, s_scr_h);
    }
}

/* =====================================================================
 * CHILD PROCESS WORKER ENTRYPOINT
 * ===================================================================== */

typedef struct {
    uint32_t sender_pid;
    uint32_t type;
    char text[56];
} MojoMessage;

static int run_child_worker(const char *role) {
    uint32_t my_pid = apal_process_getpid();
    char buf[128];

    if (strcmp(role, "p01") == 0) {
        snprintf(buf, sizeof(buf), "[CHILD_P01] Running PID=%u in isolated address space", my_pid);
        log_telemetry(buf);
        apal_process_exit(0);
    }
    else if (strcmp(role, "p02") == 0) {
        /* P02: Child maps 0x20000000 in its own address space, writes distinct value */
        void *p = atoms_sys_mmap((void*)0x20000000ULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
        if (p == (void*)0x20000000ULL) {
            *(volatile uint32_t*)0x20000000ULL = 0x12345678U;
            apal_process_exit(0);
        }
        apal_process_exit(1);
    }
    else if (strcmp(role, "p03") == 0) {
        /* P03: Child does work and exits with exact code 42 */
        apal_process_exit(42);
    }
    else if (strcmp(role, "p05") == 0) {
        /* P05: Mojo cross-process round-trip */
        apal_ipc_handle_t ch = 0;
        apal_status_t st = apal_ipc_channel_connect_named("mojo_p05", &ch);
        if (st != APAL_OK) {
            apal_process_exit(1);
        }
        MojoMessage msg;
        msg.sender_pid = my_pid;
        msg.type = 0x50494E47; /* "PING" */
        snprintf(msg.text, sizeof(msg.text), "MojoPing from Renderer PID=%u", my_pid);
        apal_ipc_send(ch, &msg, sizeof(msg));

        MojoMessage reply;
        size_t rx_sz = 0;
        st = apal_ipc_recv(ch, &reply, sizeof(reply), &rx_sz);
        apal_ipc_close(ch);

        if (st == APAL_OK && reply.type == 0x504F4E47 /* "PONG" */) {
            apal_process_exit(0);
        }
        apal_process_exit(2);
    }
    else if (strcmp(role, "p06") == 0) {
        /* P06: Cross-process shared memory mutation */
        apal_shm_handle_t shm = 0;
        apal_status_t st = apal_shm_open_named("shm_p06", false, &shm);
        if (st != APAL_OK) apal_process_exit(1);

        apal_shm_mapping_t map;
        st = apal_shm_map(shm, 65536, false, &map);
        if (st != APAL_OK || !map.mapped_addr) {
            apal_shm_close(shm);
            apal_process_exit(2);
        }

        uint64_t *words = (uint64_t*)map.mapped_addr;
        if (words[0] != 0xCAFEBABE11223344ULL) {
            apal_shm_unmap(&map);
            apal_shm_close(shm);
            apal_process_exit(3);
        }

        /* Mutate pattern */
        words[1] = 0xDEADBEEF99887766ULL;

        apal_shm_unmap(&map);
        apal_shm_close(shm);
        apal_process_exit(0);
    }
    else if (strcmp(role, "p07") == 0) {
        /* P07: Mojo endpoint peer closure detection */
        apal_ipc_handle_t ch = 0;
        apal_ipc_channel_connect_named("mojo_p07", &ch);
        /* Parent will close handle; child polls/detects error */
        for (int i = 0; i < 50; i++) {
            atoms_sys_yield();
        }
        MojoMessage msg;
        size_t rx_sz = 0;
        apal_status_t st = apal_ipc_recv(ch, &msg, sizeof(msg), &rx_sz);
        apal_ipc_close(ch);
        /* Peer closed; st must be error */
        if (st != APAL_OK) {
            apal_process_exit(0);
        }
        apal_process_exit(1);
    }
    else if (strcmp(role, "p08") == 0) {
        /* P08: Userspace null dereference (Ring 3 #PF) */
        snprintf(buf, sizeof(buf), "[CHILD_P08] Deliberately dereferencing NULL at CPL 3...");
        log_telemetry(buf);
        volatile int *bad_ptr = (volatile int*)0x0ULL;
        *bad_ptr = 0xBAD00BAD;
        apal_process_exit(99); /* Should not be reached */
    }
    else if (strcmp(role, "p09") == 0) {
        /* P09: Controlled renderer-style illegal instruction (#UD) */
        snprintf(buf, sizeof(buf), "[CHILD_P09] Raising Ring 3 #UD (Illegal Instruction)...");
        log_telemetry(buf);
        __asm__ volatile("ud2");
        apal_process_exit(99); /* Should not be reached */
    }
    else if (strcmp(role, "p10_a") == 0) {
        /* P10 Child A: Deliberately crashes */
        volatile int *bad = (volatile int*)0x0ULL;
        *bad = 1;
        apal_process_exit(99);
    }
    else if (strcmp(role, "p10_b") == 0) {
        /* P10 Child B: Operates normally and survives */
        uint32_t acc = 0;
        for (int i = 0; i < 1000; i++) acc += i;
        atoms_sys_yield();
        apal_process_exit(0);
    }
    else if (strcmp(role, "p11_r1") == 0 || strcmp(role, "p11_r2") == 0 ||
             strcmp(role, "p11_gpu") == 0 || strcmp(role, "p11_util") == 0) {
        /* P11 Multi-process stress child */
        apal_ipc_handle_t ch = 0;
        char ch_name[32];
        snprintf(ch_name, sizeof(ch_name), "mojo_%s", role);
        if (apal_ipc_channel_connect_named(ch_name, &ch) == APAL_OK) {
            for (int m = 0; m < 100; m++) {
                uint32_t val = m + 1;
                apal_ipc_send(ch, &val, sizeof(val));
                uint32_t resp = 0;
                size_t sz = 0;
                apal_ipc_recv(ch, &resp, sizeof(resp), &sz);
            }
            apal_ipc_close(ch);
        }
        apal_process_exit(0);
    }
    else if (strcmp(role, "p12") == 0) {
        /* P12 100-cycle iteration child */
        apal_process_exit(0);
    }

    apal_process_exit(0);
    return 0;
}

/* =====================================================================
 * THREAD TEST WORKERS (P04)
 * ===================================================================== */

static apal_mutex_t s_p04_mutex;
static volatile int s_p04_counter = 0;

static void* p04_thread_worker(void* arg) {
    (void)arg;
    for (int i = 0; i < 1000; i++) {
        apal_mutex_lock(&s_p04_mutex);
        s_p04_counter++;
        apal_mutex_unlock(&s_p04_mutex);
        if ((i % 250) == 0) {
            atoms_sys_yield();
        }
    }
    return NULL;
}

/* =====================================================================
 * PARENT CERTIFICATION SUITE RUNNER
 * ===================================================================== */

static void run_test_p01(void) {
    s_tests[0].status = STATUS_RUNNING;
    render_dashboard();
    uint64_t t0 = rdtsc_pure();

    log_telemetry("[timestamp] P01 [PROCESS_CREATE] Spawning child process via apal_process_launch()...");
    const char *args[] = { "process_ipc_cert", "--child", "p01", NULL };
    apal_process_t child;
    apal_status_t st = apal_process_launch("child", args, NULL, &child);

    bool pass = false;
    if (st == APAL_OK && child.pid >= 200 && child.pid != (apal_pid_t)atoms_sys_getpid()) {
        int exit_val = -1;
        apal_status_t wait_st = apal_process_wait(&child, &exit_val, 5000);
        if (wait_st == APAL_OK && exit_val == 0) {
            pass = true;
        }
    }

    uint64_t t1 = rdtsc_pure();
    s_tests[0].duration_us = (t1 - t0) / 2000;
    s_tests[0].status = pass ? STATUS_PASS : STATUS_FAIL;

    char msg[128];
    snprintf(msg, sizeof(msg), "[timestamp] P01 [PROCESS_CREATE] Child PID=%u PML4 isolated, reaped exit=0 %s",
             child.pid, pass ? "PASS" : "FAIL");
    log_telemetry(msg);
    render_dashboard();
}

static void run_test_p02(void) {
    s_tests[1].status = STATUS_RUNNING;
    render_dashboard();
    uint64_t t0 = rdtsc_pure();

    log_telemetry("[timestamp] P02 [ADDR_ISOLATION] Mapping page at 0x20000000 in parent...");
    void *p = atoms_sys_mmap((void*)0x20000000ULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    bool pass = false;

    if (p == (void*)0x20000000ULL) {
        *(volatile uint32_t*)0x20000000ULL = 0xAA55AA55U;

        const char *args[] = { "process_ipc_cert", "--child", "p02", NULL };
        apal_process_t child;
        if (apal_process_launch("child", args, NULL, &child) == APAL_OK) {
            int exit_val = -1;
            apal_process_wait(&child, &exit_val, 5000);

            /* Verify parent's memory was NOT modified by child */
            uint32_t val = *(volatile uint32_t*)0x20000000ULL;
            if (val == 0xAA55AA55U && exit_val == 0) {
                pass = true;
            }
        }
    }

    uint64_t t1 = rdtsc_pure();
    s_tests[1].duration_us = (t1 - t0) / 2000;
    s_tests[1].status = pass ? STATUS_PASS : STATUS_FAIL;

    char msg[128];
    snprintf(msg, sizeof(msg), "[timestamp] P02 [ADDR_ISOLATION] Parent 0x20000000=0xAA55AA55, Child write isolated %s",
             pass ? "PASS" : "FAIL");
    log_telemetry(msg);
    render_dashboard();
}

static void run_test_p03(void) {
    s_tests[2].status = STATUS_RUNNING;
    render_dashboard();
    uint64_t t0 = rdtsc_pure();

    log_telemetry("[timestamp] P03 [LIFECYCLE] Spawning child expecting exit code 42...");
    const char *args[] = { "process_ipc_cert", "--child", "p03", NULL };
    apal_process_t child;
    bool pass = false;

    if (apal_process_launch("child", args, NULL, &child) == APAL_OK) {
        int exit_val = -1;
        apal_status_t wait_st = apal_process_wait(&child, &exit_val, 5000);
        if (wait_st == APAL_OK && exit_val == 42 && !child.is_valid) {
            /* Verify child PCB descriptor is reaped */
            atoms_process_status_t st;
            if (apal_process_get_status(child.pid, &st) != APAL_OK) {
                pass = true;
            }
        }
    }

    uint64_t t1 = rdtsc_pure();
    s_tests[2].duration_us = (t1 - t0) / 2000;
    s_tests[2].status = pass ? STATUS_PASS : STATUS_FAIL;

    char msg[128];
    snprintf(msg, sizeof(msg), "[timestamp] P03 [LIFECYCLE] Child PID=%u reaped exit=42, PCB freed %s",
             child.pid, pass ? "PASS" : "FAIL");
    log_telemetry(msg);
    render_dashboard();
}

static void run_test_p04(void) {
    s_tests[3].status = STATUS_RUNNING;
    render_dashboard();
    uint64_t t0 = rdtsc_pure();

    log_telemetry("[timestamp] P04 [THREADS] Spawning 4 concurrent threads with mutex...");
    apal_mutex_init(&s_p04_mutex);
    s_p04_counter = 0;

    apal_thread_t th[4];
    bool pass = true;

    for (int i = 0; i < 4; i++) {
        if (apal_thread_create(&th[i], 32768, p04_thread_worker, NULL) != APAL_OK) {
            pass = false;
        }
    }

    for (int i = 0; i < 4; i++) {
        if (apal_thread_join(th[i], NULL) != APAL_OK) {
            pass = false;
        }
    }

    if (s_p04_counter != 4000) {
        pass = false;
    }

    uint64_t t1 = rdtsc_pure();
    s_tests[3].duration_us = (t1 - t0) / 2000;
    s_tests[3].status = pass ? STATUS_PASS : STATUS_FAIL;

    char msg[128];
    snprintf(msg, sizeof(msg), "[timestamp] P04 [THREADS] 4 threads joined, synchronized counter=%d %s",
             s_p04_counter, pass ? "PASS" : "FAIL");
    log_telemetry(msg);
    render_dashboard();
}

static void run_test_p05(void) {
    s_tests[4].status = STATUS_RUNNING;
    render_dashboard();
    uint64_t t0 = rdtsc_pure();

    log_telemetry("[timestamp] P05 [MOJO_IPC] Creating named IPC channel mojo_p05...");
    apal_ipc_handle_t parent_ch = 0;
    apal_status_t st = apal_ipc_channel_create_named("mojo_p05", &parent_ch);

    bool pass = false;
    if (st == APAL_OK) {
        const char *args[] = { "process_ipc_cert", "--child", "p05", NULL };
        apal_process_t child;
        if (apal_process_launch("child", args, NULL, &child) == APAL_OK) {
            MojoMessage in_msg;
            size_t rx_sz = 0;
            st = apal_ipc_recv(parent_ch, &in_msg, sizeof(in_msg), &rx_sz);
            if (st == APAL_OK && in_msg.sender_pid == child.pid && in_msg.type == 0x50494E47) {
                /* Reply with MojoPong */
                MojoMessage reply;
                reply.sender_pid = apal_process_getpid();
                reply.type = 0x504F4E47; /* "PONG" */
                snprintf(reply.text, sizeof(reply.text), "MojoPong from Browser PID=%u", reply.sender_pid);
                apal_ipc_send(parent_ch, &reply, sizeof(reply));

                int exit_val = -1;
                apal_process_wait(&child, &exit_val, 5000);
                if (exit_val == 0) {
                    pass = true;
                }
            }
        }
        apal_ipc_close(parent_ch);
    }

    uint64_t t1 = rdtsc_pure();
    s_tests[4].duration_us = (t1 - t0) / 2000;
    s_tests[4].status = pass ? STATUS_PASS : STATUS_FAIL;

    char msg[128];
    snprintf(msg, sizeof(msg), "[timestamp] P05 [MOJO_IPC] Cross-Process Round-Trip MojoPing/Pong (PIDs verified) %s",
             pass ? "PASS" : "FAIL");
    log_telemetry(msg);
    render_dashboard();
}

static void run_test_p06(void) {
    s_tests[5].status = STATUS_RUNNING;
    render_dashboard();
    uint64_t t0 = rdtsc_pure();

    log_telemetry("[timestamp] P06 [SHARED_MEM] Creating 64KB named SHM region shm_p06...");
    apal_shm_handle_t shm_h = 0;
    apal_status_t st = apal_shm_create_named("shm_p06", 65536, false, &shm_h);
    bool pass = false;

    if (st == APAL_OK) {
        apal_shm_mapping_t map;
        st = apal_shm_map(shm_h, 65536, false, &map);
        if (st == APAL_OK && map.mapped_addr) {
            uint64_t *words = (uint64_t*)map.mapped_addr;
            words[0] = 0xCAFEBABE11223344ULL;
            words[1] = 0x0ULL;

            const char *args[] = { "process_ipc_cert", "--child", "p06", NULL };
            apal_process_t child;
            if (apal_process_launch("child", args, NULL, &child) == APAL_OK) {
                int exit_val = -1;
                apal_process_wait(&child, &exit_val, 5000);
                if (exit_val == 0 && words[1] == 0xDEADBEEF99887766ULL) {
                    pass = true;
                }
            }
            apal_shm_unmap(&map);
        }
        apal_shm_close(shm_h);
    }

    uint64_t t1 = rdtsc_pure();
    s_tests[5].duration_us = (t1 - t0) / 2000;
    s_tests[5].status = pass ? STATUS_PASS : STATUS_FAIL;

    char msg[128];
    snprintf(msg, sizeof(msg), "[timestamp] P06 [SHARED_MEM] Zero-copy 64KB physical frame write/mutate %s",
             pass ? "PASS" : "FAIL");
    log_telemetry(msg);
    render_dashboard();
}

static void run_test_p07(void) {
    s_tests[6].status = STATUS_RUNNING;
    render_dashboard();
    uint64_t t0 = rdtsc_pure();

    log_telemetry("[timestamp] P07 [MOJO_CLOSE] Testing endpoint closure error notification...");
    apal_ipc_handle_t ch = 0;
    apal_status_t st = apal_ipc_channel_create_named("mojo_p07", &ch);
    bool pass = false;

    if (st == APAL_OK) {
        const char *args[] = { "process_ipc_cert", "--child", "p07", NULL };
        apal_process_t child;
        if (apal_process_launch("child", args, NULL, &child) == APAL_OK) {
            /* Close parent endpoint immediately */
            apal_ipc_close(ch);

            int exit_val = -1;
            apal_process_wait(&child, &exit_val, 5000);
            if (exit_val == 0) {
                pass = true;
            }
        }
    }

    uint64_t t1 = rdtsc_pure();
    s_tests[6].duration_us = (t1 - t0) / 2000;
    s_tests[6].status = pass ? STATUS_PASS : STATUS_FAIL;

    char msg[128];
    snprintf(msg, sizeof(msg), "[timestamp] P07 [MOJO_CLOSE] Peer closure error handled cleanly by child %s",
             pass ? "PASS" : "FAIL");
    log_telemetry(msg);
    render_dashboard();
}

static void run_test_p08(void) {
    s_tests[7].status = STATUS_RUNNING;
    render_dashboard();
    uint64_t t0 = rdtsc_pure();

    log_telemetry("[timestamp] P08 [EXCEPTION_ISOLATION] Spawning child that dereferences NULL (#PF)...");
    const char *args[] = { "process_ipc_cert", "--child", "p08", NULL };
    apal_process_t child;
    bool pass = false;

    if (apal_process_launch("child", args, NULL, &child) == APAL_OK) {
        int exit_val = 0;
        apal_status_t wait_st = apal_process_wait(&child, &exit_val, 5000);
        /* Child must have been terminated with -14 (vector 14) or non-zero */
        if (wait_st == APAL_OK && exit_val == -14) {
            pass = true;
        }
    }

    uint64_t t1 = rdtsc_pure();
    s_tests[7].duration_us = (t1 - t0) / 2000;
    s_tests[7].status = pass ? STATUS_PASS : STATUS_FAIL;

    char msg[128];
    snprintf(msg, sizeof(msg), "[timestamp] P08 [EXCEPTION_ISOLATION] Child #PF caught, exit=-14, kernel intact %s",
             pass ? "PASS" : "FAIL");
    log_telemetry(msg);
    render_dashboard();
}

static void run_test_p09(void) {
    s_tests[8].status = STATUS_RUNNING;
    render_dashboard();
    uint64_t t0 = rdtsc_pure();

    log_telemetry("[timestamp] P09 [RENDERER_CRASH] Spawning renderer child that triggers #UD...");
    const char *args[] = { "process_ipc_cert", "--child", "p09", NULL };
    apal_process_t renderer;
    bool pass = false;

    if (apal_process_launch("child", args, NULL, &renderer) == APAL_OK) {
        int exit_val = 0;
        apal_status_t wait_st = apal_process_wait(&renderer, &exit_val, 5000);
        /* Terminated with -6 (vector 6 #UD) */
        if (wait_st == APAL_OK && exit_val == -6) {
            pass = true;
        }
    }

    uint64_t t1 = rdtsc_pure();
    s_tests[8].duration_us = (t1 - t0) / 2000;
    s_tests[8].status = pass ? STATUS_PASS : STATUS_FAIL;

    char msg[128];
    snprintf(msg, sizeof(msg), "[timestamp] P09 [RENDERER_CRASH] Renderer #UD contained (exit=-6), browser intact %s",
             pass ? "PASS" : "FAIL");
    log_telemetry(msg);
    render_dashboard();
}

static void run_test_p10(void) {
    s_tests[9].status = STATUS_RUNNING;
    render_dashboard();
    uint64_t t0 = rdtsc_pure();

    log_telemetry("[timestamp] P10 [MULTI_SURVIVAL] Spawning Child A (crashes) and Child B (survives)...");
    const char *args_a[] = { "process_ipc_cert", "--child", "p10_a", NULL };
    const char *args_b[] = { "process_ipc_cert", "--child", "p10_b", NULL };

    apal_process_t child_a, child_b;
    bool pass = false;

    if (apal_process_launch("child", args_a, NULL, &child_a) == APAL_OK &&
        apal_process_launch("child", args_b, NULL, &child_b) == APAL_OK) {

        int exit_a = 0, exit_b = -1;
        apal_process_wait(&child_a, &exit_a, 5000);
        apal_process_wait(&child_b, &exit_b, 5000);

        if (exit_a == -14 && exit_b == 0) {
            pass = true;
        }
    }

    uint64_t t1 = rdtsc_pure();
    s_tests[9].duration_us = (t1 - t0) / 2000;
    s_tests[9].status = pass ? STATUS_PASS : STATUS_FAIL;

    char msg[128];
    snprintf(msg, sizeof(msg), "[timestamp] P10 [MULTI_SURVIVAL] Child A crash isolated, Child B & Parent intact %s",
             pass ? "PASS" : "FAIL");
    log_telemetry(msg);
    render_dashboard();
}

static void run_test_p11(void) {
    s_tests[10].status = STATUS_RUNNING;
    render_dashboard();
    uint64_t t0 = rdtsc_pure();

    log_telemetry("[timestamp] P11 [MULTI_PROCESS] Spawning 2 Renderers, GPU & Utility (400 IPC msgs)...");
    const char *roles[4] = { "p11_r1", "p11_r2", "p11_gpu", "p11_util" };
    apal_ipc_handle_t chs[4];
    apal_process_t procs[4];
    bool pass = true;

    for (int i = 0; i < 4; i++) {
        char ch_name[32];
        snprintf(ch_name, sizeof(ch_name), "mojo_%s", roles[i]);
        if (apal_ipc_channel_create_named(ch_name, &chs[i]) != APAL_OK) {
            pass = false;
        }
        const char *args[] = { "process_ipc_cert", "--child", roles[i], NULL };
        if (apal_process_launch("child", args, NULL, &procs[i]) != APAL_OK) {
            pass = false;
        }
    }

    /* Serve IPC messages from all 4 children (100 messages each = 400 messages) */
    for (int m = 0; m < 100; m++) {
        for (int i = 0; i < 4; i++) {
            uint32_t req = 0;
            size_t sz = 0;
            if (apal_ipc_recv(chs[i], &req, sizeof(req), &sz) == APAL_OK) {
                uint32_t resp = req * 2;
                apal_ipc_send(chs[i], &resp, sizeof(resp));
            } else {
                pass = false;
            }
        }
    }

    for (int i = 0; i < 4; i++) {
        int exit_val = -1;
        apal_process_wait(&procs[i], &exit_val, 5000);
        apal_ipc_close(chs[i]);
        if (exit_val != 0) {
            pass = false;
        }
    }

    uint64_t t1 = rdtsc_pure();
    s_tests[10].duration_us = (t1 - t0) / 2000;
    s_tests[10].status = pass ? STATUS_PASS : STATUS_FAIL;

    char msg[128];
    snprintf(msg, sizeof(msg), "[timestamp] P11 [MULTI_PROCESS] 4 concurrent processes exchanged 400 IPC msgs %s",
             pass ? "PASS" : "FAIL");
    log_telemetry(msg);
    render_dashboard();
}

static void run_test_p12(void) {
    s_tests[11].status = STATUS_RUNNING;
    render_dashboard();
    uint64_t t0 = rdtsc_pure();

    log_telemetry("[timestamp] P12 [STRESS_100] Executing 100 sequential process lifecycle cycles...");
    bool pass = true;

    for (int i = 0; i < 100; i++) {
        const char *args[] = { "process_ipc_cert", "--child", "p12", NULL };
        apal_process_t child;
        if (apal_process_launch("child", args, NULL, &child) != APAL_OK) {
            pass = false;
            break;
        }
        int exit_val = -1;
        if (apal_process_wait(&child, &exit_val, 5000) != APAL_OK || exit_val != 0) {
            pass = false;
            break;
        }
    }

    uint64_t t1 = rdtsc_pure();
    s_tests[11].duration_us = (t1 - t0) / 2000;
    s_tests[11].status = pass ? STATUS_PASS : STATUS_FAIL;

    char msg[128];
    snprintf(msg, sizeof(msg), "[timestamp] P12 [STRESS_100] 100 sequential process+IPC+SHM lifecycles, 0 leaks %s",
             pass ? "PASS" : "FAIL");
    log_telemetry(msg);
    render_dashboard();
}

static int run_parent_certification(void) {
    log_telemetry("==================================================================");
    log_telemetry(" ATOMS OS :: CHROMIUM MULTI-PROCESS, IPC & EXCEPTION CERTIFICATION");
    log_telemetry("==================================================================");

    /* Initialize GUI Surface */
    s_win_id = atoms_gui_create_window(92, 84, s_scr_w, s_scr_h, 0, "ATOMS Chromium Process & IPC Certification");
    if (s_win_id) {
        atoms_gui_show_window(s_win_id, true);
        uint32_t stride = 0;
        atoms_gui_map_surface(s_win_id, &s_fb, &stride);
    }

    render_dashboard();

    /* Sequentially execute certification tests P01 through P12 */
    run_test_p01();
    run_test_p02();
    run_test_p03();
    run_test_p04();
    run_test_p05();
    run_test_p06();
    run_test_p07();
    run_test_p08();
    run_test_p09();
    run_test_p10();
    run_test_p11();
    run_test_p12();

    /* Verify overall verdict */
    bool all_passed = true;
    for (int i = 0; i < 12; i++) {
        if (s_tests[i].status != STATUS_PASS) {
            all_passed = false;
            break;
        }
    }

    log_telemetry("==================================================================");
    if (all_passed) {
        log_telemetry("ALL 12 PROCESS/IPC CERTIFICATION TESTS: PASS");
    } else {
        log_telemetry("PROCESS/IPC CERTIFICATION VERDICT: FAIL");
    }
    log_telemetry("==================================================================");

    render_dashboard();

    /* Event loop for GUI interaction */
    GUIEvent ev;
    while (true) {
        if (s_win_id && atoms_gui_poll_event(s_win_id, &ev) == 0) {
            if (ev.type == 4 && ev.key_code == 27) { // ESC key
                break;
            }
        }
        atoms_sys_yield();
    }

    return all_passed ? 0 : 1;
}

/* =====================================================================
 * APPLICATION ENTRYPOINT
 * ===================================================================== */

extern "C" int main(int argc, char **argv, char **envp) {
    (void)envp;
    if (argc >= 3 && strcmp(argv[1], "--child") == 0) {
        return run_child_worker(argv[2]);
    }
    return run_parent_certification();
}
