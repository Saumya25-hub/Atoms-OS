#include "arch/x86_64/cpu/cpu_features.h"
#include "arch/x86_64/interrupt/idt.h"
#include "arch/x86_64/smp/smp.h"
#include "bovisual/Include/boscal.h"
#include "bovisual/Include/bovisual_types.h"
#include "bovisual/Include/controls.h"
#include "bovisual/Include/cursor_manager.h"
#include "bovisual/Include/input.h"
#include "bovisual/Include/renderer.h"
#include "bovisual/Include/text.h"
#include "drivers/input/ps2/mouse.h"
#include "drivers/interrupt/pic/pic.h"
#include "drivers/video/vga/vga.h"
#include "kernel/ame/include/ame.h"
#include "kernel/audio/api/audio_api.h"
#include "kernel/audio/diagnostics/audio_test_mode.h"
#include "kernel/audio/hal/audio_hal.h"
#include "kernel/audio/mixer/audio_mixer.h"
#include "kernel/audio/session/audio_player.h"
#include "kernel/core/core_legacy/boot/include/boot_info.h"
#include "kernel/core/core_legacy/config/build_config.h"
#include "kernel/core/execution/include/execution_contract.h"
#include "kernel/core/interrupt/include/exception.h"
#include "kernel/core/interrupt/include/irq.h"
#include "kernel/core/interrupt/include/isr.h"
#include "kernel/core/lib/include/crash_log.h"
#include "kernel/core/lib/include/list.h"
#include "kernel/core/loader/elf/include/elf.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/core/memory/vmm/include/paging.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/process/include/enter_usermode.h"
#include "kernel/core/process/include/process_builder.h"
#include "kernel/core/process/include/process_image.h"
#include "kernel/core/process/process_manager.h"
#include "kernel/core/scheduler/include/context.h"
#include "kernel/core/scheduler/include/runqueue.h"
#include "kernel/core/scheduler/include/scheduler.h"
#include "kernel/core/syscall/include/syscall.h"
#include "kernel/core/thread/thread_manager.h"
#include "kernel/core/timer/include/timer.h"
#include "kernel/debug/phase7_reliability.h"
#include "kernel/debug/step14_telemetry.h"
#include "kernel/debug/test_bgl_phase1.h"
#include "kernel/debug/test_cpu_phase0.h"
#include "kernel/display/agdpe/agdpe.h"
#include "kernel/drivers/display/display.h"
#include "kernel/graphics/gpu/include/gpu.h"
#include "kernel/drivers/input/bmde.h"
#include "kernel/drivers/input/input.h"
#include "kernel/drivers/keyboard/include/keyboard.h"
#include "kernel/drivers/storage_legacy/storage/include/ata.h"
#include "kernel/drivers/usb/host/xhci/xhci.h"
#include "kernel/ipc/include/ipc_types.h"
#include "kernel/loader/include/loader_types.h"
#include "kernel/mm/amsss/amsss.h"
#include "kernel/shell/console/console.h"
#include "kernel/shell/rook/include/rook.h"
#include "kernel/ui/bofont/bofont.h"
#include "kernel/vfs/vfs_legacy/fs/fat32/include/fat32.h"
#include "kernel/system/boot/boot_mode.h"
#include "kernel/system/boot/deferred_work.h"
#include "kernel/security/include/bos_security.h"
#include "kernel/sandbox/include/bos_sandbox.h"
#include "browser/html/include/bos_html.h"
#include "browser/css/include/bos_css.h"
#include "kernel/graphics/bvmm/include/bvmm.h"



static void deferred_security_tests_wrapper(void) {
    bos_security_run_certification_tests();
}

static void deferred_sandbox_tests_wrapper(void) {
    bos_sandbox_run_certification_tests();
}

static void deferred_html_tests_wrapper(void) {
    bos_html_run_certification_tests();
}


bool g_enable_runtime_telemetry = false;


#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/vfs/vfs_legacy/storage/include/disk_manager.h"
#include "kernel/wm/surface/surface.h"
#include <stddef.h>

bool bwe_dirty = true;

uint32_t g_kernel_screen_width = 1280;
uint32_t g_kernel_screen_height = 720;

/* Task layout is validated by the context ABI tests; do not duplicate a stale
 * size assertion here. */
_Static_assert(offsetof(Task, rsp) == 32, "Task rsp offset mismatch!");
_Static_assert(sizeof(Context) == 176, "Context struct size mismatch!");

static BackendDriver vga_backend = {.init = vga_init,
                                    .draw_character = vga_draw_character,
                                    .set_hardware_cursor =
                                        vga_set_hardware_cursor,
                                    .clear_memory = vga_clear_memory,
                                    .get_width = vga_get_screen_width,
                                    .get_height = vga_get_screen_height};

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
    while (1)
      __asm__ volatile("hlt");
  }

  list_node_t *popped = list_remove_head(&my_list);
  TestNode *popped_node = LIST_ENTRY(popped, TestNode, queue_node);

  if (popped_node->magic != 111 || my_list.size != 1) {
    display_print("[TEST] Intrusive List: POP FAIL\n");
    while (1)
      __asm__ volatile("hlt");
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

  /* BVMM All 12 Phases Certification Tests (BVMM V1.0 PRODUCTION) */
  extern bool bvmm_run_phase1_tests(void);
  extern bool bvmm_run_phase2_tests(void);
  extern bool bvmm_run_phase3_tests(void);
  extern bool bvmm_run_phase4_tests(void);
  extern bool bvmm_run_phase5_tests(void);
  extern bool bvmm_run_phase6_tests(void);
  extern bool bvmm_run_phase7_tests(void);
  extern bool bvmm_run_phase8_tests(void);
  extern bool bvmm_run_phase9_tests(void);
  extern bool bvmm_run_phase10_tests(void);
  extern bool bvmm_run_phase11_tests(void);
  extern bool bvmm_run_phase12_tests(void);
  bvmm_init();
  bvmm_run_phase1_tests();
  bvmm_run_phase2_tests();
  bvmm_run_phase3_tests();
  bvmm_run_phase4_tests();
  bvmm_run_phase5_tests();
  bvmm_run_phase6_tests();
  bvmm_run_phase7_tests();
  bvmm_run_phase8_tests();
  bvmm_run_phase9_tests();
  bvmm_run_phase10_tests();
  bvmm_run_phase11_tests();
  bvmm_run_phase12_tests();

#include "kernel/loader/include/loader_types.h"
  extern loader_status_t bos_loader_init(void);
  extern bool loader_run_unit_tests(void);
  bos_loader_init();
  loader_run_unit_tests();
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
  __asm__ volatile("mov $999, %%rax; int $0x80; mov %%rax, %0"
                   : "=r"(err)
                   :
                   : "rax", "memory");
  if (err == (uint64_t)-1) {
    display_print("[PID ");
    display_print_dec(sys_getpid());
    display_print("] SYS_INVALID: PASS\n");
  }

  display_print("[PID ");
  display_print_dec(sys_getpid());
  display_print("] Stress Test Complete. Sleeping forever.\n");

  while (1) {
    sys_sleep(100000);
  }
}

static void task_user_entry(void) {
  while (1) {
    // We cannot call display_print from User Mode because it uses outb!
    // Instead, we just spin and sleep using our valid Syscalls to prove it
    // stays alive.
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
  while (1)
    sys_sleep(100);
}

// ============================================================
// XP Fast Debug Helper
// ============================================================
static void draw_number(int32_t x, int32_t y, uint32_t num, uint32_t color) {
  char buf[16];
  int i = 14;
  buf[15] = '\0';
  if (num == 0)
    buf[i--] = '0';
  while (num > 0 && i >= 0) {
    buf[i--] = '0' + (num % 10);
    num /= 10;
  }
  BOFont_DrawText(BOFont_GetRole(BOFONT_ROLE_MONO), &buf[i + 1], x, y, color);
}
bool ENABLE_XP_DEBUG = true;

// Helper to format unsigned integers
static void uitoa(uint32_t val, char *buf) {
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

static void uitoa_signed(int32_t val, char *buf) {
  if (val < 0) {
    buf[0] = '-';
    uitoa((uint32_t)(-val), buf + 1);
  } else {
    uitoa((uint32_t)val, buf);
  }
}

// Helper to format hex (simple)
static void uitoa_hex(uint64_t val, char *buf) {
  buf[0] = '0';
  buf[1] = 'x';
  if (val == 0) {
    buf[2] = '0';
    buf[3] = '\0';
    return;
  }
  int i = 2;
  char temp[32];
  int ti = 0;
  while (val > 0) {
    uint32_t rem = val % 16;
    temp[ti++] = rem < 10 ? rem + '0' : (rem - 10) + 'A';
    val /= 16;
  }
  while (ti > 0) {
    buf[i++] = temp[--ti];
  }
  buf[i] = '\0';
}

// ============================================================
// Multimedia Background Service
// ============================================================
void serial_write_direct(const char *str) {
  extern void io_out8(uint16_t port, uint8_t data);
  extern uint8_t io_in8(uint16_t port);
  for (int i = 0; str[i] != '\0'; i++) {
    while ((io_in8(0x3F8 + 5) & 0x20) == 0)
      ;
    io_out8(0x3F8, str[i]);
  }
}
void serial_write_dec_direct(int val) {
  char buf[32];
  int i = 0;
  if (val == 0) {
    serial_write_direct("0");
    return;
  }
  if (val < 0) {
    serial_write_direct("-");
    val = -val;
  }
  while (val > 0) {
    buf[i++] = '0' + (val % 10);
    val /= 10;
  }
  while (i > 0) {
    char c[2] = {buf[--i], '\0'};
    serial_write_direct(c);
  }
}

// === SILENT IN-MEMORY TELEMETRY SNAPSHOT (NO SERIAL BLOCKING) ===
typedef struct {
  uint32_t runtime_seconds;
  uint32_t ram_bytes_produced;
  uint32_t ram_refill_calls;
  uint32_t vfs_reads_after_start;
  uint32_t ata_reads;
  uint32_t ring_available;
  uint32_t mixer_silence_bytes;
  uint8_t ac97_civ;
  uint8_t ac97_lvi;
} AudioTelemetrySnapshot;

volatile AudioTelemetrySnapshot g_audio_telemetry_snapshot = {0};

static void audio_service_entry(void) {
  crash_log_add("[AUDIO] AudioSvc STARTED");
  static int atm_loop_iter = 0;
  static int telemetry_iter = 0;
  while (1) {
    audio_player_update();
#if AUDIO_TEST_MODE_ENABLED
    audio_test_mode_telemetry_tick();
#endif
    telemetry_iter++;
    if (telemetry_iter >= 250) { // 5 seconds (250 * 20ms)
      extern bool g_RAM_Only_Test_Active;
      if (g_RAM_Only_Test_Active) {
        extern uint32_t g_RAMAudioBytesProduced;
        extern uint32_t g_RAMAudioRefillCalls;
        extern uint32_t g_AudioVFSReadsAfterStart;
        extern uint32_t g_ata_read_count;
        extern uint64_t g_MixerSilenceInjectedBytes;
        extern size_t audio_stream_available(uint32_t stream_id);
        extern uint8_t ac97_get_civ(void);
        extern uint8_t ac97_get_lvi(void);
        // SILENT in-memory update — NO serial I/O, NO display_print



        atm_loop_iter++;
        g_audio_telemetry_snapshot.runtime_seconds = atm_loop_iter * 5;
        g_audio_telemetry_snapshot.ram_bytes_produced = g_RAMAudioBytesProduced;
        g_audio_telemetry_snapshot.ram_refill_calls = g_RAMAudioRefillCalls;
        g_audio_telemetry_snapshot.vfs_reads_after_start =
            g_AudioVFSReadsAfterStart;
        g_audio_telemetry_snapshot.ata_reads = g_ata_read_count;
        g_audio_telemetry_snapshot.ring_available =
            (uint32_t)audio_stream_available(0);
        g_audio_telemetry_snapshot.mixer_silence_bytes =
            (uint32_t)g_MixerSilenceInjectedBytes;
        g_audio_telemetry_snapshot.ac97_civ = ac97_get_civ();
        g_audio_telemetry_snapshot.ac97_lvi = ac97_get_lvi();
      }
      telemetry_iter = 0;
    }

    scheduler_sleep(20); // Wake up every 20ms to pump DMA and refill buffer
  }
}

volatile uint64_t g_main_loop_iterations_count = 0;

#include "kernel/graphics/BSPE/Cursor/bspe_cursor_present.h"

static void print_1sec_telemetry(void) {
  extern uint64_t timer_get_ticks(void);
  static uint64_t s_last_serial_ticks = 0;
  uint64_t cur_ticks = timer_get_ticks();
  if (cur_ticks - s_last_serial_ticks < 1000 && s_last_serial_ticks != 0)
    return;
  s_last_serial_ticks = cur_ticks;

  extern volatile uint64_t g_irq1_count;
  extern volatile uint64_t g_irq12_count;
  extern volatile uint64_t g_vmmouse_read_count;
  extern volatile uint64_t g_input_events_count;
  extern volatile uint64_t g_cursor_state_calls_count;
  extern volatile uint64_t g_bwe_update_calls_count;
  extern volatile uint64_t g_bvcursor_draw_count;
  extern volatile uint64_t g_frames_presented_count;
  extern volatile uint64_t g_kernel_input_motion_coalesced;
  extern volatile uint32_t g_kernel_input_queue_peak;
  extern volatile uint64_t g_bwe_motion_events_coalesced;
  extern volatile uint32_t g_bwe_event_queue_peak;
  extern volatile uint64_t g_bwe_process_events_pushed;
  extern volatile uint64_t g_bwe_process_events_popped;
  extern volatile uint64_t g_bwe_process_key_events_pushed;
  extern volatile uint64_t g_bwe_process_key_events_popped;
  extern volatile uint32_t g_bwe_process_last_push_pid;
  extern volatile uint32_t g_bwe_process_last_pop_pid;
  extern volatile uint64_t g_sys_get_input_event_calls;
  extern volatile uint64_t g_sys_get_input_event_empty;
  extern volatile uint32_t g_sys_get_input_event_last_pid;

  uint64_t c_irq1 = g_irq1_count;
  g_irq1_count = 0;
  uint64_t c_irq = g_irq12_count;
  g_irq12_count = 0;
  uint64_t c_vmm = g_vmmouse_read_count;
  g_vmmouse_read_count = 0;
  uint64_t c_inp = g_input_events_count;
  g_input_events_count = 0;
  uint64_t c_cur = g_cursor_state_calls_count;
  g_cursor_state_calls_count = 0;
  uint64_t c_bwe = g_bwe_update_calls_count;
  g_bwe_update_calls_count = 0;
  uint64_t c_bvc = g_bvcursor_draw_count;
  g_bvcursor_draw_count = 0;
  uint64_t c_frm = g_frames_presented_count;
  g_frames_presented_count = 0;
  uint64_t c_itr = g_main_loop_iterations_count;
  g_main_loop_iterations_count = 0;
  uint64_t c_raw_motion_coalesced = g_kernel_input_motion_coalesced;
  g_kernel_input_motion_coalesced = 0;
  uint32_t c_raw_queue_peak = g_kernel_input_queue_peak;
  g_kernel_input_queue_peak = 0;
  uint64_t c_bwe_motion_coalesced = g_bwe_motion_events_coalesced;
  g_bwe_motion_events_coalesced = 0;
  uint32_t c_bwe_queue_peak = g_bwe_event_queue_peak;
  g_bwe_event_queue_peak = 0;
  uint64_t c_process_pushed = g_bwe_process_events_pushed;
  g_bwe_process_events_pushed = 0;
  uint64_t c_process_popped = g_bwe_process_events_popped;
  g_bwe_process_events_popped = 0;
  uint64_t c_process_keys_pushed = g_bwe_process_key_events_pushed;
  g_bwe_process_key_events_pushed = 0;
  uint64_t c_process_keys_popped = g_bwe_process_key_events_popped;
  g_bwe_process_key_events_popped = 0;
  uint32_t c_process_last_push_pid = g_bwe_process_last_push_pid;
  uint32_t c_process_last_pop_pid = g_bwe_process_last_pop_pid;
  uint64_t c_input_poll_calls = g_sys_get_input_event_calls;
  g_sys_get_input_event_calls = 0;
  uint64_t c_input_poll_empty = g_sys_get_input_event_empty;
  g_sys_get_input_event_empty = 0;
  uint32_t c_input_poll_pid = g_sys_get_input_event_last_pid;

  extern void cursor_state_get_position(int32_t *out_x, int32_t *out_y);
  int32_t cur_x = 0, cur_y = 0;
  cursor_state_get_position(&cur_x, &cur_y);

  extern int32_t g_bwe_mouse_x;
  extern int32_t g_bwe_mouse_y;

  BSPE_CursorPresenterState p_st;
  BSPE_CursorPresenter_GetState(&p_st);

  serial_write_direct("\n=== RUNTIME TELEMETRY (1 SEC INTERVAL) ===\n");
  serial_write_direct("IRQ1/sec              : ");
  serial_write_dec_direct((int)c_irq1);
  serial_write_direct("\n");
  serial_write_direct("IRQ12/sec             : ");
  serial_write_dec_direct((int)c_irq);
  serial_write_direct("\n");
  serial_write_direct("VMMouseRead/sec       : ");
  serial_write_dec_direct((int)c_vmm);
  serial_write_direct("\n");
  serial_write_direct("InputEvents/sec       : ");
  serial_write_dec_direct((int)c_inp);
  serial_write_direct("\n");
  serial_write_direct("RawMotionCoalesced/sec: ");
  serial_write_dec_direct((int)c_raw_motion_coalesced);
  serial_write_direct("\n");
  serial_write_direct("RawInputQueuePeak     : ");
  serial_write_dec_direct((int)c_raw_queue_peak);
  serial_write_direct("\n");
  serial_write_direct("BWEMotionCoalesced/sec: ");
  serial_write_dec_direct((int)c_bwe_motion_coalesced);
  serial_write_direct("\n");
  serial_write_direct("BWEEventQueuePeak     : ");
  serial_write_dec_direct((int)c_bwe_queue_peak);
  serial_write_direct("\n");
  extern uint32_t g_doom_checkpoint;
  serial_write_direct("ProcessEvents push/pop: ");
  serial_write_dec_direct((int)c_process_pushed);
  serial_write_direct("/");
  serial_write_dec_direct((int)c_process_popped);
  serial_write_direct("\n");
  serial_write_direct("ProcessKeys push/pop  : ");
  serial_write_dec_direct((int)c_process_keys_pushed);
  serial_write_direct("/");
  serial_write_dec_direct((int)c_process_keys_popped);
  serial_write_direct("\n");
  serial_write_direct("ProcessQueue last PIDs: ");
  serial_write_dec_direct((int)c_process_last_push_pid);
  serial_write_direct("/");
  serial_write_dec_direct((int)c_process_last_pop_pid);
  serial_write_direct("\n");
  serial_write_direct("BVCursorDraw/sec      : ");
  serial_write_dec_direct((int)c_bvc);
  serial_write_direct("\n");
  serial_write_direct("FramesPresented/sec   : ");
  serial_write_dec_direct((int)c_frm);
  serial_write_direct("\n");
  serial_write_direct("MainLoopIterations/sec: ");
  serial_write_dec_direct((int)c_itr);
  serial_write_direct("\n");
  serial_write_direct("Current Cursor X/Y    : X=");
  serial_write_dec_direct(cur_x);
  serial_write_direct(" Y=");
  serial_write_dec_direct(cur_y);
  serial_write_direct("\n");
  serial_write_direct("Current BWE Mouse X/Y : X=");
  serial_write_dec_direct(g_bwe_mouse_x);
  serial_write_direct(" Y=");
  serial_write_dec_direct(g_bwe_mouse_y);
  serial_write_direct("\n");

  extern volatile uint64_t g_usb_reports_count;
  extern volatile uint64_t g_usb_motion_reports_count;
  extern volatile uint64_t g_hid_decoded_motion_count;
  extern volatile uint64_t g_hid_max_gap_ms;
  extern volatile uint64_t g_cursor_damage_requests_count;

  uint64_t c_usb_rep = g_usb_reports_count;
  g_usb_reports_count = 0;
  uint64_t c_usb_mot = g_usb_motion_reports_count;
  g_usb_motion_reports_count = 0;
  uint64_t c_hid_dec = g_hid_decoded_motion_count;
  g_hid_decoded_motion_count = 0;
  uint64_t c_hid_gap = g_hid_max_gap_ms;
  g_hid_max_gap_ms = 0;
  uint64_t c_cur_dam = g_cursor_damage_requests_count;
  g_cursor_damage_requests_count = 0;

  serial_write_direct("--- CUSTOM MOUSE TELEMETRY ---\n");
  serial_write_direct("USBReports/sec        : ");
  serial_write_dec_direct((int)c_usb_rep);
  serial_write_direct("\n");
  serial_write_direct("USBMotionReports/sec  : ");
  serial_write_dec_direct((int)c_usb_mot);
  serial_write_direct("\n");
  serial_write_direct("HIDDecodedMotion/sec  : ");
  serial_write_dec_direct((int)c_hid_dec);
  serial_write_direct("\n");
  serial_write_direct("HIDMaxInterReportGapMs: ");
  serial_write_dec_direct((int)c_hid_gap);
  serial_write_direct("\n");
  serial_write_direct("CursorPositionUpdates/sec: ");
  serial_write_dec_direct((int)c_cur);
  serial_write_direct("\n");
  serial_write_direct("CursorDamageRequests/sec : ");
  serial_write_dec_direct((int)c_cur_dam);
  serial_write_direct("\n");
  serial_write_direct("CursorDraws/sec       : ");
  serial_write_dec_direct((int)c_bvc);
  serial_write_direct("\n");
  serial_write_direct("FramesPresented/sec   : ");
  serial_write_dec_direct((int)c_frm);
  serial_write_direct("\n");

  extern volatile uint64_t g_frame_interval_min_ms;
  extern volatile uint64_t g_frame_interval_max_ms;
  uint64_t c_frm_min = g_frame_interval_min_ms;
  g_frame_interval_min_ms = 999999;
  uint64_t c_frm_max = g_frame_interval_max_ms;
  g_frame_interval_max_ms = 0;
  serial_write_direct("FrameIntervalMinMs    : ");
  serial_write_dec_direct(c_frm_min == 999999 ? 0 : (int)c_frm_min);
  serial_write_direct("\n");
  serial_write_direct("FrameIntervalMaxMs    : ");
  serial_write_dec_direct((int)c_frm_max);
  serial_write_direct("\n");

  extern volatile uint64_t g_gui_yields;
  extern volatile uint64_t g_gui_hlts;
  extern volatile uint64_t g_context_switches;
  extern uint32_t scheduler_get_task_count(void);

  uint64_t c_gui_yields = g_gui_yields;
  g_gui_yields = 0;
  uint64_t c_gui_hlts = g_gui_hlts;
  g_gui_hlts = 0;
  uint64_t c_ctx_sw = g_context_switches;
  g_context_switches = 0;

  serial_write_direct("--- SCHEDULER IDLE TELEMETRY ---\n");
  serial_write_direct("GUIYields/sec         : ");
  serial_write_dec_direct((int)c_gui_yields);
  serial_write_direct("\n");
  serial_write_direct("GUIHlts/sec           : ");
  serial_write_dec_direct((int)c_gui_hlts);
  serial_write_direct("\n");
  serial_write_direct("CtxSwitches/sec       : ");
  serial_write_dec_direct((int)c_ctx_sw);
  serial_write_direct("\n");
  serial_write_direct("RunnableTasks         : ");
  serial_write_dec_direct((int)scheduler_get_task_count());
  serial_write_direct("\n");

  extern uint32_t g_bspe_telemetry_present_calls;
  extern uint32_t g_bspe_telemetry_partial_presents;
  extern uint32_t g_bspe_telemetry_full_presents;
  extern uint32_t g_bspe_telemetry_no_damage;
  extern uint32_t g_bspe_telemetry_legacy_fallbacks;
  extern uint32_t g_bspe_telemetry_fallback_reason_tracker;
  extern uint32_t g_bspe_telemetry_fallback_reason_eval;
  extern uint32_t g_bspe_telemetry_fallback_reason_corrupt;
  extern uint32_t g_bspe_telemetry_fallback_reason_vram;

  uint32_t c_bspe_present = g_bspe_telemetry_present_calls;
  g_bspe_telemetry_present_calls = 0;
  uint32_t c_bspe_partial = g_bspe_telemetry_partial_presents;
  g_bspe_telemetry_partial_presents = 0;
  uint32_t c_bspe_full = g_bspe_telemetry_full_presents;
  g_bspe_telemetry_full_presents = 0;
  uint32_t c_bspe_nodmg = g_bspe_telemetry_no_damage;
  g_bspe_telemetry_no_damage = 0;
  uint32_t c_bspe_fallback = g_bspe_telemetry_legacy_fallbacks;
  g_bspe_telemetry_legacy_fallbacks = 0;
  uint32_t c_bspe_reason_t = g_bspe_telemetry_fallback_reason_tracker;
  g_bspe_telemetry_fallback_reason_tracker = 0;
  uint32_t c_bspe_reason_e = g_bspe_telemetry_fallback_reason_eval;
  g_bspe_telemetry_fallback_reason_eval = 0;
  uint32_t c_bspe_reason_c = g_bspe_telemetry_fallback_reason_corrupt;
  g_bspe_telemetry_fallback_reason_corrupt = 0;
  uint32_t c_bspe_reason_v = g_bspe_telemetry_fallback_reason_vram;
  g_bspe_telemetry_fallback_reason_vram = 0;

  serial_write_direct("--- PHASE 2 BSPE TELEMETRY ---\n");
  serial_write_direct("BSPE_PresentCalls/sec : ");
  serial_write_dec_direct((int)c_bspe_present);
  serial_write_direct("\n");
  serial_write_direct("BSPE_Partial/sec      : ");
  serial_write_dec_direct((int)c_bspe_partial);
  serial_write_direct("\n");
  serial_write_direct("BSPE_Full/sec         : ");
  serial_write_dec_direct((int)c_bspe_full);
  serial_write_direct("\n");
  serial_write_direct("BSPE_NoDamage/sec     : ");
  serial_write_dec_direct((int)c_bspe_nodmg);
  serial_write_direct("\n");
  serial_write_direct("BSPE_Fallbacks/sec    : ");
  serial_write_dec_direct((int)c_bspe_fallback);
  serial_write_direct("\n");
  serial_write_direct("  Reason: Tracker     : ");
  serial_write_dec_direct((int)c_bspe_reason_t);
  serial_write_direct("\n");
  serial_write_direct("  Reason: Eval        : ");
  serial_write_dec_direct((int)c_bspe_reason_e);
  serial_write_direct("\n");
  serial_write_direct("  Reason: Corrupt     : ");
  serial_write_dec_direct((int)c_bspe_reason_c);
  serial_write_direct("\n");
  serial_write_direct("  Reason: VRAM/Other  : ");
  serial_write_dec_direct((int)c_bspe_reason_v);
  serial_write_direct("\n");

  extern volatile uint64_t g_cursor_position_requests;
  extern volatile uint64_t g_cursor_fast_presents;
  extern volatile uint64_t g_cursor_updates_coalesced;
  extern volatile uint64_t g_cursor_fast_path_max_us;
  extern volatile uint64_t g_cursor_fast_path_total_us;
  extern volatile uint64_t g_cursor_blocked_by_compositor;
  extern volatile uint64_t g_cursor_fallback_invalid_state;
  extern volatile uint64_t g_cursor_fallback_vram_fail;

  extern volatile uint64_t g_cursor_pump_calls;
  extern volatile uint64_t g_cursor_pump_pending_consumed;
  extern volatile uint64_t g_cursor_pump_no_pending;
  extern volatile uint64_t g_cursor_pending_age_max_us;
  extern volatile uint64_t g_cursor_pending_over_2ms;
  extern volatile uint64_t g_cursor_pending_over_5ms;
  extern volatile uint64_t g_cursor_pending_over_16ms;
  extern volatile uint64_t g_cursor_pending_over_50ms;

  uint64_t c_cur_req = g_cursor_position_requests;
  g_cursor_position_requests = 0;
  uint64_t c_cur_fp = g_cursor_fast_presents;
  g_cursor_fast_presents = 0;
  uint64_t c_cur_coal = g_cursor_updates_coalesced;
  g_cursor_updates_coalesced = 0;
  uint64_t c_cur_blk = g_cursor_blocked_by_compositor;
  g_cursor_blocked_by_compositor = 0;
  uint64_t c_cur_fall_inv = g_cursor_fallback_invalid_state;
  g_cursor_fallback_invalid_state = 0;
  uint64_t c_cur_fall_vram = g_cursor_fallback_vram_fail;
  g_cursor_fallback_vram_fail = 0;
  uint64_t c_cur_max = g_cursor_fast_path_max_us;
  g_cursor_fast_path_max_us = 0;
  uint64_t c_cur_avg = 0;
  if (c_cur_fp > 0) {
    c_cur_avg = g_cursor_fast_path_total_us / c_cur_fp;
  }
  g_cursor_fast_path_total_us = 0;

  uint64_t c_pump_calls = g_cursor_pump_calls;
  g_cursor_pump_calls = 0;
  uint64_t c_pump_cons = g_cursor_pump_pending_consumed;
  g_cursor_pump_pending_consumed = 0;
  uint64_t c_pump_nop = g_cursor_pump_no_pending;
  g_cursor_pump_no_pending = 0;
  uint64_t c_age_max = g_cursor_pending_age_max_us;
  g_cursor_pending_age_max_us = 0;
  uint64_t c_age_2ms = g_cursor_pending_over_2ms;
  g_cursor_pending_over_2ms = 0;
  uint64_t c_age_5ms = g_cursor_pending_over_5ms;
  g_cursor_pending_over_5ms = 0;
  uint64_t c_age_16ms = g_cursor_pending_over_16ms;
  g_cursor_pending_over_16ms = 0;
  uint64_t c_age_50ms = g_cursor_pending_over_50ms;
  g_cursor_pending_over_50ms = 0;

  serial_write_direct("--- PHASE 3 CURSOR TELEMETRY ---\n");
  serial_write_direct("CursorPositionRequests/sec : ");
  serial_write_dec_direct((int)c_cur_req);
  serial_write_direct("\n");
  serial_write_direct("CursorFastPresents/sec     : ");
  serial_write_dec_direct((int)c_cur_fp);
  serial_write_direct("\n");
  serial_write_direct("CursorPumpCalls/sec        : ");
  serial_write_dec_direct((int)c_pump_calls);
  serial_write_direct("\n");
  serial_write_direct("CursorPumpPendingConsumed/s: ");
  serial_write_dec_direct((int)c_pump_cons);
  serial_write_direct("\n");
  serial_write_direct("CursorPumpNoPending/sec    : ");
  serial_write_dec_direct((int)c_pump_nop);
  serial_write_direct("\n");
  serial_write_direct("CursorPendingAgeMaxUs      : ");
  serial_write_dec_direct((int)c_age_max);
  serial_write_direct("\n");
  serial_write_direct("  > 2ms                    : ");
  serial_write_dec_direct((int)c_age_2ms);
  serial_write_direct("\n");
  serial_write_direct("  > 5ms                    : ");
  serial_write_dec_direct((int)c_age_5ms);
  serial_write_direct("\n");
  serial_write_direct("  > 16ms                   : ");
  serial_write_dec_direct((int)c_age_16ms);
  serial_write_direct("\n");
  serial_write_direct("  > 50ms                   : ");
  serial_write_dec_direct((int)c_age_50ms);
  serial_write_direct("\n");
  serial_write_direct("CursorUpdatesCoalesced/sec : ");
  serial_write_dec_direct((int)c_cur_coal);
  serial_write_direct("\n");
  serial_write_direct("CursorFastPathAvgUs        : ");
  serial_write_dec_direct((int)c_cur_avg);
  serial_write_direct("\n");
  serial_write_direct("CursorFastPathMaxUs        : ");
  serial_write_dec_direct((int)c_cur_max);
  serial_write_direct("\n");
  serial_write_direct("CursorBlockedByCompositor/s: ");
  serial_write_dec_direct((int)c_cur_blk);
  serial_write_direct("\n");
  serial_write_direct("CursorFallback_InvState/s  : ");
  serial_write_dec_direct((int)c_cur_fall_inv);
  serial_write_direct("\n");
  serial_write_direct("CursorFallback_VRAMFail/s  : ");
  serial_write_dec_direct((int)c_cur_fall_vram);
  serial_write_direct("\n");

  extern void FPJA_PrintReport(void);
  FPJA_PrintReport();
  serial_write_direct("==========================================\n");
}


volatile uint64_t g_usb_reports_count = 0;
volatile uint64_t g_usb_motion_reports_count = 0;
volatile uint64_t g_hid_decoded_motion_count = 0;
volatile uint64_t g_hid_max_gap_ms = 0;
volatile uint64_t g_cursor_damage_requests_count = 0;
volatile uint64_t g_frame_interval_min_ms = 999999;
volatile uint64_t g_frame_interval_max_ms = 0;
volatile uint64_t g_gui_yields = 0;
volatile uint64_t g_gui_hlts = 0;

/* Phase 3 Cursor Fast Path Telemetry */
volatile uint64_t g_cursor_position_requests = 0;
volatile uint64_t g_cursor_fast_presents = 0;
volatile uint64_t g_cursor_updates_coalesced = 0;
volatile uint64_t g_cursor_fast_path_avg_us = 0;
volatile uint64_t g_cursor_fast_path_max_us = 0;
volatile uint64_t g_cursor_fast_path_total_us = 0;
volatile uint64_t g_cursor_blocked_by_compositor = 0;
volatile uint64_t g_cursor_fallback_invalid_state = 0;
volatile uint64_t g_cursor_fallback_vram_fail = 0;

static void com1_dbg(const char *msg) {
    while (*msg) {
        if (*msg == '\n') {
            __asm__ __volatile__ ("outb %b0, %w1" : : "a"((uint8_t)'\r'), "Nd"((uint16_t)0x3F8));
        }
        __asm__ __volatile__ ("outb %b0, %w1" : : "a"((uint8_t)*msg), "Nd"((uint16_t)0x3F8));
        msg++;
    }
}

void kernel_main(boot_info_t *boot_info) {
  cpu_features_init();

  if (boot_info && boot_info->vbe_width > 0 && boot_info->vbe_height > 0) {
    g_kernel_screen_width = boot_info->vbe_width;
    g_kernel_screen_height = boot_info->vbe_height;
  }

  // 1. Display Subsystem
  console_set_backend(&vga_backend);
  display_init();
  display_clear();
  display_print(
      "ATOMS Kernel v0.9.8 - The Final Milestone Before Kernel v1.0\n");
  display_print("[RELEASE] Production NTFS Read-Only Certification\n\n");
  display_print("[BUILD_ID] USB_ONLY_DIAG_2026_07_19_A\n\n");
  if (boot_info) {
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
  }

  // Initialize new C-based GDT
  extern void gdt_init(void);
  gdt_init();
  display_print("GDT OK\n");

  /* Phase 6 discovery is read-only and keeps the known BSP/PIC path. APs are
     described but never marked online until direct AP execution exists. */
  atoms_smp_discover();
  atoms_smp_initialize_bsp();
  atoms_smp_prepare_aps();
  atoms_smp_print_diagnostics();

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
#if !AUDIO_TEST_MODE_ENABLED
  extern void vizier_init(void);
  vizier_init();
  kernel_input_init();
  keyboard_init();
#ifdef BMDE_DEBUG
  bmde_init();
#endif
  // Initialize all input device drivers for dynamic Input Device Manager (HIDA)
  extern void ps2_mouse_init(void);
  ps2_mouse_init();

  // Probe VMware VMMouse Backdoor (Absolute mode) after PS/2 controller reset
  extern bool vmmouse_init(uint32_t screen_w, uint32_t screen_h);
  extern uint32_t g_kernel_screen_width;
  extern uint32_t g_kernel_screen_height;
  bool vmmouse_ok = vmmouse_init(g_kernel_screen_width, g_kernel_screen_height);
  if (vmmouse_ok) {
    display_print("[INPUT] VMware VMMouse Absolute Pointer Initialized Successfully.\n");
  }

  // Report Input Device Manager (HIDA Arbiter) status
  extern void hida_dump_status(void);
  hida_dump_status();
#endif // !AUDIO_TEST_MODE_ENABLED

  // 5. Physical Memory Manager
  pmm_init(boot_info);
  display_print("PMM OK\n");

  // 6. VMM — Step 1 bring-up
  vmm_init();

  BOS_BootMode_Init(NULL);
  BOS_DeferredWork_Init();

  bos_security_init();
  bos_sandbox_init();

  // 7. Kernel Heap
  heap_init();
  atoms_p7_init(0);
  amsss_init();
  (void)amsss_register_defaults();

  // Initialize AGDPE and VBE Graphical Display Driver immediately after heap/vmm
  extern void AGDPE_Initialize(void);
  extern void AGDPE_VBE_Driver_Initialize(void *boot_info);
  if (boot_info && boot_info->vbe_framebuffer != 0) {
    AGDPE_Initialize();
    AGDPE_VBE_Driver_Initialize(boot_info);
    display_print("[AGDPE] VBE Graphical Console Active\n");
  }

  bos_html_init();
  bos_css_init();

  // Boot Mode Certification Test Scheduling
  if (BOS_IsTestMode()) {
    display_print("[BOOT_MODE] Running Synchronous Boot Certification Suite...\n");
    bos_security_run_certification_tests();
    bos_sandbox_run_certification_tests();
    extern void ATOMS_RunPhase10_VerificationSuite(void);
    ATOMS_RunPhase10_VerificationSuite();
    bos_html_run_certification_tests();
  } else {
    /* RELEASE & DEBUG MODE: Register tests into Deferred Work Queue for background idle execution */
    BOS_DeferredWork_Register("Security Certification", deferred_security_tests_wrapper);
    BOS_DeferredWork_Register("Sandbox Certification", deferred_sandbox_tests_wrapper);
    extern void ATOMS_RunPhase10_VerificationSuite(void);
    BOS_DeferredWork_Register("Phase 10 Verification Suite", ATOMS_RunPhase10_VerificationSuite);
    BOS_DeferredWork_Register("HTML Certification", deferred_html_tests_wrapper);
  }



  // Initialize PCI and xHCI (Phase 1 USB)
  extern void pci_init(void);
  pci_init();

  // Initialize BOS GPU Subsystem (Phase 1 HAL & GPU Manager)
  bos_gpu_init();
  bos_gpu_print_diagnostics();
  bos_gpu_run_tests();

  // Initialize BOS Cursor Engine V1.0 (BCE) with Windows 11 Concept Cursor Theme
  extern uint32_t bos_cursor_subsystem_init(void);
  extern bool bos_cursor_run_certification_suite(void);
  bos_cursor_subsystem_init();
  bos_cursor_run_certification_suite();

  extern void usb_registry_init(void);
  extern void usb_core_init(void);
  extern void usb_hid_init(void);
  extern void xhci_bte_init(void);
  extern void xhci_bte_run_certification_tests(void);
  extern void lhce_run_certification_tests(void);
  extern void ucue_run_certification_tests(void);
  extern void uhe_run_certification_tests(void);
  extern void ums_run_certification_tests(void);
  extern void usm_run_certification_tests(void);
  extern void bsec_run_certification_tests(void);

  usb_registry_init();
  usb_core_init();
  usb_hid_init();

  xhci_init();
  xhci_bte_init();
  xhci_bte_run_certification_tests();
  lhce_run_certification_tests();
  ucue_run_certification_tests();
  uhe_run_certification_tests();
  ums_run_certification_tests();
  usm_run_certification_tests();

  extern void e1000_init(void);
  e1000_init();
#if !AUDIO_TEST_MODE_ENABLED
  extern void BOImage_Init(void);
  BOImage_Init();
  extern void BOImage_v2_Init(void);
  BOImage_v2_Init();
#endif

  syscall_init();
  display_print("[PHASE7] Reliability diagnostics armed (BSP-safe, bounded)\n");
  display_print("SYS OK\n");

#if !AUDIO_TEST_MODE_ENABLED
  extern void conhost_init(void);
  conhost_init();
#endif

  // 8. Storage + VFS + FAT32 + NTFS
  extern int32_t BAR_Init(void);
  BAR_Init();
  extern void bar_run_certification_suite(void);
  bar_run_certification_suite();
  extern int32_t KERNEL32_Init(void);
  KERNEL32_Init();
  extern void kernel32_run_certification_suite(void);
  kernel32_run_certification_suite();
  extern int32_t USER32_Init(void);
  USER32_Init();
  extern void user32_run_certification_suite(void);
  user32_run_certification_suite();
  extern int32_t GDI32_Init(void);
  GDI32_Init();
  extern void gdi32_run_certification_suite(void);
  gdi32_run_certification_suite();
  extern int32_t COMDLG32_Init(void);
  COMDLG32_Init();
  extern void comdlg32_run_certification_suite(void);
  comdlg32_run_certification_suite();
  extern int32_t COMCTL32_Init(void);
  COMCTL32_Init();
  extern void comctl32_run_certification_suite(void);
  comctl32_run_certification_suite();
  extern int32_t ShellInitialize(void);
  ShellInitialize();
  extern void shell32_run_certification_suite(void);
  shell32_run_certification_suite();
  extern uint64_t BosInitialize(void);
  BosInitialize();
  extern void bosll_run_certification_suite(void);
  bosll_run_certification_suite();
  extern int32_t OpenGLInitialize(void);
  OpenGLInitialize();
  extern void opengl32_run_certification_suite(void);
  opengl32_run_certification_suite();
  extern int32_t AdvApiInitialize(void);
  AdvApiInitialize();
  extern void advapi32_run_certification_suite(void);
  advapi32_run_certification_suite();
  extern int32_t WS2_32Initialize(void);
  WS2_32Initialize();
  extern void ws2_32_run_certification_suite(void);
  ws2_32_run_certification_suite();
  extern int32_t Ole32Initialize(void);
  Ole32Initialize();
  extern void ole32_run_certification_suite(void);
  ole32_run_certification_suite();
  extern int32_t ExplorerInitialize(void);
  ExplorerInitialize();
  extern bool ExplorerStartSession(void);
  ExplorerStartSession();
  extern void explorer_run_certification_suite(void);
  explorer_run_certification_suite();
  extern int32_t ControlPanelInitialize(void);
  ControlPanelInitialize();
  extern void controlpanel_run_certification_suite(void);
  controlpanel_run_certification_suite();

  extern void disk_manager_init(void);
  extern void vfs_init(void);
  extern void fat32_init(void);
  extern void ntfs_init(void);
  extern void ntfs_run_tests(void);
  extern int block_device_count(void);

  disk_manager_init();
  display_print("DSK OK\n");

  vfs_init();
  fat32_init();
  ntfs_init();
  ntfs_run_tests();
  display_print("VFS OK\n");

  extern int32_t BDe_Init(void);
  BDe_Init();
  extern void dre_run_certification_suite(void);
  dre_run_certification_suite();
  extern void bdr_run_certification_suite(void);
  bdr_run_certification_suite();
  extern int32_t BSR_Init(void);
  BSR_Init();
  extern void bsr_run_certification_suite(void);
  bsr_run_certification_suite();
  extern int32_t BRT_Init(void);
  BRT_Init();
  extern void brt_run_certification_suite(void);
  brt_run_certification_suite();
  extern int32_t BFS_Init(void);
  BFS_Init();
  extern void bfs_run_certification_suite(void);
  bfs_run_certification_suite();
  extern int32_t BSOM_Init(void);
  BSOM_Init();
  extern void bsom_run_certification_suite(void);
  bsom_run_certification_suite();
  extern void explorer_rewrite_run_certification_suite(void);
  explorer_rewrite_run_certification_suite();
  extern int32_t AGP_Init(void);
  AGP_Init();
  extern void agp_run_certification_suite(void);
  agp_run_certification_suite();

  // Mount root filesystem — partition 1 is typically block device ID 1
  // (ID 0 = raw ATA drive, ID 1 = first MBR partition)
  if (vfs_get_mount("/")) {
    display_print("[VFS] Root (/) mounted successfully\n");
  } else {
    int bd_count = block_device_count();
    if (bd_count > 1) {
      int mount_result = vfs_mount_fs("/", 1, "fat32");
      if (mount_result == 0) {
        display_print("[VFS] Root (/) mounted successfully\n");
      } else {
        display_print("[VFS] Root mount failed, trying device 0\n");
        vfs_mount_fs("/", 0, "fat32");
      }
    } else if (bd_count > 0) {
      vfs_mount_fs("/", 0, "fat32");
    }
  }

#if !AUDIO_TEST_MODE_ENABLED
  // Initialize BOASSET Resource Manager and preload critical assets
  extern void BOAsset_Initialize(void);
  extern void BOAsset_PreloadCritical(void);
  BOAsset_Initialize();
  BOAsset_PreloadCritical();
  display_print("[BOASSET] Engine Initialized & Critical Assets Preloaded\n");

  extern void BOFont_Initialize(void);
  BOFont_Initialize();
  display_print("[BOFONT] Engine v2 Initialized & Default Atlas Generated\n");
#endif // !AUDIO_TEST_MODE_ENABLED

  // 9. Integrated Execution Management, Scheduler, User Mode & Timer
  ATOMS_Execution_Init();
  ATOMS_ProcessManager_Init();
  ATOMS_ThreadManager_Init();
  context_init();
  scheduler_init();
  extern void ATOMS_UserMode_Init(void);
  ATOMS_UserMode_Init();
  extern void ATOMS_UserMode_Init(void);
  ATOMS_UserMode_Init();
  timer_init(1000); // 1000 Hz = 1ms resolution
  ATOMS_P7StressReport phase7_report;
  atoms_p7_run_bounded_self_test(32, &phase7_report);
#if !AUDIO_TEST_MODE_ENABLED
  AME_Init(); // Initialize ATOMS Motion Engine Core Service
  display_print("TMR & AME OK\n");
#else
  display_print("TMR OK (AME skipped — AUDIO_TEST_MODE)\n");
#endif

#if AUDIO_TEST_MODE_ENABLED
  // ============================================================
  // AUDIO TEST MODE: Isolated Audio Boot
  // ============================================================
  audio_test_mode_entry(); // Init audio + open + play DEMO1.WAV

  scheduler_create_kernel_task("AudioSvc", audio_service_entry);
  display_print("[ATM] AudioSvc task spawned\n");

  // Register boot task and start scheduler so scheduler_on_tick/scheduler_yield
  // switch to AudioSvc
  extern void scheduler_register_boot_task(void);
  scheduler_register_boot_task();
  display_print("[ATM] Scheduler started (boot task registered)\n");

  display_print("[ATM] Enabling interrupts (sti)...\n");
  crash_log_add("[ATM] pre-sti");
  __asm__ volatile("sti");
  crash_log_add("[ATM] post-sti");

  display_print("[ATM] Entering idle loop — audio running in background\n");
  display_print("[ATM] Telemetry will print every ~1 second\n\n");

  // Simple idle loop — no GUI, no compositor, no rendering
  while (1) {
    extern void scheduler_yield(void);
    scheduler_yield();
  }

#else
  // ============================================================
  // NORMAL BOOT: Full GUI Desktop Experience
  // ============================================================
  display_print("[AUDIO] Initializing...\n");
  audio_init();
  audio_mixer_init();
  audio_hal_init();

  scheduler_create_kernel_task("AudioSvc", audio_service_entry);
  display_print("[AUDIO] Ready (Background Service Spawned)\n");

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

  // ----------------------------------------------------
  // BOGUI Phase 1: Graphics Foundation
  // ----------------------------------------------------
  extern void AGDPE_Initialize(void);
  extern void AGDPE_VBE_Driver_Initialize(void *boot_info);
  extern struct AGDPE_DisplayDevice *AGDPE_GetPrimaryDisplay(void);

  extern bool BOVISUAL_Init(const BVFramebuffer *framebuffer);
  extern void BOVISUAL_Text_Init(void);
  extern void BOVISUAL_Graphics_PutPixel(int32_t x, int32_t y,
                                         BOVISUAL_Color color);
  extern void BOVISUAL_Graphics_Fill(int32_t x, int32_t y, int32_t width,
                                     int32_t height, BOVISUAL_Color color);
  extern void BOVISUAL_Graphics_Clear(BOVISUAL_Color color);
  extern void BOVISUAL_Graphics_SwapBuffers(const BVFramebuffer *hw_fb);

  AGDPE_Initialize();
  AGDPE_VBE_Driver_Initialize(boot_info);

  extern void DIE_Initialize(void);
  DIE_Initialize();

  struct AGDPE_DisplayDevice *primary_dev = AGDPE_GetPrimaryDisplay();
  BVFramebuffer *hw_fb = &primary_dev->framebuffer;

  extern void AGDAE_Initialize(uint32_t, uint32_t, uint32_t, uint32_t);
  AGDAE_Initialize(hw_fb->width, hw_fb->height, g_kernel_screen_width,
                   g_kernel_screen_height);

  extern void BDCE_SeedFromCurrentSystem(const void *boot_info,
                                         const void *hw_fb);
  BDCE_SeedFromCurrentSystem(boot_info, hw_fb);

#include "kernel/loader/include/loader_types.h"
  extern loader_status_t bos_loader_init(void);
  extern bool loader_run_unit_tests(void);
  bos_loader_init();
  loader_run_unit_tests();

  extern ipc_status_t bos_ipc_init(void);
  extern bool ipc_run_unit_tests(void);
  bos_ipc_init();
  ipc_run_unit_tests();

  extern bool BDCE_ValidateCurrentSystem(const void *boot_info,
                                         const void *hw_fb, void *out_report);
  BDCE_ValidateCurrentSystem(boot_info, hw_fb, 0);

  BVFramebuffer fb;
  fb.buffer = (uint32_t *)boot_info->vbe_framebuffer;
  fb.width = g_kernel_screen_width;
  fb.height = g_kernel_screen_height;

  // ----------------------------------------------------
  // ROOK ENGINE V1.0: Runtime initialization & ATOMS Boot Splash Integration
  // Disable GUI graphical console so display_print logs to COM1 serial only
  // ----------------------------------------------------
  extern void display_gui_console_set_enabled(bool enabled);
  display_gui_console_set_enabled(false);

  rook_init((uint32_t *)hw_fb->buffer, hw_fb->width, hw_fb->height,
            hw_fb->pitch);
  rook_register_page(rook_page_boot_get());
  rook_register_page(rook_page_login_get());
  rook_goto(ROOK_PAGE_BOOT_SPLASH);
  
  /* Initial active boot splash animation (1.2s smooth 60 FPS spinner rotation) */
  rook_splash_spin(1200);

  // Phase 4/5: Back Buffer Allocation
  static BVFramebuffer back_fb;
  back_fb.width = hw_fb->width;
  back_fb.height = hw_fb->height;
  back_fb.pitch = hw_fb->pitch;

  uint64_t fb_size = hw_fb->height * hw_fb->pitch;
  uint64_t num_pages = (fb_size + 4095) / 4096;
  uint64_t bb_vaddr = 0x90000000ULL;

  void *active_pml4 = vmm_get_active_pml4();
  for (uint64_t i = 0; i < num_pages; i++) {
    vmm_alloc_mapped_page(active_pml4, bb_vaddr + (i * 4096),
                          PAGE_WRITABLE | PAGE_USER);
  }
  back_fb.buffer = (BOVISUAL_Color *)bb_vaddr;

  BOVISUAL_Init(&back_fb);
  BOVISUAL_Text_Init();
  BVCursor_Init(hw_fb->width, hw_fb->height);

  // ----------------------------------------------------
  // BWE Phase 4: Integrated GUI Loop
  // ----------------------------------------------------
  BOVISUAL_Color bg_color = 0xFF222222;
  BOVISUAL_Graphics_Clear(bg_color);

  extern void Identity_Init(void);
  extern uint32_t BWE_Initialize(void);

#define DEBUG_DOOM_DIRECT_BOOT 0

#ifndef DEBUG_DOOM_DIRECT_BOOT
  display_print("[DIAG] Step A: Identity_Init\n");
  Identity_Init();
  rook_splash_spin(100);

#include "kernel/loader/include/loader_types.h"
  extern loader_status_t bos_loader_init(void);
  extern bool loader_run_unit_tests(void);
  bos_loader_init();
  loader_run_unit_tests();
  rook_splash_spin(100);
#endif

  extern void BOTHEME_Initialize(void);
  display_print("[DIAG] Step B: BOTHEME_Initialize & BWE_Initialize\n");
  BOTHEME_Initialize();
  BWE_Initialize();
  rook_splash_spin(100);

  extern uint32_t Desktop_Shell_Initialize(void);
  display_print("[DIAG] Step B2: Desktop_Shell_Initialize\n");
  Desktop_Shell_Initialize();
  extern void ATOMS_RunPhase9_VerificationSuite(void);
  ATOMS_RunPhase9_VerificationSuite();
  extern void Desktop_RunPhase28_CertificationSuite(void);
  Desktop_RunPhase28_CertificationSuite();
  rook_splash_spin(100);

  display_print("[DIAG] Step C: Horse Engine & DOOM Direct Boot\n");
  extern void horse_init(void);
  horse_init();
  rook_splash_spin(100);

  display_print("[DIAG] Step D: AGDTE_Initialize\n");
  extern int AGDTE_Initialize(void);
  AGDTE_Initialize();
  rook_splash_spin(100);

  /* Boot transition complete: Hand over to Login Page (Lock Screen / Sign In) */
  rook_goto(ROOK_PAGE_LOGIN);

#if DEBUG_DOOM_DIRECT_BOOT
  extern void *vmm_create_address_space(void);
  extern void *process_spawn(ProcessImage * image, const char *name);

  void *new_pml4 = vmm_create_address_space();
  ProcessImage *new_image = elf_load_image(new_pml4, "/DOOM.ELF");
  if (new_image) {
    if (process_build_user_stack(new_image, new_pml4)) {
      display_print("[SUCCESS] SKIPPED manual Spawned DOOM.ELF\n");
    } else {
      display_print("[FATAL] DOOM user stack failed!\n");
    }
  } else {
    display_print("[FATAL] DOOM.ELF not found!\n");
  }
#endif

  display_print("[DIAG] Step D: AGDTE_Initialize\n");
  extern int AGDTE_Initialize(void);
  AGDTE_Initialize();

  display_print("[DIAG] Step E: BWE_Compose\n");
  BWE_Compose();

  display_print("[DIAG] Step F: scheduler_register_boot_task\n");
  scheduler_register_boot_task();

  // Disable GUI graphical console so display_print logs directly to COM1 serial
  display_gui_console_set_enabled(false);

  display_print("[DIAG] Step H: sti\n");
  crash_log_add("[BOOT] Step H: pre-sti");
  __asm__ volatile("sti");
  crash_log_add("[BOOT] Step H: post-sti");

#ifdef ENABLE_BOOT_GL_TESTS
  // Run ATOMS OS OpenGL Phase 8 Mipmapping & Readback Verification Suite FIRST
  extern void run_phase8_gl_verification_suite(void);
  run_phase8_gl_verification_suite();

  // Run ATOMS OS OpenGL Phase 9 Advanced Raster Operations Verification Suite
  extern void run_phase9_gl_verification_suite(void);
  run_phase9_gl_verification_suite();

  // Run ATOMS OS OpenGL Phase 10 Offscreen Rendering / FBO / Render-to-Texture
  // Suite
  extern void run_phase10_gl_verification_suite(void);
  run_phase10_gl_verification_suite();

  // Run ATOMS OS OpenGL Phase 11 ATOMS GRAPH 3D Benchmark & Stress Verification
  // Suite
  extern void run_phase11_gl_verification_suite(void);
  run_phase11_gl_verification_suite();
#endif
  crash_log_add("[BOOT] Step H: post-sti");

  // ================================================================
  // Phase 24 — Settings.BOSX V1.0 Modern Settings Application
  // ================================================================
  extern int32_t SettingsInitialize(void);
  SettingsInitialize();
  extern void settings_run_certification_suite(void);
  settings_run_certification_suite();

  // ================================================================
  // Phase 25 — Terminal.BOSX V1.0 Native Console Host & Command Runtime
  // ================================================================
  extern int32_t TerminalInitialize(void);
  TerminalInitialize();
  extern void terminal_run_certification_suite(void);
  terminal_run_certification_suite();

  // ================================================================
  // Phase 26 — TaskManager.BOSX V1.0 Native System Monitor & Hardware Diagnostics Center
  // ================================================================
  extern int32_t TaskManagerInitialize(void);
  TaskManagerInitialize();
  extern void taskmgr_run_certification_suite(void);
  taskmgr_run_certification_suite();

  // ================================================================
  // Phase 27 — FileExplorer.BOSX V1.0 Enterprise Storage Shell
  // ================================================================
  extern int32_t FileExplorerInitialize(void);
  FileExplorerInitialize();
  extern void fileexplorer_run_certification_suite(void);
  fileexplorer_run_certification_suite();

  // ================================================================
  // Transition to GUI Mode: disable graphical console output.
  // From this point, display_print() writes to serial (COM1) only.
  // ================================================================

  display_print("[DIAG] Step K: Entering main loop\n");
  crash_log_add("[BOOT] Step K: pre-loop");

  extern void BOF_BeginAtomicFrame(void);
  crash_log_add("[BOOT] pre-BOF_BeginAtomicFrame");
  BOF_BeginAtomicFrame();
  crash_log_add("[BOOT] post-BOF_BeginAtomicFrame");

  while (1) {
    g_main_loop_iterations_count++;
    static bool s_was_playing = false;
    extern bool audio_player_is_playing(void);
    bool is_playing = audio_player_is_playing();

    if (!is_playing && s_was_playing) {
      // Just stopped playing! Dump BRE telemetry
      extern void bre_get_telemetry(void *);
      struct {
        uint32_t total_signals[32];
        uint32_t coalesced_signals[32];
        uint32_t total_dispatches[32];
        uint32_t budget_exhaustions[32];
        uint32_t max_dispatch_us[32];
        uint64_t total_dispatch_time_us[32];
        uint32_t invariant_failures[32];
      } bre_stats;
      bre_get_telemetry(&bre_stats);

      display_print("\n=== BOS REFLEX ENGINE (BRE) TELEMETRY ===\n");
      display_print("Audio Signals: ");
      display_print_dec(bre_stats.total_signals[0]);
      display_print("\n");
      display_print("Audio Coalesced: ");
      display_print_dec(bre_stats.coalesced_signals[0]);
      display_print("\n");
      display_print("Audio Dispatches: ");
      display_print_dec(bre_stats.total_dispatches[0]);
      display_print("\n");
      display_print("Audio Budg Exhaust: ");
      display_print_dec(bre_stats.budget_exhaustions[0]);
      display_print("\n");
      display_print("Audio Max Dur(us): ");
      display_print_dec(bre_stats.max_dispatch_us[0]);
      display_print("\n");
      display_print("Audio Invar Fails: ");
      display_print_dec(bre_stats.invariant_failures[0]);
      display_print("\n");
      display_print("=========================================\n");

      // Also dump the forensic summary
      extern void ac97_forensic_session_dump(void);
      ac97_forensic_session_dump();
    }
    s_was_playing = is_playing;

    if (!is_playing && g_enable_runtime_telemetry)
      print_1sec_telemetry();


    extern void xhci_poll(void);
    xhci_poll();

    extern void vmmouse_poll(void);
    vmmouse_poll();

    extern void input_adapter_pump(void);
    input_adapter_pump();
    extern void BWE_PumpEvents(void);
    BWE_PumpEvents();

    extern void BSPE_CursorPresenter_PumpFastPath(void);
    BSPE_CursorPresenter_PumpFastPath();

    extern uint64_t timer_get_ticks(void);
    static uint64_t next_frame_deadline = 0;
    static uint64_t last_present_ticks = 0;
    uint64_t current_ticks = timer_get_ticks();

    if (next_frame_deadline == 0) {
      next_frame_deadline = current_ticks + 16;
    }

    if (current_ticks >= next_frame_deadline) {
      uint64_t frames_passed = ((current_ticks - next_frame_deadline) / 16) + 1;
      next_frame_deadline += (frames_passed * 16);

      if (last_present_ticks != 0) {
        uint64_t actual_interval = current_ticks - last_present_ticks;
        extern volatile uint64_t g_frame_interval_min_ms;
        extern volatile uint64_t g_frame_interval_max_ms;
        if (actual_interval < g_frame_interval_min_ms)
          g_frame_interval_min_ms = actual_interval;
        if (actual_interval > g_frame_interval_max_ms)
          g_frame_interval_max_ms = actual_interval;
      }
      last_present_ticks = current_ticks;
    } else {
      uint64_t wait_ms = next_frame_deadline - current_ticks;
      if (wait_ms > 2) {

        // Poll xhci and input adapter during the wait
        extern void xhci_poll(void);
        xhci_poll();
        extern void input_adapter_pump(void);
        input_adapter_pump();
        extern uint32_t kernel_input_get_queue_size(void);

        extern void BWE_PumpEvents(void);
        BWE_PumpEvents();

        extern void BSPE_CursorPresenter_PumpFastPath(void);
        BSPE_CursorPresenter_PumpFastPath();

        // If input arrived, do not yield to scheduler, process immediately next
        // iteration
        if (kernel_input_get_queue_size() > 0) {
          continue;
        }

        /* Pump background deferred work tasks during idle frame passes */
        BOS_DeferredWork_PumpIdleQueue();

        extern uint32_t scheduler_get_task_count(void);

        if (scheduler_get_task_count() > 0) {
          extern volatile uint64_t g_gui_yields;
          g_gui_yields++;
          extern void scheduler_yield(void);
          scheduler_yield();
        } else {
          extern volatile uint64_t g_gui_hlts;
          g_gui_hlts++;
          __asm__ volatile("sti");
          __asm__ volatile("hlt" : : : "memory");
        }
      }
      continue;
    }

    extern void FPJA_FrameBegin(void);
    extern void FPJA_FrameEnd(void);
    FPJA_FrameBegin();

    crash_log_add("[LOOP] pre-BOHeart_Pulse");
    BOHeart_Pulse(hw_fb);
    crash_log_add("[LOOP] post-BOHeart_Pulse");
    extern bool AGDTE_IsInitialized(void);
    extern int AGDTE_Pulse(uint64_t current_time_us);
    if (AGDTE_IsInitialized()) {
      AGDTE_Pulse(timer_get_ticks() * 1000ULL);
    }
#ifdef TEST_BUILD
    step14_telemetry_on_frame();
#endif

    FPJA_FrameEnd();

    extern void bodebug_dump(void);
    bodebug_dump();
  }
#endif // !AUDIO_TEST_MODE_ENABLED
}

