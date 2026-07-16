#include "vmmouse.h"
#include "kernel/drivers/display/display.h"

#define BDOOR_MAGIC 0x564D5868
#define BDOOR_PORT  0x5658

#define BDOOR_CMD_GETVERSION        10
#define BDOOR_CMD_ABSPOINTER_DATA   39
#define BDOOR_CMD_ABSPOINTER_STATUS 40
#define BDOOR_CMD_ABSPOINTER_COMMAND 41

#define VMMOUSE_CMD_READ_ID          0x45414552
#define VMMOUSE_CMD_DISABLE          0x000000f5
#define VMMOUSE_CMD_REQUEST_ABSOLUTE 0x53424152
#define VMMOUSE_CMD_REQUEST_RELATIVE 0x4c455252

typedef struct {
    uint64_t eax, ebx, ecx, edx, esi, edi;
} bdoor_regs_t;

static void bdoor_in(bdoor_regs_t *r) {
    __asm__ volatile (
        "inl %%dx, %%eax"
        : "=a"(r->eax), "=b"(r->ebx), "=c"(r->ecx), "=d"(r->edx), "=S"(r->esi), "=D"(r->edi)
        : "a"(r->eax), "b"(r->ebx), "c"(r->ecx), "d"(r->edx), "S"(r->esi), "D"(r->edi)
        : "memory"
    );
}


static bool g_vmmouse_active = false;
static uint32_t g_screen_w = 1280;
static uint32_t g_screen_h = 720;

static void serial_write_hex_direct(uint32_t val);

void vmmouse_update_resolution(uint32_t screen_width, uint32_t screen_height) {
    g_screen_w = (screen_width > 0) ? screen_width : 1280;
    g_screen_h = (screen_height > 0) ? screen_height : 720;
}

void vmmouse_get_bounds(uint32_t* out_w, uint32_t* out_h) {
    if (out_w) *out_w = g_screen_w;
    if (out_h) *out_h = g_screen_h;
}

bool vmmouse_init(uint32_t screen_width, uint32_t screen_height) {
    g_screen_w = screen_width;
    g_screen_h = screen_height;
    g_vmmouse_active = false;

    // 1. Check if VMware backdoor is available (Get Version)
    bdoor_regs_t r = {0};
    r.eax = BDOOR_MAGIC;
    r.ecx = BDOOR_CMD_GETVERSION;
    r.edx = BDOOR_PORT;
    bdoor_in(&r);
    
    display_print("[VMMOUSE] Backdoor GetVersion: EAX=");
    {
        serial_write_hex_direct(r.eax);
        display_print(" EBX=");
        serial_write_hex_direct(r.ebx);
        display_print(" ECX=");
        serial_write_hex_direct(r.ecx);
        display_print(" EDX=");
        serial_write_hex_direct(r.edx);
        display_print("\n");
    }
    
    if (r.ebx != BDOOR_MAGIC && r.eax == 0xFFFFFFFF) {
        display_print("[VMMOUSE] VMware backdoor not found.\n");
        return false;
    }

    // 2. Read VMMouse ID
    r.eax = BDOOR_MAGIC;
    r.ebx = VMMOUSE_CMD_READ_ID;
    r.ecx = BDOOR_CMD_ABSPOINTER_COMMAND;
    r.edx = BDOOR_PORT;
    bdoor_in(&r);
    
    display_print("[VMMOUSE] Backdoor ReadID: EAX=");
    {
        serial_write_hex_direct(r.eax);
        display_print(" EBX=");
        serial_write_hex_direct(r.ebx);
        display_print("\n");
    }
    
    // Check if VMMouse is present (version 0x3442554A 'JUB4' or 0x3542554A 'JUB5')
    if (r.eax == 0xFFFFFFFF || r.eax == 0) {
        display_print("[VMMOUSE] VMMouse device not present on backdoor.\n");
        return false;
    }

    display_print("[VMMOUSE] VMMouse detected via backdoor!\n");

    // 3. Request absolute mode
    r.eax = BDOOR_MAGIC;
    r.ebx = VMMOUSE_CMD_REQUEST_ABSOLUTE;
    r.ecx = BDOOR_CMD_ABSPOINTER_COMMAND;
    r.edx = BDOOR_PORT;
    bdoor_in(&r);

    display_print("[VMMOUSE] Absolute mode ENABLED.\n");
    g_vmmouse_active = true;
    return true;
}

bool vmmouse_is_active(void) {
    return g_vmmouse_active;
}

static void serial_write_hex_direct(uint32_t val) {
    extern void serial_write_direct(const char* str);
    char hex_chars[] = "0123456789ABCDEF";
    char buf[11];
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 0; i < 8; i++) {
        buf[9 - i] = hex_chars[(val >> (i * 4)) & 0x0F];
    }
    buf[10] = '\0';
    serial_write_direct(buf);
}

volatile uint64_t g_vmmouse_read_count = 0;

bool vmmouse_read(int32_t* abs_x, int32_t* abs_y, uint8_t* buttons) {
    g_vmmouse_read_count++;
    if (!g_vmmouse_active) return false;

    extern void serial_write_direct(const char* str);
    extern void serial_write_dec_direct(int val);

    // 1. Check status
    bdoor_regs_t r = {0};
    r.eax = BDOOR_MAGIC;
    r.ebx = 0;
    r.ecx = BDOOR_CMD_ABSPOINTER_STATUS;
    r.edx = BDOOR_PORT;
    bdoor_in(&r);

    uint32_t status_eax = r.eax;

    // EAX returns the number of words available in the queue
    // If < 4, we don't have a full packet (sometimes status bit 0 indicates error)
    if (r.eax == 0xFFFF0000 || (r.eax & 0xFFFF) < 4) {
        static int status_fail_count = 0;
        if (++status_fail_count % 100 == 0) {
            serial_write_direct("[VMMOUSE TRACE] Status fail: EAX=");
            serial_write_hex_direct(status_eax);
            serial_write_direct("\n");
        }
        return false;
    }

    // 2. Read exactly 4 words atomically (1 packet)
    r.eax = BDOOR_MAGIC;
    r.ebx = 4;                           // Request 4 words
    r.ecx = BDOOR_CMD_ABSPOINTER_DATA;   // Command 39 (VMMOUSE_DATA)
    r.edx = BDOOR_PORT;
    bdoor_in(&r);

    uint32_t flags = r.eax;
    uint32_t x_raw = r.ebx; // Word 1 (X) is in EBX
    uint32_t y_raw = r.ecx; // Word 2 (Y) is in ECX
    uint32_t z_raw = r.edx; // Word 3 (Z) is in EDX

    // Scale from 0..0xFFFF to screen pixels
    *abs_x = (int32_t)(((uint64_t)x_raw * g_screen_w) / 0xFFFF);
    *abs_y = (int32_t)(((uint64_t)y_raw * g_screen_h) / 0xFFFF);

    // Clamp
    if (*abs_x < 0) *abs_x = 0;
    if (*abs_x >= (int32_t)g_screen_w) *abs_x = (int32_t)g_screen_w - 1;
    if (*abs_y < 0) *abs_y = 0;
    if (*abs_y >= (int32_t)g_screen_h) *abs_y = (int32_t)g_screen_h - 1;

    // Extract buttons (VMMouse provides button states in flags)
    *buttons = 0;
    if (flags & 0x20) *buttons |= 1; // Left
    if (flags & 0x10) *buttons |= 2; // Right
    if (flags & 0x08) *buttons |= 4; // Middle

    // Tracing trace
    serial_write_direct("[VMMOUSE TRACE] SUCCESS: EAX=");
    serial_write_hex_direct(status_eax);
    serial_write_direct(" Raw: [");
    serial_write_hex_direct(flags);
    serial_write_direct(",");
    serial_write_hex_direct(x_raw);
    serial_write_direct(",");
    serial_write_hex_direct(y_raw);
    serial_write_direct(",");
    serial_write_hex_direct(z_raw);
    serial_write_direct("] Decoded: x=");
    serial_write_dec_direct(*abs_x);
    serial_write_direct(" y=");
    serial_write_dec_direct(*abs_y);
    serial_write_direct(" btns=");
    serial_write_dec_direct(*buttons);
    serial_write_direct("\n");

    return true;
}
