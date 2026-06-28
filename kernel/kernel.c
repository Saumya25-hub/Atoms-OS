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

_Static_assert(sizeof(Task) == 112, "Task struct size mismatch!");
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

// ============================================================
// XP Fast Debug Helper
// ============================================================
static void draw_number(int32_t x, int32_t y, uint32_t num, uint32_t color) {
    char buf[16];
    int i = 14;
    buf[15] = '\0';
    if (num == 0) buf[i--] = '0';
    while (num > 0 && i >= 0) {
        buf[i--] = '0' + (num % 10);
        num /= 10;
    }
    extern const BVFontMetrics* BV_GetDefaultFont(void);
    BOVISUAL_Draw_String(x, y, &buf[i + 1], color, 0, true, BV_GetDefaultFont());
}
bool ENABLE_XP_DEBUG = true;

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

    syscall_init();
    display_print("SYS OK\n");

    extern void conhost_init(void);
    conhost_init();

    // 8. Storage + VFS + FAT32
    extern void disk_manager_init(void);
    extern void vfs_init(void);
    extern void fat32_init(void);
    extern int block_device_count(void);

    disk_manager_init();
    display_print("DSK OK\n");

    vfs_init();
    fat32_init();
    display_print("VFS OK\n");

    // Mount root filesystem — partition 1 is typically block device ID 1
    // (ID 0 = raw ATA drive, ID 1 = first MBR partition)
    int bd_count = block_device_count();
    if (bd_count > 1) {
        int mount_result = vfs_mount_fs("/", 1, "fat32");
        if (mount_result == 0) {
            display_print("[VFS] Root (/) mounted successfully\n");
        } else {
            display_print("[VFS] Root mount failed, trying device 0\n");
            mount_result = vfs_mount_fs("/", 0, "fat32");
            if (mount_result == 0) {
                display_print("[VFS] Root (/) mounted on device 0\n");
            } else {
                display_print("[VFS] WARNING: No root filesystem!\n");
            }
        }
    } else if (bd_count > 0) {
        int mount_result = vfs_mount_fs("/", 0, "fat32");
        if (mount_result == 0) {
            display_print("[VFS] Root (/) mounted on device 0\n");
        } else {
            display_print("[VFS] WARNING: No root filesystem!\n");
        }
    } else {
        display_print("[VFS] WARNING: No block devices found!\n");
    }

    // 9. Scheduler & Timer (Moved up for GUI Profiling)
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
    extern void BOS_Test_Phase5(void);
    BOS_Test_Phase5();
    
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
    
    // Setup Phase 7 State Test Surfaces
    extern void BOS_Test_Phase12_TextViewer(void);
    BOS_Test_Phase12_TextViewer();
    BWE_Compose(); // Initial draw
    
    // Register kernel_main as a schedulable task and enable preemptive multitasking.
    // Without this, user processes (SHELL.BOSX etc.) would never get CPU time.
    scheduler_register_boot_task();
    
    // Add full screen damage so SwapBuffers actually copies it!
    extern void BOVISUAL_Graphics_AddDamage(int32_t x, int32_t y, int32_t width, int32_t height);
    BOVISUAL_Graphics_AddDamage(0, 0, g_kernel_screen_width, g_kernel_screen_height);
    BOVISUAL_Graphics_SwapBuffers(hw_fb);
    
    BVEvent ev;
    int32_t current_mouse_x = g_kernel_screen_width / 2;
    int32_t current_mouse_y = g_kernel_screen_height / 2;
    int32_t target_mouse_x = current_mouse_x;
    int32_t target_mouse_y = current_mouse_y;
    
    // CRITICAL: Enable interrupts so mouse works and hlt doesn't freeze CPU forever
    __asm__ volatile("sti");

    extern void BOF_BeginAtomicFrame(void);
    BOF_BeginAtomicFrame();

    static uint8_t current_mouse_buttons = 0;

    while (1) {
        bool bwe_dirty = false;
        
        while (kernel_get_event(&ev)) {
            if (ev.type == BV_EVENT_MOUSE_MOVE || ev.type == BV_EVENT_MOUSE_DOWN || ev.type == BV_EVENT_MOUSE_UP) {
                target_mouse_x = ev.mouse_x;
                target_mouse_y = ev.mouse_y;
                current_mouse_buttons = ev.mouse_buttons;
            }
            
            // Pass clicks and keys immediately with current lerped pos
            if (ev.type != BV_EVENT_MOUSE_MOVE) {
                ev.mouse_x = current_mouse_x;
                ev.mouse_y = current_mouse_y;
                BOS_ProcessEvent(&ev);
                bwe_dirty = true;
            }
        }
        
        // Phase 19.1: Hardware Safety Net
        if (current_mouse_buttons == 0) {
            extern uint32_t bwe_capture_surface_id;
            extern bool bwe_is_dragging;
            if (bwe_capture_surface_id != 0 || bwe_is_dragging) {
                bwe_capture_surface_id = 0;
                bwe_is_dragging = false;
                extern uint32_t bwe_drag_surface_id;
                bwe_drag_surface_id = 0;
            }
        }

        // Mouse Lerp Step
        if (current_mouse_x != target_mouse_x || current_mouse_y != target_mouse_y) {
            current_mouse_x += (target_mouse_x - current_mouse_x) / 2;
            if (current_mouse_x == target_mouse_x - 1 || current_mouse_x == target_mouse_x + 1) current_mouse_x = target_mouse_x;
            
            current_mouse_y += (target_mouse_y - current_mouse_y) / 2;
            if (current_mouse_y == target_mouse_y - 1 || current_mouse_y == target_mouse_y + 1) current_mouse_y = target_mouse_y;
            
            BVEvent move_ev;
            move_ev.type = BV_EVENT_MOUSE_MOVE;
            move_ev.mouse_x = current_mouse_x;
            move_ev.mouse_y = current_mouse_y;
            
            // Store old drag bounds if dragging before processing event
            extern bool bwe_is_dragging;
            extern uint32_t bwe_drag_surface_id;
            extern BWE_Surface* BWE_GetSurface(uint32_t);
            extern void BOF_AddDirtyRect(BWE_Rect rect);
            
            if (bwe_is_dragging && bwe_drag_surface_id != 0) {
                BWE_Surface* dragged = BWE_GetSurface(bwe_drag_surface_id);
                if (dragged) {
                    BOF_AddDirtyRect((BWE_Rect){ dragged->screen_bounds.x, dragged->screen_bounds.y, dragged->screen_bounds.width + 10, dragged->screen_bounds.height + 40 }); // Old bounds
                }
            }
            
            BOS_ProcessEvent(&move_ev);
            
            // Damage new drag bounds
            if (bwe_is_dragging && bwe_drag_surface_id != 0) {
                BWE_Surface* dragged = BWE_GetSurface(bwe_drag_surface_id);
                if (dragged) {
                    BOF_AddDirtyRect((BWE_Rect){ dragged->screen_bounds.x, dragged->screen_bounds.y, dragged->screen_bounds.width + 10, dragged->screen_bounds.height + 40 }); // New bounds
                }
            }
            bwe_dirty = true;
        }
        
        extern bool conhost_has_dirty_sessions(void);
        extern void conhost_clear_dirty_all(void);
        if (conhost_has_dirty_sessions()) {
            bwe_dirty = true;
            conhost_clear_dirty_all();
        }
        
        if (bwe_dirty) {
            extern uint64_t timer_get_ticks(void);
            uint64_t frame_start = timer_get_ticks();
            
            extern void BOF_BeginAtomicFrame(void);
            extern void BOF_AddDirtyRect(BWE_Rect rect);
            extern bool BOF_SkipIfClean(void);
            extern void BOF_ComposeDirtyOnly(uint32_t bg_color);
            extern void BOF_EndAtomicFrame(const BVFramebuffer* hw_fb);

            // Add other dirty rects
            static int32_t last_m_x = -1, last_m_y = -1;
            if (last_m_x != -1) BOF_AddDirtyRect((BWE_Rect){last_m_x, last_m_y, 16, 16});
            BOF_AddDirtyRect((BWE_Rect){current_mouse_x, current_mouse_y, 16, 16});
            last_m_x = current_mouse_x;
            last_m_y = current_mouse_y;
            
            if (ev.type != BV_EVENT_MOUSE_MOVE && ev.type != 0) {
                // Removed full-screen fallback dirty rect. 
                // Only targeted UI invalidations will redraw now.
            }
            
            if (ENABLE_XP_DEBUG) {
                // No continuous dirty rect for overlay to save CPU.
            }

            if (BOF_SkipIfClean()) {
                extern void sys_sleep(uint64_t ms);
                sys_sleep(16);
                continue;
            }

            // Always compute layout bounds for logical consistency
            BWE_ComputeScreenBounds();
            
            // Draw ONLY dirty regions (The Core Fix)
            BOF_ComposeDirtyOnly(bg_color);
            
            // Draw Cursor (Fast overlay)
            BVCursor_Draw(current_mouse_x, current_mouse_y);
            
            // Fast Debug Overlay Disabled (Pure event-driven micro-compositor mode)
            
            // ATOMIC COMMIT: Swap exact dirty regions to MMIO
            BOF_EndAtomicFrame(hw_fb);
            
            // Start next atomic frame
            BOF_BeginAtomicFrame();
            
            // Frame Limiter (~60 FPS)
            uint64_t frame_time = timer_get_ticks() - frame_start;
            if (frame_time < 16) {
                extern void sys_sleep(uint64_t ms);
                sys_sleep(16 - frame_time);
            }
            
            extern void bodebug_dump(void);
            bodebug_dump();
        } else {
            // Yield CPU to background tasks if GUI is idle
            extern void scheduler_yield(void);
            scheduler_yield();
        }
    }

}
