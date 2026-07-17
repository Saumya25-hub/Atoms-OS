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

    // 1. Check status
    bdoor_regs_t r = {0};
    r.eax = BDOOR_MAGIC;
    r.ebx = 0;
    r.ecx = BDOOR_CMD_ABSPOINTER_STATUS;
    r.edx = BDOOR_PORT;
    bdoor_in(&r);

    // EAX returns status/count. If 0 (no data) or 0xFFFFFFFF (device error/not present), return false.
    if (r.eax == 0xFFFFFFFF || (r.eax & 0xFFFF) == 0) {
        return false;
    }

    // 2. Read exactly 4 words atomically (1 packet)
    r.eax = BDOOR_MAGIC;
    r.ebx = 4;                           // Request 4 words
    r.ecx = BDOOR_CMD_ABSPOINTER_DATA;   // Command 39 (VMMOUSE_DATA)
    r.edx = BDOOR_PORT;
    bdoor_in(&r);

    uint32_t flags, x_raw, y_raw, z_raw;
    if (r.ebx == 4 && r.ecx == BDOOR_CMD_ABSPOINTER_DATA) {
        // Real VMware Workstation / VirtualBox hardware path:
        // `inl` physically only loads EAX per instruction (word 0: flags).
        // We pop word 1 (x), word 2 (y), and word 3 (z) from the VMMOUSE_DATA FIFO
        // with 3 successive INL reads. EBX must remain 4 when reading from command 39!
        flags = r.eax;
        
        r.eax = BDOOR_MAGIC; r.ebx = 4; r.ecx = BDOOR_CMD_ABSPOINTER_DATA; r.edx = BDOOR_PORT;
        bdoor_in(&r);
        x_raw = r.eax;

        r.eax = BDOOR_MAGIC; r.ebx = 4; r.ecx = BDOOR_CMD_ABSPOINTER_DATA; r.edx = BDOOR_PORT;
        bdoor_in(&r);
        y_raw = r.eax;

        r.eax = BDOOR_MAGIC; r.ebx = 4; r.ecx = BDOOR_CMD_ABSPOINTER_DATA; r.edx = BDOOR_PORT;
        bdoor_in(&r);
        z_raw = r.eax;
    } else {
        // QEMU shortcut: all 4 words popped simultaneously into EAX, EBX, ECX, EDX.
        flags = r.eax;
        x_raw = r.ebx;
        y_raw = r.ecx;
        z_raw = r.edx;
    }

    // Output raw 0..0xFFFF coordinates. CCTE will normalize them.
    *abs_x = (int32_t)x_raw;
    *abs_y = (int32_t)y_raw;

    // Extract buttons (VMMouse provides button states in flags)
    *buttons = 0;
    if (flags & 0x20) *buttons |= 1; // Left
    if (flags & 0x10) *buttons |= 2; // Right
    if (flags & 0x08) *buttons |= 4; // Middle

    return true;
}
