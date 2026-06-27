#include "kernel/display/display.h"
#include "kernel/console/console.h"
#include "drivers/video/vga/vga.h"
#include "arch/x86_64/interrupt/idt.h"
#include "kernel/interrupt/include/isr.h"
#include "kernel/interrupt/include/exception.h"
#include "kernel/interrupt/include/irq.h"
#include "drivers/interrupt/pic/pic.h"
#include "kernel/timer/include/timer.h"
#include "kernel/keyboard/include/keyboard.h"
#include "kernel/boot/include/boot_info.h"
#include "kernel/memory/pmm/include/pmm.h"
#include "kernel/memory/vmm/include/vmm.h"
#include "kernel/memory/vmm/include/paging.h"
#include "kernel/memory/heap/include/heap.h"
#include "kernel/scheduler/include/scheduler.h"
#include "kernel/scheduler/include/context.h"
#include "kernel/process/include/process_image.h"
#include "kernel/process/include/process_builder.h"
#include "kernel/process/include/enter_usermode.h"
#include "kernel/scheduler/include/runqueue.h"
#include "kernel/config/build_config.h"
#include "kernel/lib/include/list.h"
#include "kernel/syscall/include/syscall.h"
#include "kernel/storage/include/disk_manager.h"
#include "kernel/vfs/include/vfs.h"
#include "kernel/fs/fat32/include/fat32.h"
#include "kernel/driver/storage/include/ata.h"
#include "kernel/loader/elf/include/elf.h"
#include "kernel/input/input.h"
#include "kernel/keyboard/include/keyboard.h"
#include "kernel/input/bmde.h"
#include "drivers/input/ps2/mouse.h"
#include "bovisual/Include/boscal.h"
#include "bovisual/Include/bovisual_types.h"
#include "bovisual/Include/controls.h"
#include "bovisual/Include/renderer.h"
#include "bovisual/Include/input.h"
#include "bovisual/Include/text.h"
#include "bovisual/Include/cursor_manager.h"
#include "kernel/BOSurface/Core/surface.h"
#include <stddef.h>

uint32_t g_kernel_screen_width = 1280;
uint32_t g_kernel_screen_height = 720;

_Static_assert(sizeof(Task) == 104, "Task struct size mismatch!");
_Static_assert(offsetof(Task, rsp) == 32, "Task rsp offset mismatch!");
_Static_assert(sizeof(Context) == 176, "Context struct size mismatch!");

static BackendDriver vga_backend = {
    .init = vga_init,
    .draw_character = vga_draw_character,
    .set_hardware_cursor = vga_set_hardware_cursor,
    .clear_memory = vga_clear_memory,
    .get_width = vga_get_screen_width,
    .get_height = vga_get_screen_height
};

typedef struct {
    uint32_t magic;
    list_node_t queue_node;
} TestNode;

static void test_intrusive_list(void) {
    list_t my_list;
    list_init(&my_list);

    TestNode n1, n2;
    n1.magic = 111;
    n2.magic = 222;
    
    list_node_init(&n1.queue_node);
    list_node_init(&n2.queue_node);

    list_insert_tail(&my_list, &n1.queue_node);
    list_insert_tail(&my_list, &n2.queue_node);

    if (my_list.size != 2) {
        display_print("[TEST] Intrusive List: SIZE FAIL\n");
        while(1) __asm__ volatile("hlt");
    }

    list_node_t* popped = list_remove_head(&my_list);
    TestNode* popped_node = LIST_ENTRY(popped, TestNode, queue_node);

    if (popped_node->magic != 111 || my_list.size != 1) {
        display_print("[TEST] Intrusive List: POP FAIL\n");
        while(1) __asm__ volatile("hlt");
    }

    display_print("[TEST] Intrusive List Validation: PASS\n");
}

static void kernel_run_self_tests(void) {
    display_print("\n--- Kernel Self Tests ---\n");
    pmm_self_test();
    vmm_self_test();
    ata_self_test();
    vfs_self_test();
    fat32_self_test();
    display_print("-------------------------\n\n");
}

static void task_a_entry(void) {
    while (1) {
        display_print("[PID ");
        display_print_dec(sys_getpid());
        display_print("] Tick ");
        display_print_dec(sys_uptime());
        display_print(" : A\n");
        sys_sleep(50); // Sleep via syscall
    }
}

static void task_b_entry(void) {
    while (1) {
        display_print("[PID ");
        display_print_dec(sys_getpid());
        display_print("] Tick ");
        display_print_dec(sys_uptime());
        display_print(" : B\n");
        sys_sleep(200); // Sleep via syscall
    }
}

static void task_c_entry(void) {
    display_print("[PID ");
    display_print_dec(sys_getpid());
    display_print("] Stress Test Started\n");
    
    // 1. Stress test SYS_UPTIME (100,000 calls)
    for (volatile int i = 0; i < 100000; i++) {
        volatile uint64_t uptime = sys_uptime();
        (void)uptime; // Prevent optimization
    }
    display_print("[PID ");
    display_print_dec(sys_getpid());
    display_print("] 100K SYS_UPTIME: PASS\n");
    
    // 2. Test SYS_YIELD
    sys_yield();
    display_print("[PID ");
    display_print_dec(sys_getpid());
    display_print("] SYS_YIELD: PASS\n");
    
    // 3. Test Invalid Syscall
    uint64_t err;
    __asm__ volatile("mov $999, %%rax; int $0x80; mov %%rax, %0" : "=r"(err) : : "rax", "memory");
    if (err == (uint64_t)-1) {
        display_print("[PID ");
        display_print_dec(sys_getpid());
        display_print("] SYS_INVALID: PASS\n");
    }

    display_print("[PID ");
    display_print_dec(sys_getpid());
    display_print("] Stress Test Complete. Sleeping forever.\n");
    
    while(1) {
        sys_sleep(100000);
    }
}

static void task_user_entry(void) {
    while (1) {
        // We cannot call display_print from User Mode because it uses outb!
        // Instead, we just spin and sleep using our valid Syscalls to prove it stays alive.
        sys_sleep(100);
        
        // This is to simulate a program doing work.
        volatile uint64_t uptime = sys_uptime();
        (void)uptime;
    }
}

static void task_fault_entry(void) {
    display_print("[USER PID ");
    display_print_dec(sys_getpid());
    display_print("] Preparing to execute privileged instruction...\n");
    sys_sleep(300);
    
    // Attempt privileged instruction from Ring 3 (should trigger GPF)
    __asm__ volatile("cli");
    
    display_print("ERROR: Survived privileged instruction!\n");
    while(1) sys_sleep(100);
}

// Helper to format unsigned integers
static void uitoa(uint32_t val, char* buf) {
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    char temp[32];
    int i = 0;
    while (val > 0) {
        temp[i++] = (val % 10) + '0';
        val /= 10;
    }
    int j = 0;
    while (i > 0) {
        buf[j++] = temp[--i];
    }
    buf[j] = '\0';
}



static void uitoa_signed(int32_t val, char* buf) {
    if (val < 0) {
        buf[0] = '-';
        uitoa((uint32_t)(-val), buf + 1);
    } else {
        uitoa((uint32_t)val, buf);
    }
}

// Helper to format hex (simple)
static void uitoa_hex(uint64_t val, char* buf) {
    buf[0] = '0'; buf[1] = 'x';
    if (val == 0) { buf[2] = '0'; buf[3] = '\0'; return; }
    int i = 2;
    char temp[32]; int ti = 0;
    while (val > 0) {
        uint32_t rem = val % 16;
        temp[ti++] = rem < 10 ? rem + '0' : (rem - 10) + 'A';
        val /= 16;
    }
    while (ti > 0) { buf[i++] = temp[--ti]; }
    buf[i] = '\0';
}

void kernel_main(boot_info_t* boot_info) {
    if (boot_info && boot_info->vbe_width > 0 && boot_info->vbe_height > 0) {
        g_kernel_screen_width = boot_info->vbe_width;
        g_kernel_screen_height = boot_info->vbe_height;
    }

    // 1. Display Subsystem
    console_set_backend(&vga_backend);
    display_init();
    display_clear();
    display_print("SignaturesOS v0.3 - BOS Architecture\n\n");
    display_print("[BOOT VBE] Boot Info Width: ");
    display_print_dec(boot_info->vbe_width);
    display_print("\n[BOOT VBE] Boot Info Height: ");
    display_print_dec(boot_info->vbe_height);
    display_print("\n[BOOT VBE] Boot Info Pitch: ");
    display_print_dec(boot_info->vbe_pitch);
    display_print("\n[BOOT VBE] Boot Info BPP: ");
    display_print_dec(boot_info->vbe_bpp);
    display_print("\n[BOOT VBE] Boot Info Framebuffer: ");
    display_print_hex(boot_info->vbe_framebuffer);
    display_print("\n\n");
    
    // Initialize new C-based GDT
    extern void gdt_init(void);
    gdt_init();
    display_print("GDT OK\n");
    
    test_intrusive_list();

    // 2. Interrupt Subsystem
    idt_init();
    display_print("IDT OK\n");

    isr_init();
    display_print("ISR OK\n");

    exception_init();
    display_print("EXC OK\n");

    pic_init();
    display_print("PIC OK\n");

    irq_init();
    display_print("IRQ OK\n");

    // 4. Input Subsystem
    kernel_input_init();
    keyboard_init();
#ifdef BMDE_DEBUG
    bmde_init();
#endif
    ps2_mouse_init();

    // 5. Physical Memory Manager
    pmm_init(boot_info);
    display_print("PMM OK\n");

    // 6. VMM — Step 1 bring-up
    vmm_init();

    // 7. Kernel Heap
    heap_init();

    // 8. Scheduler & Timer (Moved up for GUI Profiling)
    context_init();
    scheduler_init();
    timer_init(1000); // 1000 Hz = 1ms resolution
    display_print("TMR OK\n");

    // ----------------------------------------------------
    // BWE Phase 1: Core Surface Output
    // ----------------------------------------------------
    display_print("\n");
    display_print("┌──────────────────────────┐\n");
    display_print("│ BOSurface Demo           │\n");
    display_print("├──────────────────────────┤\n");
    display_print("│                          │\n");
    display_print("│ Hello BOSurface!         │\n");
    display_print("│                          │\n");
    display_print("└──────────────────────────┘\n\n");
    BOS_Test_Phase1();
    BOS_Test_Phase3();
    
    // ----------------------------------------------------
    // BOGUI Phase 1: Graphics Foundation
    // ----------------------------------------------------
    extern void vbe_init(boot_info_t* boot_info);
    extern BVFramebuffer* vbe_get_framebuffer(void);
    extern bool BOVISUAL_Init(const BVFramebuffer* framebuffer);
    extern void BOVISUAL_Text_Init(void);
    extern void BOVISUAL_Graphics_PutPixel(int32_t x, int32_t y, BOVISUAL_Color color);
    extern void BOVISUAL_Graphics_Fill(int32_t x, int32_t y, int32_t width, int32_t height, BOVISUAL_Color color);
    extern void BOVISUAL_Graphics_Clear(BOVISUAL_Color color);
    extern void BOVISUAL_Graphics_SwapBuffers(const BVFramebuffer* hw_fb);

    vbe_init(boot_info);
    BVFramebuffer fb;
    fb.buffer = (uint32_t*)boot_info->vbe_framebuffer;
    fb.width = g_kernel_screen_width;
    fb.height = g_kernel_screen_height;
    BVFramebuffer* hw_fb = vbe_get_framebuffer();
    
    // Phase 4/5: Back Buffer Allocation
    static BVFramebuffer back_fb;
    back_fb.width = hw_fb->width;
    back_fb.height = hw_fb->height;
    back_fb.pitch = hw_fb->pitch;
    
    uint64_t fb_size = hw_fb->height * hw_fb->pitch;
    uint64_t num_pages = (fb_size + 4095) / 4096;
    uint64_t bb_vaddr = 0x90000000ULL; // Isolated virtual region for Back Buffer
    
    void* active_pml4 = vmm_get_active_pml4();
    for (uint64_t i = 0; i < num_pages; i++) {
        vmm_alloc_mapped_page(active_pml4, bb_vaddr + (i * 4096), PAGE_WRITABLE | PAGE_USER);
    }
    back_fb.buffer = (BOVISUAL_Color*)bb_vaddr;

    BOVISUAL_Init(&back_fb);
    BOVISUAL_Text_Init();
    BVCursor_Init(hw_fb->width, hw_fb->height);

    // ----------------------------------------------------
    // BWE Phase 4: Integrated GUI Loop
    // ----------------------------------------------------
    
    // Clear initial background
    BOVISUAL_Color bg_color = 0xFF222222; // Lighter gray to test visibility
    BOVISUAL_Graphics_Clear(bg_color);
    
    // Setup Phase 4 Surfaces
    BOS_Test_Phase4();
    BWE_Compose(); // Initial draw
    
    // Add full screen damage so SwapBuffers actually copies it!
    extern void BOVISUAL_Graphics_AddDamage(int32_t x, int32_t y, int32_t width, int32_t height);
    BOVISUAL_Graphics_AddDamage(0, 0, g_kernel_screen_width, g_kernel_screen_height);
    BOVISUAL_Graphics_SwapBuffers(hw_fb);
    
    BVEvent ev;
    int32_t old_mouse_x = g_kernel_screen_width / 2;
    int32_t old_mouse_y = g_kernel_screen_height / 2;
    
    // CRITICAL: Enable interrupts so mouse works and hlt doesn't freeze CPU forever
    __asm__ volatile("sti");

    while (1) {
        bool processed_any = false;
        bool bwe_dirty = false;
        
        while (kernel_get_event(&ev)) {
            processed_any = true;
            
            // Pass to Focus & Drag Engine
            BOS_ProcessEvent(&ev);
            
            // For now, any mouse movement while dragging or any click causes a redraw
            // In the future Phase 5, we'll track dirty regions properly
            if (ev.type == BV_EVENT_MOUSE_DOWN || ev.type == BV_EVENT_MOUSE_UP || (ev.type == BV_EVENT_MOUSE_MOVE && BOS_GetActiveSurface() != 0)) {
                bwe_dirty = true;
            }
        }
        
        if (bwe_dirty || processed_any) {
            // Draw background
            BOVISUAL_Graphics_Clear(bg_color);
            
            // Draw Window Manager Surfaces
            BWE_ComputeScreenBounds();
            BWE_Compose();
            
            // Draw Cursor
            BVCursor_Draw(ev.mouse_x, ev.mouse_y);
            
            // Hardware Swap
            BOVISUAL_Graphics_AddDamage(0, 0, g_kernel_screen_width, g_kernel_screen_height);
            BOVISUAL_Graphics_SwapBuffers(hw_fb);
        } else {
            // Idle if no events
            __asm__ volatile("hlt");
        }
    }

}
