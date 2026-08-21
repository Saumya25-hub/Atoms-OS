// vmmouse.c — VMware Backdoor Absolute Pointer Protocol Driver
// Protocol verified against: QEMU hw/i386/vmmouse.c (GitLab master)
// Fixes applied:
//   1. GET_VERSION: correct detection (EBX == BDOOR_MAGIC)
//   2. INIT: drain VERSION dword from queue after READ_ID
//   3. DATA: single atomic call EBX=4, returns EAX=flags EBX=X ECX=Y EDX=Z
//   4. STATUS: correct error check (eax >> 16) == 0xFFFF
//   5. Re-entrancy guard prevents IRQ12 double-poll corruption
//   6. RELATIVE_PACKET flag filter added

#include "vmmouse.h"
#include "kernel/drivers/display/display.h"
#include "kernel/drivers/input/core/hida.h"

// =========================================================================
// VMware Backdoor Protocol Constants
// Source: QEMU hw/i386/vmmouse.c
// =========================================================================
#define BDOOR_MAGIC                 0x564D5868   // 'VMXh'
#define BDOOR_PORT                  0x5658       // 'VX'

#define BDOOR_CMD_GETVERSION        10
#define BDOOR_CMD_ABSPOINTER_DATA   39
#define BDOOR_CMD_ABSPOINTER_STATUS 40
#define BDOOR_CMD_ABSPOINTER_COMMAND 41

// ABSPOINTER_COMMAND sub-commands (EBX)
#define VMMOUSE_CMD_READ_ID          0x45414552   // 'REAE'
#define VMMOUSE_CMD_DISABLE          0x000000F5
#define VMMOUSE_CMD_REQUEST_ABSOLUTE 0x53424152   // 'RABS'
#define VMMOUSE_CMD_REQUEST_RELATIVE 0x4C455252   // 'RREL'

// QEMU source: #define VMMOUSE_VERSION 0x3442554A
#define VMMOUSE_VERSION              0x3442554A   // 'JUB4'

// Packet flags (queue[0] / EAX after DATA call)
// QEMU source: VMMOUSE_LEFT_BUTTON=0x20, RIGHT=0x10, MIDDLE=0x08
#define VMMOUSE_LEFT_BUTTON          0x20
#define VMMOUSE_RIGHT_BUTTON         0x10
#define VMMOUSE_MIDDLE_BUTTON        0x08
// Bit 16 set in flags word = relative mode packet (skip it)
// QEMU source: #define VMMOUSE_RELATIVE_PACKET 0x00010000
#define VMMOUSE_RELATIVE_PACKET      0x00010000

// DATA command: max dwords per single call = 6 (QEMU enforces size <= 6)
// One mouse event = exactly 4 dwords: [flags, X, Y, Z]
#define VMMOUSE_PACKET_SIZE          4

// Protocol logging — enable to see every register exchange
// #define VMMOUSE_PROTO_LOG

// =========================================================================
// Backdoor I/O register structure
// QEMU vmmouse_get_data/set_data maps:
//   data[0]=EAX  data[1]=EBX  data[2]=ECX  data[3]=EDX
//   data[4]=ESI  data[5]=EDI
// =========================================================================
typedef struct {
    uint32_t eax, ebx, ecx, edx, esi, edi;
} bdoor_regs_t;

static inline void bdoor_in(bdoor_regs_t *r) {
    __asm__ volatile (
        "inl %%dx, %%eax"
        : "=a"(r->eax), "=b"(r->ebx), "=c"(r->ecx), "=d"(r->edx),
          "=S"(r->esi), "=D"(r->edi)
        : "a"(r->eax), "b"(r->ebx), "c"(r->ecx), "d"(r->edx),
          "S"(r->esi), "D"(r->edi)
        : "memory"
    );
}

// =========================================================================
// EFLAGS Preservation for critical sections
// =========================================================================
static inline uint64_t vmmouse_irq_save(void) {
    uint64_t flags;
    __asm__ volatile (
        "pushfq\n\t"
        "popq %0\n\t"
        "cli"
        : "=r"(flags)
        :
        : "memory"
    );
    return flags;
}

static inline void vmmouse_irq_restore(uint64_t flags) {
    __asm__ volatile (
        "pushq %0\n\t"
        "popfq"
        :
        : "r"(flags)
        : "memory", "cc"
    );
}

// =========================================================================
// Driver state
// =========================================================================
static bool     g_vmmouse_active    = false;
static uint32_t g_screen_w          = 1280;
static uint32_t g_screen_h          = 720;

// Re-entrancy guard: prevents IRQ12 double-poll that causes
// "vmmouse: driver requested too much data 1"
static volatile bool g_vmmouse_polling = false;

// =========================================================================
// Protocol logging helper (enabled by VMMOUSE_PROTO_LOG)
// =========================================================================
#ifdef VMMOUSE_PROTO_LOG
extern void display_print_hex(uint32_t v);
extern void display_print_dec(uint64_t v);

static void vmmouse_log_regs(const char* label, const bdoor_regs_t* r) {
    display_print("[VMMOUSE PROTO] "); display_print(label); display_print("\n");
    display_print("  EAX=0x"); display_print_hex(r->eax);
    display_print("  EBX=0x"); display_print_hex(r->ebx);
    display_print("  ECX=0x"); display_print_hex(r->ecx);
    display_print("  EDX=0x"); display_print_hex(r->edx); display_print("\n");
}
#define PROTO_LOG(label, r) vmmouse_log_regs(label, r)
#define PROTO_PRINT(s)      display_print(s)
#else
#define PROTO_LOG(label, r)
#define PROTO_PRINT(s)
#endif

// =========================================================================
// Public API
// =========================================================================
void vmmouse_update_resolution(uint32_t screen_width, uint32_t screen_height) {
    g_screen_w = (screen_width  > 0) ? screen_width  : 1280;
    g_screen_h = (screen_height > 0) ? screen_height : 720;
}

void vmmouse_get_bounds(uint32_t* out_w, uint32_t* out_h) {
    if (out_w) *out_w = g_screen_w;
    if (out_h) *out_h = g_screen_h;
}

// =========================================================================
// vmmouse_init — Protocol-correct initialization sequence
//
// Correct sequence (proven against QEMU hw/i386/vmmouse.c):
//
//  Step 1: GET_VERSION  (cmd=10)
//    IN:  EAX=BDOOR_MAGIC, ECX=10, EDX=PORT
//    OUT: EBX must == BDOOR_MAGIC  (QEMU: vmport handler returns magic in EBX)
//
//  Step 2: READ_ID  (ABSPOINTER_COMMAND + EBX=READ_ID)
//    IN:  EAX=BDOOR_MAGIC, EBX=0x45414552, ECX=41, EDX=PORT
//    OUT: QEMU pushes VMMOUSE_VERSION (0x3442554A) → queue[0], nb_queue=1
//         CPU registers unchanged (COMMAND path does NOT modify data[])
//
//  Step 3: STATUS  (cmd=40)
//    IN:  EAX=BDOOR_MAGIC, EBX=0, ECX=40, EDX=PORT
//    OUT: EAX = (status<<16)|nb_queue  →  should be 0x00000001
//
//  Step 4: DATA EBX=1  — drain and verify version word
//    IN:  EAX=BDOOR_MAGIC, EBX=1, ECX=39, EDX=PORT
//    OUT: EAX = queue[0] = VMMOUSE_VERSION (0x3442554A)
//         nb_queue = 0  → queue is now CLEAN
//
//  Step 5: REQUEST_ABSOLUTE  (ABSPOINTER_COMMAND + EBX=REQUEST_ABSOLUTE)
//    IN:  EAX=BDOOR_MAGIC, EBX=0x53424152, ECX=41, EDX=PORT
//    OUT: QEMU enables absolute mode, registers abs handler
//         nb_queue remains 0 (clean queue)
// =========================================================================
bool vmmouse_init(uint32_t screen_width, uint32_t screen_height) {
    g_screen_w      = (screen_width  > 0) ? screen_width  : 1280;
    g_screen_h      = (screen_height > 0) ? screen_height : 720;
    g_vmmouse_active    = false;
    g_vmmouse_polling   = false;

    bdoor_regs_t r = {0};

    // ------------------------------------------------------------------
    // Step 1: GET_VERSION — verify VMware backdoor is present
    // Protocol: EBX must equal BDOOR_MAGIC on return
    // Bug fixed: was using (ebx != MAGIC && eax == 0xFFFFFFFF) which
    //            allows false detection when only one condition is true.
    // ------------------------------------------------------------------
    r.eax = BDOOR_MAGIC;
    r.ebx = 0;
    r.ecx = BDOOR_CMD_GETVERSION;
    r.edx = BDOOR_PORT;
    bdoor_in(&r);
    PROTO_LOG("GET_VERSION OUT", &r);

    if (r.ebx != BDOOR_MAGIC) {
        display_print("[VMMOUSE] VMware backdoor not found.\n");
        return false;
    }

    // ------------------------------------------------------------------
    // Step 2: READ_ID — queue the version identification word
    // QEMU effect: queue[0] = VMMOUSE_VERSION, nb_queue = 1
    // The version dword MUST be drained in Step 4 before runtime reads.
    // ------------------------------------------------------------------
    r.eax = BDOOR_MAGIC;
    r.ebx = VMMOUSE_CMD_READ_ID;
    r.ecx = BDOOR_CMD_ABSPOINTER_COMMAND;
    r.edx = BDOOR_PORT;
    bdoor_in(&r);
    PROTO_LOG("READ_ID OUT (queue now has 1 VERSION dword)", &r);

    // ------------------------------------------------------------------
    // Step 3: STATUS — verify the version word is in the queue
    // Expected: EAX & 0xFFFF == 1
    // ------------------------------------------------------------------
    r.eax = BDOOR_MAGIC;
    r.ebx = 0;
    r.ecx = BDOOR_CMD_ABSPOINTER_STATUS;
    r.edx = BDOOR_PORT;
    bdoor_in(&r);
    PROTO_LOG("STATUS after READ_ID", &r);

    uint32_t nb = r.eax & 0xFFFF;
    if ((r.eax >> 16) == 0xFFFF || nb < 1) {
        display_print("[VMMOUSE] VMMouse not present (no version word in queue).\n");
        return false;
    }

    // ------------------------------------------------------------------
    // Step 4: DATA EBX=1 — drain and verify the version identification word
    //
    // QEMU vmmouse_data with size=1:
    //   data[0] = queue[0]  (returned in EAX)
    //   nb_queue -= 1       → nb_queue = 0  (queue is now CLEAN)
    //
    // This is the ONLY place EBX=1 is correct — reading exactly 1 dword.
    // After this call the queue is empty and ready for clean event reads.
    // ------------------------------------------------------------------
    r.eax = BDOOR_MAGIC;
    r.ebx = 1;                          // read 1 dword (the version word)
    r.ecx = BDOOR_CMD_ABSPOINTER_DATA;
    r.edx = BDOOR_PORT;
    bdoor_in(&r);
    PROTO_LOG("DATA EBX=1 (drain version word)", &r);

    uint32_t version = r.eax;           // EAX = queue[0] = VMMOUSE_VERSION
    if (version != VMMOUSE_VERSION) {
        display_print("[VMMOUSE] VMMouse version mismatch — not a VMware device.\n");
        return false;
    }
    // nb_queue is now 0 — queue is clean for runtime event reads

    display_print("[VMMOUSE] VMMouse detected via backdoor!\n");

    // ------------------------------------------------------------------
    // Step 5: REQUEST_ABSOLUTE — enable absolute coordinate mode
    // QEMU effect: absolute=1, abs handler registered, nb_queue stays 0
    // ------------------------------------------------------------------
    r.eax = BDOOR_MAGIC;
    r.ebx = VMMOUSE_CMD_REQUEST_ABSOLUTE;
    r.ecx = BDOOR_CMD_ABSPOINTER_COMMAND;
    r.edx = BDOOR_PORT;
    bdoor_in(&r);
    PROTO_LOG("REQUEST_ABSOLUTE OUT", &r);

    // Register with HIDA Input Device Arbiter
    InputDeviceDescriptor vm_desc = {0};
    vm_desc.backend_id    = HIDA_BACKEND_VMMOUSE;
    vm_desc.type          = INPUT_DEV_TYPE_VMMOUSE;
    vm_desc.device_name   = "VMware VMMouse Absolute Pointer";
    vm_desc.driver_name   = "vmmouse";
    vm_desc.is_supported  = true;
    vm_desc.is_initialized = true;
    vm_desc.is_connected  = true;
    vm_desc.is_absolute   = true;
    vm_desc.is_polling    = true;
    vm_desc.priority_score = 100;
    vm_desc.health_score  = 100;
    vm_desc.status        = HIDA_STATE_ACTIVE;
    hida_register_device(&vm_desc);

    display_print("[VMMOUSE] Absolute mode ENABLED. Queue is clean (0 residual dwords).\n");
    g_vmmouse_active = true;
    return true;
}

bool vmmouse_is_active(void) {
    return g_vmmouse_active;
}

// =========================================================================
// Diagnostics counters
// =========================================================================
volatile uint64_t g_vmmouse_read_count    = 0;
volatile uint64_t g_vmmouse_irq_count     = 0;
volatile uint64_t g_vmmouse_packets_count = 0;

volatile uint32_t g_vmmouse_last_status   = 0;
volatile int32_t  g_vmmouse_last_raw_x    = 0;
volatile int32_t  g_vmmouse_last_raw_y    = 0;
volatile uint8_t  g_vmmouse_last_buttons  = 0;

// =========================================================================
// vmmouse_read — Single-packet atomic read
//
// Correct DATA sequence (proven against QEMU hw/i386/vmmouse.c):
//
//  Step 1: STATUS (cmd=40)
//    IN:  EAX=BDOOR_MAGIC, EBX=0, ECX=40, EDX=PORT
//    OUT: EAX = (status<<16)|nb_queue
//         If (EAX >> 16) == 0xFFFF → error state, return false
//         If nb_queue < VMMOUSE_PACKET_SIZE → not enough data, return false
//
//  Step 2: DATA (cmd=39), EBX=4  — ONE atomic call for all 4 dwords
//    IN:  EAX=BDOOR_MAGIC, EBX=4, ECX=39, EDX=PORT
//    QEMU vmmouse_data with size=4:
//      data[0] = queue[0] → written to EAX = flags/buttons
//      data[1] = queue[1] → written to EBX = X (0..0xFFFF)
//      data[2] = queue[2] → written to ECX = Y (0..0xFFFF)
//      data[3] = queue[3] → written to EDX = Z/scroll
//      nb_queue -= 4
//    OUT: EAX=flags, EBX=X, ECX=Y, EDX=Z
//
//  Why EBX=4 not EBX=1 x 4:
//    EBX=1 x 4 creates a 4-step window where IRQ12 can re-enter between
//    calls, draining the queue to 0, then our next EBX=1 call sees
//    size(1) > nb_queue(0) → QEMU prints "too much data 1" and sets
//    status=0xFFFF, removing the absolute handler. Cursor disappears.
//    EBX=4 is ONE atomic I/O instruction — IRQ cannot split it.
//
//  Interrupts are disabled for the STATUS→DATA critical section to
//  prevent the re-entrancy race even across separate calls.
// =========================================================================
bool vmmouse_read(int32_t* abs_x, int32_t* abs_y, uint8_t* buttons) {
    g_vmmouse_read_count++;
    if (!g_vmmouse_active) return false;

    // ------------------------------------------------------------------
    // Step 1: STATUS — get current queue depth
    // Disable interrupts to make STATUS+DATA atomic
    // ------------------------------------------------------------------
    uint64_t irq_flags = vmmouse_irq_save();

    bdoor_regs_t r = {0};
    r.eax = BDOOR_MAGIC;
    r.ebx = 0;
    r.ecx = BDOOR_CMD_ABSPOINTER_STATUS;
    r.edx = BDOOR_PORT;
    bdoor_in(&r);

    g_vmmouse_last_status = r.eax;
    uint32_t nb_queue = r.eax & 0xFFFF;

    // Check for QEMU error state: (status << 16) where status == 0xFFFF
    // Bug fixed: was (r.eax == 0xFFFF0000) which misses cases where
    //            nb_queue != 0 (e.g. 0xFFFF0004 was not caught)
    if ((r.eax >> 16) == 0xFFFF) {
        vmmouse_irq_restore(irq_flags);
        PROTO_PRINT("[VMMOUSE] STATUS: error state (0xFFFF in high word)\n");
        return false;
    }

    if (nb_queue < VMMOUSE_PACKET_SIZE) {
        vmmouse_irq_restore(irq_flags);
        return false;   // not enough dwords for a complete packet
    }

    PROTO_LOG("STATUS IN", &r);

    // ------------------------------------------------------------------
    // Step 2: DATA with EBX=4 — read all 4 packet dwords atomically
    //
    // QEMU vmmouse_data(size=4) returns:
    //   EAX = queue[0] = flags  (button bits + optional RELATIVE_PACKET flag)
    //   EBX = queue[1] = X      (0..0xFFFF in absolute mode)
    //   ECX = queue[2] = Y      (0..0xFFFF in absolute mode)
    //   EDX = queue[3] = Z/dz   (scroll wheel delta)
    //   nb_queue -= 4
    // ------------------------------------------------------------------
    r.eax = BDOOR_MAGIC;
    r.ebx = VMMOUSE_PACKET_SIZE;        // 4 — request all dwords in ONE call
    r.ecx = BDOOR_CMD_ABSPOINTER_DATA;
    r.edx = BDOOR_PORT;
    bdoor_in(&r);

    vmmouse_irq_restore(irq_flags);  // interrupts safely restored after atomic STATUS+DATA pair

    PROTO_LOG("DATA EBX=4 OUT", &r);

    // ------------------------------------------------------------------
    // Parse packet from returned registers
    // Register mapping (QEMU vmmouse_data, size=4):
    //   data[0]=EAX = flags  → button state + packet type
    //   data[1]=EBX = X      → absolute X coordinate 0..0xFFFF
    //   data[2]=ECX = Y      → absolute Y coordinate 0..0xFFFF
    //   data[3]=EDX = Z      → scroll wheel delta (ignored here)
    // ------------------------------------------------------------------
    uint32_t flags = r.eax;

    // Filter relative packets — QEMU sets bit 16 of flags for relative
    // This happens if absolute mode was not accepted or during mode switch
    if (flags & VMMOUSE_RELATIVE_PACKET) {
        PROTO_PRINT("[VMMOUSE] Skipping relative packet\n");
        return false;
    }

    uint32_t x_raw = r.ebx;    // X: 0..0xFFFF
    uint32_t y_raw = r.ecx;    // Y: 0..0xFFFF
    // r.edx = scroll delta (available but not consumed here)

    g_vmmouse_last_raw_x    = (int32_t)x_raw;
    g_vmmouse_last_raw_y    = (int32_t)y_raw;

    // Output raw canonical coordinates — CCTE normalizes them downstream
    *abs_x = (int32_t)x_raw;
    *abs_y = (int32_t)y_raw;

    // Button mapping — verified against QEMU source:
    //   VMMOUSE_LEFT_BUTTON   = 0x20
    //   VMMOUSE_RIGHT_BUTTON  = 0x10
    //   VMMOUSE_MIDDLE_BUTTON = 0x08
    *buttons = 0;
    if (flags & VMMOUSE_LEFT_BUTTON)   *buttons |= 1;
    if (flags & VMMOUSE_RIGHT_BUTTON)  *buttons |= 2;
    if (flags & VMMOUSE_MIDDLE_BUTTON) *buttons |= 4;

    g_vmmouse_last_buttons = *buttons;

    hida_report_event_parsed(HIDA_BACKEND_VMMOUSE, true);
    return true;
}

// =========================================================================
// vmmouse_poll — Drain pending packets from backdoor queue
//
// Re-entrancy guard prevents IRQ12-triggered recursive call from
// interfering with an in-progress poll from the main loop.
// Without this guard:
//   Main loop → poll → STATUS(nb=5) → DATA call 1 (nb→4)
//   [IRQ12] → poll → STATUS(nb=4) → DATA calls drain to nb=0
//   Main loop → DATA call 2 → size(1) > nb_queue(0) → QEMU WARNING
// =========================================================================
void vmmouse_poll(void) {
    if (!g_vmmouse_active) return;

    // Re-entrancy guard — atomic test-and-set via irq_save
    uint64_t poll_flags = vmmouse_irq_save();
    if (g_vmmouse_polling) {
        vmmouse_irq_restore(poll_flags);
        return;
    }
    g_vmmouse_polling = true;
    vmmouse_irq_restore(poll_flags);

    static int32_t  last_vm_x    = -1;
    static int32_t  last_vm_y    = -1;
    static uint8_t  last_vm_btns = 0;

    // Drain up to 4 packets per poll call
    for (uint32_t i = 0; i < 4; i++) {
        int32_t  vm_x, vm_y;
        uint8_t  vm_buttons;

        if (!vmmouse_read(&vm_x, &vm_y, &vm_buttons)) {
            break;  // no more complete packets
        }

        // Suppress unchanged reports (mouse did not move or click)
        if (vm_x == last_vm_x && vm_y == last_vm_y && vm_buttons == last_vm_btns) {
            continue;
        }

        last_vm_x    = vm_x;
        last_vm_y    = vm_y;
        last_vm_btns = vm_buttons;

        g_vmmouse_packets_count++;
        // Push to HIDA arbiter — max coordinate = 0xFFFF (canonical space)
        hida_push_absolute(HIDA_BACKEND_VMMOUSE, vm_x, vm_y, 0xFFFF, 0xFFFF, vm_buttons, 0);
    }

    g_vmmouse_polling = false;
}
