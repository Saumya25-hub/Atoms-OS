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
    uint32_t eax, ebx, ecx, edx, esi, edi;
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

volatile uint64_t g_vmmouse_read_count = 0;

bool vmmouse_read(int32_t* abs_x, int32_t* abs_y, uint8_t* buttons) {
    g_vmmouse_read_count++;
    if (!g_vmmouse_active) return false;

    // 1. Check status
    bdoor_regs_t r = {0};
    r.eax = BDOOR_MAGIC;
    r.ebx = 0;
    r.ecx = BDOOR_CMD_ABSPOINTER_STATUS;
    r.edx = BDOOR_PORT;
    bdoor_in(&r);

    // EAX returns the number of words available in the queue
    // If < 4, we don't have a full packet (sometimes status bit 0 indicates error)
    if (r.eax == 0xFFFF0000 || (r.eax & 0xFFFF) < 4) {
        return false;
    }

    // 2. Read exactly 4 words (1 packet atomically across EAX, EBX, ECX, EDX)
    r.eax = BDOOR_MAGIC;
    r.ebx = 4;                           // Request 4-word atomic packet
    r.ecx = BDOOR_CMD_ABSPOINTER_DATA;   // Command 39 (VMMOUSE_DATA)
    r.edx = BDOOR_PORT;
    bdoor_in(&r);

    uint32_t flags = r.eax;
    uint32_t x_raw = r.ecx; // Assume Candidate B temporarily
    uint32_t y_raw = r.edx; // Assume Candidate B temporarily

    // display_print("\n=== PIPELINE TRACE ===\n(1) VMMouse Raw: X="); display_print_dec((uint64_t)x_raw);
    // display_print(" Y="); display_print_dec((uint64_t)y_raw); display_print("\n");

    // Scale from 0..0xFFFF to screen pixels
    *abs_x = (int32_t)(((uint64_t)x_raw * g_screen_w) / 0xFFFF);
    *abs_y = (int32_t)(((uint64_t)y_raw * g_screen_h) / 0xFFFF);

    // Clamp
    if (*abs_x < 0) *abs_x = 0;
    if (*abs_x >= (int32_t)g_screen_w) *abs_x = (int32_t)g_screen_w - 1;
    if (*abs_y < 0) *abs_y = 0;
    if (*abs_y >= (int32_t)g_screen_h) *abs_y = (int32_t)g_screen_h - 1;

    // display_print("(2) VMMouse Scaled: X="); display_print_dec((uint64_t)*abs_x);
    // display_print(" Y="); display_print_dec((uint64_t)*abs_y); display_print("\n");

    // Extract buttons (VMMouse provides button states in flags)
    // Flags bit mapping is slightly different from PS/2 but we can map it
    // PS/2 buttons: bit 0 = Left, bit 1 = Right, bit 2 = Middle
    // VMMouse flags: 0x20 = Left, 0x10 = Right, 0x08 = Middle
    *buttons = 0;
    if (flags & 0x20) *buttons |= 1; // Left
    if (flags & 0x10) *buttons |= 2; // Right
    if (flags & 0x08) *buttons |= 4; // Middle

    return true;
}
