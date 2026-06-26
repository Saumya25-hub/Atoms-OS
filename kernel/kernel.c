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
#include <stddef.h>

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
    // 1. Display Subsystem
    console_set_backend(&vga_backend);
    display_init();
    display_clear();
    display_print("SignaturesOS v0.3 - BOS Architecture\n\n");
    
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
    // BOGUI Phase 1: Graphics Foundation
    // ----------------------------------------------------
    extern void vbe_init(boot_info_t* boot_info);
    extern BVFramebuffer* vbe_get_framebuffer(void);
    extern bool BOVISUAL_Init(const BVFramebuffer* framebuffer);
    extern void BOVISUAL_Text_Init(void);
    extern void BOVISUAL_Graphics_PutPixel(int32_t x, int32_t y, BOVISUAL_Color color);
    extern void BOVISUAL_Graphics_Fill(int32_t x, int32_t y, int32_t width, int32_t height, BOVISUAL_Color color);
    extern void BOVISUAL_Graphics_Clear(BOVISUAL_Color color);
    // BOVISUAL_Draw_String declaration removed as it's now properly included from text.h

    vbe_init(boot_info);
    BOVISUAL_Init(vbe_get_framebuffer());
    BOVISUAL_Text_Init();

    // ----------------------------------------------------
    // BOVISUAL Phase 2: Native UI Boot Splash
    // ----------------------------------------------------
    
    // Clear the screen to a deep, premium dark blue background
    BOVISUAL_Color bg_color = 0xFF0B1120; 
    BOVISUAL_Graphics_Clear(bg_color);

    // Initialize BOSCAL with VBE info
    // For safety, assume 1920x1080 if boot_info is 0
    uint32_t sw = boot_info->vbe_width ? boot_info->vbe_width : 1920;
    uint32_t sh = boot_info->vbe_height ? boot_info->vbe_height : 1080;
    BOSCAL_Init(sw, sh, boot_info->vbe_pitch, boot_info->vbe_bpp);

    // 1. Draw a main decorative Panel in the center (Percentage based: 35vw x 25vh)
    BOVISUAL_Control_Panel main_panel;
    main_panel.bounds_def_w = BV_VW(35);
    main_panel.bounds_def_h = BV_VH(25);
    main_panel.anchor = BV_ANCHOR_CENTER;
    // Resolve abstract constraints into absolute pixel rect based on Work Area
    main_panel.bounds = BOSCAL_ResolveDesktopLayout(main_panel.bounds_def_w, main_panel.bounds_def_h, main_panel.anchor);
    // Move it up slightly just for aesthetic preference (can't do pure constraints without parent container yet)
    main_panel.bounds.y -= 50; 
    main_panel.bg_color = 0xFF172033;
    main_panel.border_color = 0xFF3B82F6; // Blue border
    main_panel.draw_border = true;
    BVRenderer_DrawPanel(&main_panel);

    // 2. Draw "SignaturesOS" Label (100% width of panel)
    BOVISUAL_Control_Label title_label;
    title_label.text = "SignaturesOS V1";
    title_label.bounds_def_w = BV_FILL();
    title_label.bounds_def_h = BV_PX(16);
    title_label.anchor = BV_ANCHOR_TOP;
    title_label.bounds = BOSCAL_ResolveLayout(main_panel.bounds, title_label.bounds_def_w, title_label.bounds_def_h, title_label.anchor);
    title_label.bounds.y += (main_panel.bounds.height * 20) / 100; // Manual offset for now until full Constraint Layout engine
    title_label.text_color = 0xFFFFFFFF; // White text
    title_label.bg_color = main_panel.bg_color;
    title_label.transparent_bg = true;
    title_label.h_align = BV_ALIGN_CENTER;
    title_label.v_align = BV_ALIGN_CENTER;
    BVRenderer_DrawLabel(&title_label);

    // 3. Draw a "Booting..." Button (60% width of panel)
    BOVISUAL_Control_Button boot_btn;
    boot_btn.text = "Booting...";
    boot_btn.bounds_def_w = BV_PCT(60);
    boot_btn.bounds_def_h = BV_PX(40);
    boot_btn.anchor = BV_ANCHOR_CENTER;
    boot_btn.bounds = BOSCAL_ResolveLayout(main_panel.bounds, boot_btn.bounds_def_w, boot_btn.bounds_def_h, boot_btn.anchor);
    boot_btn.bg_color = 0xFF2563EB; // Bright blue
    boot_btn.hover_color = 0xFF3B82F6; // Lighter blue
    boot_btn.pressed_color = 0xFF1D4ED8; // Darker blue
    boot_btn.text_color = 0xFFFFFFFF;
    boot_btn.border_color = 0xFF1D4ED8;
    boot_btn.is_pressed = false;
    boot_btn.is_hovered = false;
    boot_btn.is_focused = false;
    BVRenderer_DrawButton(&boot_btn);

    // Optional: Draw some circles to test drawing primitives
    extern void BOVISUAL_Draw_Circle(int32_t x0, int32_t y0, int32_t radius, BOVISUAL_Color color);
    BOVISUAL_Draw_Circle(BOS_Display_Get()->width / 2, main_panel.bounds.y - 60, 40, 0xFF10B981); // Emerald circle
    BOVISUAL_Draw_Circle(BOS_Display_Get()->width / 2, main_panel.bounds.y - 60, 30, 0xFF34D399);

    // Print Initialization Info at bottom left
    // Mini itoa functions for formatting
    char res_w_str[16] = {0}, res_h_str[16] = {0}, pitch_str[16] = {0}, bpp_str[16] = {0}, fb_str[16] = {0};

    uitoa(BOS_Display_Get()->width, res_w_str);
    uitoa(BOS_Display_Get()->height, res_h_str);
    uitoa(boot_info->vbe_pitch, pitch_str);
    uitoa(boot_info->vbe_bpp, bpp_str);
    uitoa_hex(boot_info->vbe_framebuffer, fb_str);

    // Build the diagnostic string dynamically (simple strcpy/strcat logic since we don't have sprintf)
    char diag_str[256];
    int di = 0;
    const char* p = "Graphics Engine: BOVISUAL | Core: BISHOP | Res: ";
    while(*p) diag_str[di++] = *p++;
    p = res_w_str; while(*p) diag_str[di++] = *p++;
    diag_str[di++] = 'x';
    p = res_h_str; while(*p) diag_str[di++] = *p++;
    p = " | Pitch: "; while(*p) diag_str[di++] = *p++;
    p = pitch_str; while(*p) diag_str[di++] = *p++;
    p = " | BPP: "; while(*p) diag_str[di++] = *p++;
    p = bpp_str; while(*p) diag_str[di++] = *p++;
    p = " | FB: "; while(*p) diag_str[di++] = *p++;
    p = fb_str; while(*p) diag_str[di++] = *p++;
    diag_str[di] = '\0';

    BOVISUAL_Control_Label info_label;
    info_label.text = diag_str;
    info_label.bounds_def_w = BV_FILL();
    info_label.bounds_def_h = BV_PX(16);
    info_label.anchor = BV_ANCHOR_BOTTOM;
    info_label.bounds = BOSCAL_ResolveDesktopLayout(info_label.bounds_def_w, info_label.bounds_def_h, info_label.anchor);
    // Left padding
    info_label.bounds.x += 20;
    info_label.bounds.width -= 40;
    info_label.text_color = 0xFF9CA3AF; // Gray text
    info_label.bg_color = bg_color;
    info_label.transparent_bg = true;
    info_label.h_align = BV_ALIGN_START;
    info_label.v_align = BV_ALIGN_CENTER;
    BVRenderer_DrawLabel(&info_label);

    // Boot Animation: Progress Bar (80% width of panel)
    BOVISUAL_Control_ProgressBar pbar;
    pbar.bounds_def_w = BV_PCT(80);
    pbar.bounds_def_h = BV_PX(12);
    pbar.anchor = BV_ANCHOR_CENTER;
    pbar.bounds = BOSCAL_ResolveLayout(main_panel.bounds, pbar.bounds_def_w, pbar.bounds_def_h, pbar.anchor);
    // Push it below the button
    pbar.bounds.y = boot_btn.bounds.y + boot_btn.bounds.height + 20;
    pbar.min_value = 0;
    pbar.max_value = 100;
    pbar.current_value = 0;
    pbar.bg_color = 0xFF1E293B;
    pbar.fill_color = 0xFF10B981; // Emerald green
    pbar.border_color = 0xFF3B82F6;
    pbar.draw_border = true;

    for (int i = 0; i <= 100; i += 2) {
        pbar.current_value = i;
        BV_ProgressBar_Render(&pbar);

        // Simple busy-wait for animation (since timer isn't up yet)
        for (volatile int delay = 0; delay < 100000; delay++) {}
    }

    // --- Phase 4 Validation: Interactive GUI Loop ---
    // CRITICAL: Enable hardware interrupts so IRQ12 (mouse) can fire!
    // Without this, the CPU ignores ALL hardware interrupts including the mouse.
    __asm__ volatile("sti");
    display_print("\nEntering Interactive GUI Loop. Click 'Booting...' to proceed.\n");
    bool proceed = false;
    BVEvent ev;
    ev.mouse_x = BOS_Display_Get()->width / 2;
    ev.mouse_y = BOS_Display_Get()->height / 2;
    
    // Initial draw
    BOVISUAL_Graphics_Clear(bg_color);
    BVRenderer_DrawPanel(&main_panel);
    BVRenderer_DrawLabel(&title_label);
    BVRenderer_DrawButton(&boot_btn);
    BVRenderer_DrawProgressBar(&pbar);
    BVRenderer_DrawLabel(&info_label);
    
    int loop_counter = 0;
    
    // Profiling State
    uint64_t last_clear_ms = 0;
    uint64_t last_panel_ms = 0;
    uint64_t last_widgets_ms = 0;
    uint64_t last_cursor_ms = 0;
    uint64_t last_total_ms = 0;

    // True FPS State
    uint64_t frames_this_second = 0;
    uint64_t last_fps_update_tick = timer_get_ticks();
    uint64_t current_fps = 0;

    while (!proceed) {
        bool got_event = kernel_get_event(&ev);
        bool force_overlay = false;

        uint64_t current_time = timer_get_ticks();
        if (current_time >= last_fps_update_tick + 1000) {
            current_fps = frames_this_second;
            frames_this_second = 0;
            last_fps_update_tick = current_time;
            force_overlay = true;
        }

        if (got_event) {
            // Process Input
            BV_Input_ProcessEvent(&ev, &boot_btn, 1);

            // If clicked and released, proceed
            if (ev.type == BV_EVENT_MOUSE_UP && boot_btn.is_hovered) {
                proceed = true;
            }
            
            // Profiling: Start
            uint64_t t_start = timer_get_ticks();

            // 1. Clear Screen
            BOVISUAL_Graphics_Clear(bg_color);
            uint64_t t_clear = timer_get_ticks();

            // 2. Draw Panel
            BVRenderer_DrawPanel(&main_panel);
            uint64_t t_panel = timer_get_ticks();

            // 3. Draw Widgets
            BVRenderer_DrawLabel(&title_label);
            BVRenderer_DrawButton(&boot_btn);
            BVRenderer_DrawProgressBar(&pbar);
            BVRenderer_DrawLabel(&info_label);
            uint64_t t_widgets = timer_get_ticks();

            // 4. Draw Cursor
            BVRenderer_DrawCursor(ev.mouse_x, ev.mouse_y);
            uint64_t t_end = timer_get_ticks();

            // Store metrics (in milliseconds)
            last_clear_ms = t_clear - t_start;
            last_panel_ms = t_panel - t_clear;
            last_widgets_ms = t_widgets - t_panel;
            last_cursor_ms = t_end - t_widgets;
            last_total_ms = t_end - t_start;
            
            // Frame is complete
            frames_this_second++;
        }

        loop_counter++;
        if (got_event || force_overlay || (loop_counter % 100000 == 0)) {
            // BMDE Live Overlay
            char d_irq[16], d_pkt[16], d_x[16], d_y[16];
            char d_fps[16], d_q[16], d_sync[16], d_drop[16], d_to[16], d_ack[16], d_dx[16], d_dy[16];
            
            uitoa_signed((int32_t)bmde_state.irq_count, d_irq);
            uitoa_signed((int32_t)bmde_state.total_packets, d_pkt);
            uitoa_signed((int32_t)current_fps, d_fps);
            uitoa_signed((int32_t)bmde_state.queue_size, d_q);
            uitoa_signed((int32_t)bmde_state.sync_errors, d_sync);
            uitoa_signed((int32_t)bmde_state.dropped_events, d_drop);
            uitoa_signed((int32_t)bmde_state.timeouts, d_to);
            uitoa_signed((int32_t)bmde_state.invalid_acks, d_ack);
            uitoa_signed((int32_t)bmde_state.dx, d_dx);
            uitoa_signed((int32_t)bmde_state.dy, d_dy);
            uitoa_signed((int32_t)ev.mouse_x, d_x);
            uitoa_signed((int32_t)ev.mouse_y, d_y);

            char dbg_str[256];
            int di = 0;
            const char* p = "[BMDE] IRQ:"; while(*p) dbg_str[di++] = *p++;
            p = d_irq; while(*p) dbg_str[di++] = *p++;
            p = " PKT:"; while(*p) dbg_str[di++] = *p++;
            p = d_pkt; while(*p) dbg_str[di++] = *p++;
            p = " FPS:"; while(*p) dbg_str[di++] = *p++;
            p = d_fps; while(*p) dbg_str[di++] = *p++;
            p = " Q:"; while(*p) dbg_str[di++] = *p++;
            p = d_q; while(*p) dbg_str[di++] = *p++;
            p = " SYNC:"; while(*p) dbg_str[di++] = *p++;
            p = d_sync; while(*p) dbg_str[di++] = *p++;
            p = " DROP:"; while(*p) dbg_str[di++] = *p++;
            p = d_drop; while(*p) dbg_str[di++] = *p++;
            p = " T/O:"; while(*p) dbg_str[di++] = *p++;
            p = d_to; while(*p) dbg_str[di++] = *p++;
            p = " ACK:"; while(*p) dbg_str[di++] = *p++;
            p = d_ack; while(*p) dbg_str[di++] = *p++;
            p = " DX:"; while(*p) dbg_str[di++] = *p++;
            p = d_dx; while(*p) dbg_str[di++] = *p++;
            p = " DY:"; while(*p) dbg_str[di++] = *p++;
            p = d_dy; while(*p) dbg_str[di++] = *p++;
            p = " X:"; while(*p) dbg_str[di++] = *p++;
            p = d_x; while(*p) dbg_str[di++] = *p++;
            p = " Y:"; while(*p) dbg_str[di++] = *p++;
            p = d_y; while(*p) dbg_str[di++] = *p++;
            dbg_str[di] = '\0';

            BOVISUAL_Control_Label debug_label;
            debug_label.text = dbg_str;
            debug_label.bounds_def_w = BV_FILL();
            debug_label.bounds_def_h = BV_PX(16);
            debug_label.anchor = BV_ANCHOR_TOP;
            debug_label.bounds = BOSCAL_ResolveDesktopLayout(debug_label.bounds_def_w, debug_label.bounds_def_h, debug_label.anchor);
            debug_label.bounds.x += 20;
            debug_label.bounds.width -= 40;
            debug_label.text_color = 0xFFEF4444; // Red for debug
            debug_label.bg_color = bg_color;
            debug_label.transparent_bg = false; // Overwrite background so it doesn't smear
            debug_label.h_align = BV_ALIGN_START;
            debug_label.v_align = BV_ALIGN_START;
            BVRenderer_DrawLabel(&debug_label);
        }
    }
    // Show visual feedback that click was registered
    __asm__ volatile("cli"); // Disable interrupts for clean transition
    BOVISUAL_Graphics_Clear(bg_color);
    BOVISUAL_Control_Label proceed_label;
    proceed_label.text = "Button Clicked! Loading OS...";
    proceed_label.bounds_def_w = BV_PX(400);
    proceed_label.bounds_def_h = BV_PX(20);
    proceed_label.anchor = BV_ANCHOR_CENTER;
    proceed_label.bounds = BOSCAL_ResolveDesktopLayout(proceed_label.bounds_def_w, proceed_label.bounds_def_h, proceed_label.anchor);
    proceed_label.text_color = 0xFF10B981;
    proceed_label.bg_color = bg_color;
    proceed_label.transparent_bg = true;
    proceed_label.h_align = BV_ALIGN_CENTER;
    proceed_label.v_align = BV_ALIGN_CENTER;
    BVRenderer_DrawLabel(&proceed_label);

    // 8. Scheduler (Moved up)
    
    // 3. Timer Subsystem (Moved up)

    // 4. Keyboard Subsystem
    display_print("KBD OK\n");
    display_print("\n4. Multitasking Subsystem\n");
    // scheduler_create_kernel_task("TaskA", task_a_entry);
    // scheduler_create_user_task("TaskUser", task_user_entry);
    // scheduler_create_user_task("TaskFault", task_fault_entry);
    
    display_clear();
    display_print("Phase 18 Validation Initializing...\n");
    
    syscall_init();
    display_print("Syscalls OK\n");
    
    // Initialize Phase 20 Storage Layer using Disk Manager
    disk_manager_init();

    // Initialize Phase 21 VFS Core & Phase 22 FAT32
    vfs_init();
    fat32_init();
    
    // TODO
    // Replace static BlockDevice ID
    // with Disk Manager partition lookup.
    // Mount fat32 to / using disk0p1 (Block Device ID 1)
    vfs_mount_fs("/", 1, "fat32");
    
    // Test VFS Routing (Sprint 7: Generic VFS API)
    display_print("\n--- Phase 22 Sprint 7 Validation ---\n");
    int fd = vfs_open("/BOS_OS.TXT");
    if (fd >= 0) {
        char buffer[256];
        int bytes_read = vfs_read(fd, buffer, 255);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            display_print("Reading Contents using generic VFS API:\n");
            display_print("--------------------------------------------------\n");
            display_print(buffer);
            display_print("--------------------------------------------------\n");
        }
        vfs_close(fd);
    } else {
        display_print("VFS: Failed to open file!\n");
    }
    
    // Validation Test: Invalid Syscall
    uint64_t err;
    __asm__ volatile("mov $999, %%rax; int $0x80; mov %%rax, %0" : "=r"(err) : : "rax", "memory");
    if (err == (uint64_t)-1) {
        display_print("[VALIDATION] SYS_INVALID handled safely\n");
    } else {
        display_print("[VALIDATION] SYS_INVALID failed\n");
    }

    // Phase 23 Sprint 1-4 Validation (ELF Memory Mapper)
    // Clear screen so the user doesn't have to record videos to catch the fast output!
    display_clear();
    
    extern void* vmm_get_active_pml4(void);
    
    // We include process.h to know about ProcessImage and process_spawn
    #include "kernel/process/include/process.h"
    extern ProcessImage* elf_load_image(void* pml4, const char* path);
    
    void* new_pml4 = vmm_create_address_space();
    ProcessImage* init_process = elf_load_image(new_pml4, "/SHELL.ELF");
    if (!init_process) {
        display_print("[ELF] Failed to load /SHELL.ELF\n");
    } else {
#ifdef BOS_DEBUG
        display_print("\n------------------------------\n\n");
        display_print("Segments Loaded : "); display_print_dec(init_process->segments_loaded); display_print("\n\n");
        display_print("Image Base      : "); display_print_hex(init_process->image_base); display_print("\n");
        display_print("Image End       : "); display_print_hex(init_process->image_end); display_print("\n");
        
        uint64_t size_kb = init_process->image_size / 1024;
        if (size_kb == 0 && init_process->image_size > 0) size_kb = 1; // Show at least 1 KB if size > 0 but < 1024
        display_print("Image Size      : "); display_print_dec(size_kb); display_print(" KB\n\n");
        
        display_print("Heap Start      : "); display_print_hex(init_process->heap_start); display_print("\n\n");
        
        display_print("Entry Point     : "); display_print_hex(init_process->entry_point); display_print("\n\n");
        
        if (init_process->pml4 != 0) {
            display_print("User PML4       : PASS\n");
        } else {
            display_print("User PML4       : FAIL\n");
        }
#endif
        
        // Phase 23 Sprint 6, 7, 8: Transition to Ring 3
        if (!process_build_user_stack(init_process, new_pml4)) {
            display_print("[FAIL] Could not build user stack\n");
            while(1) { __asm__ volatile("hlt"); }
        }
        
        display_print("\n[BOS] Spawning Shell Process...\n");
        
        process_spawn(init_process, "Shell");
    }

    // Start Scheduler
    display_print("\n[BOS] Starting Scheduler...\n");
    scheduler_start();

    display_print("\n[DEBUG] Returned from Scheduler? This should not happen!\n");
    while(1) { __asm__ volatile("hlt"); }

    display_print("\n--- BOS Kernel Diagnostics ---\n");
    display_print("Kernel Build   : 0.4.0\n");
    display_print("CPU            : x86_64\n");
    display_print("Memory         : 64 MB\n");
    display_print("Page Size      : 4096\n");
    display_print("Heap           : 16 KB\n");
    display_print("Processes      : 2\n");
    display_print("Threads        : 2\n");
    display_print("Block Devices  : 2\n");
    display_print("Mounted FS     : 1\n");
    display_print("Filesystem     : FAT32\n");
    display_print("Kernel Size    : ~500 KB\n");
    display_print("Free Pages     : (Managed dynamically)\n");
    display_print("Uptime         : ");
    display_print_dec(sys_uptime());
    display_print(" ms\n");
    display_print("------------------------------\n");

#if BOS_DEBUG
    display_print("\n[DEBUG] Halting CPU to view output.\n");
    while(1) { __asm__ volatile("hlt"); }
#endif

    scheduler_start();
}
