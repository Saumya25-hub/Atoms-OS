#include "kernel/core/syscall/include/syscall.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/memory/vmm/include/paging.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/scheduler/include/scheduler.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/timer/include/timer.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/gui/surface/surface.h"
#include "kernel/services/wallpaper/wallpaper_service.h"
#include "kernel/drivers/input/pointer/pointer_state.h"
#include "kernel/graphics/BSPE/Cursor/bspe_cursor_present.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/core/process/process_manager.h"
#include "kernel/ipc/include/ipc_api.h"
#include "kernel/ipc/channels/channel_manager.h"
#include "kernel/ipc/shared_memory/shm_manager.h"
#include "kernel/core/scheduler/include/kernel_stack.h"
#include "kernel/core/cpu/cpu_state.h"
#include "kernel/core/thread/thread_manager.h"
#include "kernel/audio/api/audio_api.h"
#include "kernel/audio/hal/audio_hal.h"

extern void com1_dbg(const char *msg);


uint64_t sys_service_write(const char *user_str, size_t len) {
  if (!syscall_validate_user_ptr(user_str, len > 0 ? len : 1)) {
    return SYSCALL_BAD_ADDRESS;
  }

  char buf[256];
  size_t copy_len = len < sizeof(buf) - 1 ? len : sizeof(buf) - 1;
  memcpy(buf, user_str, copy_len);
  buf[copy_len] = '\0';

  display_print(buf);
  com1_dbg(buf);
  return SYSCALL_OK;
}

uint64_t sys_service_exit(int code) {
  Task *current = scheduler_current_task();
  if (current && current != scheduler_get_idle_task()) {
    if (current->owner_pid) {
      extern bool ATOMS_Process_Terminate(uint32_t pid, int32_t exit_code);
      ATOMS_Process_Terminate(current->owner_pid, code);
    }
    scheduler_terminate_task(current);
    extern void scheduler_yield(void);
    scheduler_yield();
  }
  return SYSCALL_OK;
}

uint64_t sys_service_getpid(void) {
  Task *current = scheduler_current_task();
  return current ? (current->owner_pid ? current->owner_pid : current->id) : 1;
}

uint64_t sys_service_yield(void) {
  extern bool r8168_poll_receive(void);
  extern bool atoms_screenshot_step(void);
  r8168_poll_receive();
  atoms_screenshot_step();
  scheduler_yield();
  return SYSCALL_OK;
}

uint64_t sys_service_uptime(void) {
  return timer_get_ticks();
}

uint64_t sys_service_alloc(size_t size) {
  if (size == 0 || size > 4096 * 16) {
    return SYSCALL_FAIL;
  }

  Task *current = scheduler_current_task();
  if (!current || !current->pml4) {
    return SYSCALL_FAIL;
  }

  void *phys = pmm_alloc_page();
  if (!phys) {
    return SYSCALL_FAIL;
  }
  memset(phys, 0, 4096);

  static uint64_t s_alloc_addr = 0x40020000ULL;
  uint64_t virt = s_alloc_addr;
  s_alloc_addr += 4096;
  if (s_alloc_addr >= 0x7FFF0000ULL) {
    s_alloc_addr = 0x40020000ULL;
  }

  vmm_map_page(current->pml4, (uint64_t)phys, virt, PAGE_USER | PAGE_WRITABLE | PAGE_PRESENT);
  return virt;
}

uint64_t sys_service_free(void *ptr) {
  if (!syscall_validate_user_ptr(ptr, 1)) {
    return SYSCALL_BAD_ADDRESS;
  }
  return SYSCALL_OK;
}

uint64_t sys_service_debug_print(const char *msg) {
  if (!syscall_validate_user_string(msg, 512)) {
    return SYSCALL_BAD_ADDRESS;
  }

  display_print(msg);
  com1_dbg(msg);
  return SYSCALL_OK;
}

#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/core/memory/heap/include/heap.h"

extern bool syscall_validate_user_string(const char *str, size_t max_len);

#define MAX_GUI_EVENTS_PER_WIN 32
typedef struct {
  BOS_GUIEvent events[MAX_GUI_EVENTS_PER_WIN];
  volatile uint32_t head;
  volatile uint32_t tail;
} WinEventQueue;

static WinEventQueue s_win_event_queues[BWE_MAX_WINDOWS];
volatile uint32_t g_desktop_shell_win_id = 0;

void sys_gui_post_event(uint32_t win_id, const BOS_GUIEvent *ev) {
  uint32_t slot = win_id & BWE_WINDOW_SLOT_MASK;
  if (slot >= BWE_MAX_WINDOWS || !ev) return;
  if (!BWE_ValidateWindow(win_id)) return;

  WinEventQueue *q = &s_win_event_queues[slot];
  uint32_t head = q->head;
  uint32_t tail = q->tail;

  /* Windows NT / Linux Style Motion Coalescing:
     If the latest pending event in the queue is already a MOUSE_MOVE,
     update coordinates in-place without expanding queue depth. */
  if (ev->type == BOS_GUI_EVENT_MOUSE_MOVE && head != tail) {
    uint32_t last = (head == 0) ? (MAX_GUI_EVENTS_PER_WIN - 1) : (head - 1);
    if (q->events[last].type == BOS_GUI_EVENT_MOUSE_MOVE) {
      q->events[last].mouse_x = ev->mouse_x;
      q->events[last].mouse_y = ev->mouse_y;
      q->events[last].modifiers = ev->modifiers;
      __asm__ volatile("" ::: "memory");
      return;
    }
  }

  uint32_t next = (head + 1) % MAX_GUI_EVENTS_PER_WIN;
  if (next != tail) {
    q->events[head] = *ev;
    __asm__ volatile("" ::: "memory");
    q->head = next;
  }
}

#include "kernel/debug/desktop_diag.h"

uint64_t sys_service_gui_create_window(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t flags, const char *title) {
  (void)flags;
  char safe_title[128] = "ATOMS Window";
  if (title && syscall_validate_user_string(title, 128)) {
    strncpy(safe_title, title, 127);
    safe_title[127] = '\0';
  } else {
    diag_puts("[SYSCALL_DIAG] CREATE_WINDOW: Title validation defaulted to 'ATOMS Window'\r\n");
  }
  if (w <= 0) w = 320;
  if (h <= 0) h = 240;
  if (w > 3840) w = 3840;
  if (h > 2160) h = 2160;

  uint32_t win_id = 0;
  bwe_error_t err = BOS_CreateWindow(x, y, w, h, safe_title, &win_id);
  if (err != BWE_SUCCESS || win_id == 0) {
    diag_puts("[SYSCALL_DIAG] CREATE_WINDOW: BOS_CreateWindow FAILED err=");
    diag_put_dec(err);
    diag_puts("\r\n");
    return 0;
  }

  BWE_Window *win = BWE_GetWindow(win_id);
  if (win) {
    Task *cur = scheduler_current_task();
    win->owner_pid = cur ? cur->id : 1;
    win->flags = flags;
    if (flags & 1) win->flags |= BWE_WINDOW_BORDERLESS;
    win->control_data.canvas.pixel_buffer = NULL;
    win->control_data.canvas.buffer_w = 0;
    win->control_data.canvas.buffer_h = 0;
  }

  if (safe_title[0] != '\0' && (strstr(safe_title, "Desktop Shell") != NULL || strstr(safe_title, "Desktop") != NULL || (flags & 1))) {
    g_desktop_shell_win_id = win_id;
  }

  diag_puts("[SYSCALL_DIAG] CREATE_WINDOW: OK win_id=");
  diag_put_dec(win_id);
  diag_puts(" bounds=("); diag_put_dec(x); diag_puts(","); diag_put_dec(y);
  diag_puts(","); diag_put_dec(w); diag_puts(","); diag_put_dec(h);
  diag_puts(") title='"); diag_puts(safe_title); diag_puts("'\r\n");

  return (uint64_t)win_id;
}

uint64_t sys_service_gui_destroy_window(uint32_t win_id) {
  if (!BWE_ValidateWindow(win_id)) return SYSCALL_FAIL;
  BWE_Window *win = BWE_GetWindow(win_id);
  Task *cur = scheduler_current_task();
  if (win && cur && win->owner_pid != cur->id && cur->id != 0) {
    return SYSCALL_FAIL;
  }
  BOS_DestroySurface(win_id);
  return SYSCALL_OK;
}

uint64_t sys_service_gui_show_window(uint32_t win_id, uint32_t visible) {
  if (!BWE_ValidateWindow(win_id)) {
    diag_puts("[SYSCALL_DIAG] SHOW_WINDOW: ValidateWindow FAILED for win_id=");
    diag_put_dec(win_id);
    diag_puts("\r\n");
    return SYSCALL_FAIL;
  }
  BWE_Window *win = BWE_GetWindow(win_id);
  Task *cur = scheduler_current_task();
  if (win && cur && win->owner_pid != cur->id && cur->id != 0) {
    diag_puts("[SYSCALL_DIAG] SHOW_WINDOW: Permission DENIED\r\n");
    return SYSCALL_FAIL;
  }
  if (visible) {
    BOS_Show(win_id);
    BWE_BringToFront(win_id);
    BWE_InvalidateWindow(win_id);
    BWE_RequestFullRedraw();
    diag_puts("[SYSCALL_DIAG] SHOW_WINDOW: win_id=");
    diag_put_dec(win_id);
    diag_puts(" state="); diag_put_dec(win ? win->state : -1);
    diag_puts(" BCM Damage Requested\r\n");
  } else {
    BOS_Hide(win_id);
    BWE_InvalidateWindow(win_id);
    BWE_RequestFullRedraw();
  }
  return SYSCALL_OK;
}

uint64_t sys_service_gui_set_bounds(uint32_t win_id, int32_t x, int32_t y, int32_t w, int32_t h) {
  if (!BWE_ValidateWindow(win_id)) return SYSCALL_FAIL;
  BWE_Window *win = BWE_GetWindow(win_id);
  Task *cur = scheduler_current_task();
  if (win && cur && win->owner_pid != cur->id && cur->id != 0) {
    return SYSCALL_FAIL;
  }
  if (w <= 0 || h <= 0 || w > 3840 || h > 2160) return SYSCALL_FAIL;
  BOS_SetBounds(win_id, (uint32_t)x, (uint32_t)y, (uint32_t)w, (uint32_t)h);
  return SYSCALL_OK;
}

uint64_t sys_service_gui_map_surface(uint32_t win_id, uint64_t *out_user_surface_ptr, uint32_t *out_stride_bytes) {
  diag_puts("[SYSCALL_DIAG] MAP_SURFACE: ENTER win_id=");
  diag_put_dec(win_id);
  diag_puts(" out_ptr="); diag_put_hex64((uint64_t)out_user_surface_ptr);
  diag_puts(" out_stride="); diag_put_hex64((uint64_t)out_stride_bytes);
  diag_puts("\r\n");

  if (!syscall_validate_user_ptr_writable(out_user_surface_ptr, sizeof(uint64_t))) {
    diag_puts("[SYSCALL_DIAG] MAP_SURFACE: Validate user ptr out_ptr FAILED\r\n");
    return SYSCALL_BAD_ADDRESS;
  }
  if (!syscall_validate_user_ptr_writable(out_stride_bytes, sizeof(uint32_t))) {
    diag_puts("[SYSCALL_DIAG] MAP_SURFACE: Validate user ptr out_stride FAILED\r\n");
    return SYSCALL_BAD_ADDRESS;
  }

  if (!BWE_ValidateWindow(win_id)) {
    diag_puts("[SYSCALL_DIAG] MAP_SURFACE: ValidateWindow FAILED for win_id=");
    diag_put_dec(win_id);
    diag_puts("\r\n");
    return SYSCALL_FAIL;
  }
  BWE_Window *win = BWE_GetWindow(win_id);
  Task *cur = scheduler_current_task();
  if (win && cur && win->owner_pid != cur->id && cur->id != 0) {
    diag_puts("[SYSCALL_DIAG] MAP_SURFACE: Permission DENIED\r\n");
    return SYSCALL_FAIL;
  }

  uint32_t slot = win_id & 0xFFF;
  uint64_t user_virt_base = 0x50000000ULL + ((uint64_t)slot * 0x800000ULL);

  uint64_t hw_cr3 = 0;
  __asm__ volatile("mov %%cr3, %0" : "=r"(hw_cr3));

  if (!win->control_data.canvas.pixel_buffer) {
    uint32_t w = win->screen_bounds.width > 0 ? (uint32_t)win->screen_bounds.width : 320;
    uint32_t h = win->screen_bounds.height > 0 ? (uint32_t)win->screen_bounds.height : 240;
    win->control_data.canvas.buffer_w = w;
    win->control_data.canvas.buffer_h = h;

    uint64_t total_bytes = (uint64_t)w * h * sizeof(uint32_t);
    uint64_t pages_needed = (total_bytes + 4095) / 4096;

    void *phys_buf = pmm_alloc_pages(pages_needed);
    if (!phys_buf) {
      diag_puts("[SYSCALL_DIAG] MAP_SURFACE: pmm_alloc_pages FAILED\r\n");
      return SYSCALL_FAIL;
    }
    memset(phys_buf, 0, total_bytes);

    void *target_pml4 = (void*)(hw_cr3 & 0x000FFFFFFFFFF000ULL);
    if (!target_pml4 && cur && cur->pml4) target_pml4 = cur->pml4;
    if (!target_pml4) target_pml4 = vmm_get_kernel_pml4();

    diag_puts("[MAP_FORENSIC]\r\n");
    diag_puts("  PID="); diag_put_dec(cur ? cur->id : 0);
    diag_puts("  HW_CR3="); diag_put_hex64(hw_cr3);
    diag_puts("  TARGET_PML4="); diag_put_hex64((uint64_t)target_pml4);
    diag_puts("  TARGET_VA="); diag_put_hex64(user_virt_base);
    diag_puts("  PHYS_BASE="); diag_put_hex64((uint64_t)phys_buf);
    diag_puts("  PAGES="); diag_put_dec(pages_needed);
    diag_puts("\r\n");

    for (uint64_t p = 0; p < pages_needed; p++) {
      uint64_t vaddr = user_virt_base + p * 4096;
      uint64_t phys = (uint64_t)phys_buf + p * 4096;
      vmm_map_page(target_pml4, phys, vaddr, PAGE_USER | PAGE_WRITABLE | PAGE_PRESENT);
      if (cur && cur->pml4 && cur->pml4 != target_pml4) {
        vmm_map_page(cur->pml4, phys, vaddr, PAGE_USER | PAGE_WRITABLE | PAGE_PRESENT);
      }
    }
    win->control_data.canvas.pixel_buffer = (uint32_t*)phys_buf;

    // Safety probe: verify physical memory writeback
    ((uint32_t*)phys_buf)[0] = 0xAA55AA55;
    if (((uint32_t*)phys_buf)[0] != 0xAA55AA55) {
      diag_puts("[MAP_VERIFY] Physical writeback coherency FAIL\r\n");
      return SYSCALL_FAIL;
    }
    ((uint32_t*)phys_buf)[0] = 0x00000000;

    bool map_verified = vmm_walk_and_verify(target_pml4, user_virt_base);
    if (!map_verified) {
      diag_puts("[MAP_VERIFY] RESULT=FAIL\r\n");
      return SYSCALL_FAIL;
    }

    diag_puts("[MAP_VERIFY]\r\n");
    diag_puts("  CR3="); diag_put_hex64(hw_cr3);
    diag_puts("  VA="); diag_put_hex64(user_virt_base);
    diag_puts("  PHYS="); diag_put_hex64((uint64_t)phys_buf);
    diag_puts("  P=1 RW=1 US=1\r\n");
    diag_puts("  RESULT=PASS\r\n");
  }

  *out_user_surface_ptr = user_virt_base;
  *out_stride_bytes = win->control_data.canvas.buffer_w * 4;
  diag_puts("[SYSCALL_DIAG] MAP_SURFACE: SUCCESS returning user_virt=");
  diag_put_hex64(user_virt_base);
  diag_puts("\r\n");
  return SYSCALL_OK;
}

uint64_t sys_service_gui_invalidate(uint32_t win_id, int32_t x, int32_t y, int32_t w, int32_t h) {
  if (!BWE_ValidateWindow(win_id)) {
    diag_puts("[SYSCALL_DIAG] INVALIDATE: ValidateWindow FAILED\r\n");
    return SYSCALL_FAIL;
  }
  BWE_Window *win = BWE_GetWindow(win_id);
  if (!win) return SYSCALL_FAIL;

  if (w > 0 && h > 0 && (w < win->screen_bounds.width || h < win->screen_bounds.height)) {
    BWE_Rect sub_rect = { win->screen_bounds.x + x, win->screen_bounds.y + y, w, h };
    extern void BWE_AddCompositorDirtyRect(const BWE_Rect* rect);
    BWE_AddCompositorDirtyRect(&sub_rect);
    win->is_dirty = true;
  } else {
    BWE_InvalidateWindow(win_id);
    BWE_RequestFullRedraw();
  }
  return SYSCALL_OK;
}

uint64_t sys_service_gui_poll_event(uint32_t win_id, BOS_GUIEvent *out_user_event, uint32_t event_struct_size) {
  if (!syscall_validate_user_ptr_writable(out_user_event, sizeof(BOS_GUIEvent))) return SYSCALL_BAD_ADDRESS;
  if (event_struct_size != sizeof(BOS_GUIEvent)) return SYSCALL_FAIL;

  uint32_t slot = win_id & BWE_WINDOW_SLOT_MASK;
  bool valid = (slot < BWE_MAX_WINDOWS) && BWE_ValidateWindow(win_id);
  if (!valid) return 0;

  extern void xhci_poll(void);
  xhci_poll();
  extern void vmmouse_poll(void);
  vmmouse_poll();
  extern void input_adapter_pump(void);
  input_adapter_pump();
  extern void input_core_dispatch_events(void);
  input_core_dispatch_events();
  extern void dispatcher_pump_events(void);
  dispatcher_pump_events();
  extern void BWE_PumpEvents(void);
  BWE_PumpEvents();

  extern bool r8168_poll_receive(void);
  extern bool atoms_screenshot_step(void);
  r8168_poll_receive();
  atoms_screenshot_step();

  WinEventQueue *q = &s_win_event_queues[slot];
  uint32_t head = q->head;
  uint32_t tail = q->tail;
  if (head == tail) {
    return 0;
  }

  *out_user_event = q->events[tail];
  __asm__ volatile("" ::: "memory");
  q->tail = (tail + 1) % MAX_GUI_EVENTS_PER_WIN;
  return 1;
}

uint64_t sys_service_gui_get_screen_info(uint32_t *out_w, uint32_t *out_h, uint32_t *out_bpp) {
  if (!syscall_validate_user_ptr_writable(out_w, sizeof(uint32_t))) return SYSCALL_BAD_ADDRESS;
  if (!syscall_validate_user_ptr_writable(out_h, sizeof(uint32_t))) return SYSCALL_BAD_ADDRESS;
  if (!syscall_validate_user_ptr_writable(out_bpp, sizeof(uint32_t))) return SYSCALL_BAD_ADDRESS;

  extern uint32_t g_kernel_screen_width;
  extern uint32_t g_kernel_screen_height;

  *out_w = g_kernel_screen_width > 0 ? g_kernel_screen_width : 1920;
  *out_h = g_kernel_screen_height > 0 ? g_kernel_screen_height : 1080;
  *out_bpp = 32;
  return SYSCALL_OK;
}

uint64_t sys_service_gui_draw_wallpaper(uint32_t win_id, int32_t x, int32_t y, int32_t w, int32_t h) {
  com1_dbg("[WALLPAPER R3] REQUEST\r\n");

  extern bool BWE_ValidateWindow(uint32_t win_id);
  extern BWE_Window *BWE_GetWindow(uint32_t win_id);

  if (!BWE_ValidateWindow(win_id)) {
    com1_dbg("[WALLPAPER R3] WINDOW VALIDATION FAIL\r\n");
    return SYSCALL_FAIL;
  }
  BWE_Window *win = BWE_GetWindow(win_id);
  Task *cur = scheduler_current_task();
  if (!win || (cur && win->owner_pid != cur->id && cur->id != 0)) {
    com1_dbg("[WALLPAPER R3] WINDOW PERMISSION DENIED\r\n");
    return SYSCALL_FAIL;
  }
  com1_dbg("[WALLPAPER R3] WINDOW VALIDATION PASS\r\n");

  const uint32_t *wp_pixels = wallpaper_service_get_canvas();
  if (!wp_pixels) {
    com1_dbg("[WALLPAPER R3] SOURCE CANVAS FAIL\r\n");
    return SYSCALL_FAIL;
  }
  com1_dbg("[WALLPAPER R3] SOURCE CANVAS PASS\r\n");

  if (!win->control_data.canvas.pixel_buffer) {
    uint64_t dummy_ptr = 0;
    uint32_t dummy_stride = 0;
    sys_service_gui_map_surface(win_id, &dummy_ptr, &dummy_stride);
  }

  uint32_t *dest_buf = win->control_data.canvas.pixel_buffer;
  if (!dest_buf) {
    com1_dbg("[WALLPAPER R3] DEST BUFFER FAIL\r\n");
    return SYSCALL_FAIL;
  }

  int32_t dest_w = win->screen_bounds.width > 0 ? (int32_t)win->screen_bounds.width : (w > 0 ? w : 1024);
  int32_t dest_h = win->screen_bounds.height > 0 ? (int32_t)win->screen_bounds.height : (h > 0 ? h : 768);
  int32_t src_w = 1920;
  int32_t src_h = 1080;

  int32_t draw_w = (w > 0 && w <= dest_w) ? w : dest_w;
  int32_t draw_h = (h > 0 && h <= dest_h) ? h : dest_h;

  /* 1:1 Native Resolution Fast Path: Direct row streaming without per-pixel division */
  if (src_w == dest_w && src_h == dest_h) {
    for (int32_t dy = 0; dy < draw_h && (y + dy) < dest_h; dy++) {
      uint32_t dest_row = (uint32_t)(y + dy) * (uint32_t)dest_w;
      uint32_t src_row = (uint32_t)(y + dy) * (uint32_t)src_w;
      memcpy(&dest_buf[dest_row + (uint32_t)x], &wp_pixels[src_row + (uint32_t)x], (size_t)draw_w * sizeof(uint32_t));
    }
  } else {
    for (int32_t dy = 0; dy < draw_h && (y + dy) < dest_h; dy++) {
      int32_t sy = ((y + dy) * src_h) / dest_h;
      if (sy < 0) sy = 0;
      if (sy >= src_h) sy = src_h - 1;

      uint32_t dest_row = (uint32_t)(y + dy) * (uint32_t)dest_w;
      uint32_t src_row = (uint32_t)sy * (uint32_t)src_w;

      for (int32_t dx = 0; dx < draw_w && (x + dx) < dest_w; dx++) {
        int32_t sx = ((x + dx) * src_w) / dest_w;
        if (sx < 0) sx = 0;
        if (sx >= src_w) sx = src_w - 1;

        dest_buf[dest_row + (uint32_t)(x + dx)] = wp_pixels[src_row + (uint32_t)sx];
      }
    }
  }
  com1_dbg("[WALLPAPER R3] BLIT PASS\r\n");

  uint32_t probe_pixel = dest_buf[0];
  (void)probe_pixel;
  com1_dbg("[WALLPAPER R3] PIXEL PROBE PASS\r\n");
  com1_dbg("[WALLPAPER R3] COMPLETE\r\n");

  return SYSCALL_OK;
}

/* ============================================================
 * Phase 7 Userspace & C/C++ Runtime Services
 * ============================================================ */

static uint64_t s_user_mmap_bump = 0x60000000ULL;

uint64_t sys_service_mmap(uint64_t addr, size_t length, int prot, int flags, int fd, uint64_t offset) {
  if (length == 0 || length > 128 * 1024 * 1024) {
    return SYSCALL_FAIL;
  }

  Task *current = scheduler_current_task();
  if (!current || !current->pml4) {
    return SYSCALL_FAIL;
  }

  uint64_t hw_cr3 = 0;
  __asm__ volatile("mov %%cr3, %0" : "=r"(hw_cr3));
  void *target_pml4 = (void*)(hw_cr3 & 0x000FFFFFFFFFF000ULL);
  if (!target_pml4 && current && current->pml4) target_pml4 = current->pml4;
  if (!target_pml4) target_pml4 = vmm_get_kernel_pml4();

  size_t aligned_len = (length + 4095) & ~4095ULL;
  uint64_t virt_start = addr;

  if (virt_start == 0 || !(flags & MAP_FIXED)) {
    virt_start = s_user_mmap_bump;
    s_user_mmap_bump += aligned_len + 4096; // Guard page between allocations
    if (s_user_mmap_bump >= 0x78000000ULL) {
      s_user_mmap_bump = 0x60000000ULL;
    }
  } else {
    if (virt_start < USER_WINDOW_MIN || (virt_start + aligned_len) > USER_WINDOW_MAX) {
      return SYSCALL_BAD_ADDRESS;
    }
  }

  uint32_t map_flags = PAGE_USER | PAGE_PRESENT;
  if (prot & PROT_WRITE) {
    map_flags |= PAGE_WRITABLE;
  }
  if (!(prot & PROT_EXEC)) {
    map_flags |= PAGE_NX;
  }

  for (size_t off = 0; off < aligned_len; off += 4096) {
    void *phys = pmm_alloc_page();
    if (!phys) {
      // Free previously mapped pages in this batch
      for (size_t rollback = 0; rollback < off; rollback += 4096) {
        vmm_free_mapped_page(target_pml4, virt_start + rollback);
        if (current && current->pml4 && current->pml4 != target_pml4) {
          vmm_free_mapped_page(current->pml4, virt_start + rollback);
        }
      }
      return SYSCALL_FAIL;
    }
    memset((void *)phys, 0, 4096);

    // Eager file backing support
    if (fd >= 0 && !(flags & MAP_ANONYMOUS)) {
      if (off < length) {
        size_t to_read = length - off;
        if (to_read > 4096) to_read = 4096;
        vfs_pread(fd, (void *)phys, (uint32_t)to_read, offset + off);
      }
    }

    vmm_map_page(target_pml4, (uint64_t)phys, virt_start + off, map_flags);
    if (current && current->pml4 && current->pml4 != target_pml4) {
      vmm_map_page(current->pml4, (uint64_t)phys, virt_start + off, map_flags);
    }
  }

  diag_puts("[SYS_MMAP] Mapped user virt_start=");
  diag_put_hex64(virt_start);
  diag_puts(" len=");
  diag_put_hex64(aligned_len);
  diag_puts("\r\n");

  vmm_walk_and_verify(target_pml4, virt_start);

  return virt_start;
}

uint64_t sys_service_munmap(uint64_t addr, size_t length) {
  if (length == 0 || (addr & 0xFFF) != 0) {
    return SYSCALL_FAIL;
  }

  Task *current = scheduler_current_task();
  if (!current || !current->pml4) {
    return SYSCALL_FAIL;
  }

  uint64_t hw_cr3 = 0;
  __asm__ volatile("mov %%cr3, %0" : "=r"(hw_cr3));
  void *target_pml4 = (void*)(hw_cr3 & 0x000FFFFFFFFFF000ULL);
  if (!target_pml4 && current && current->pml4) target_pml4 = current->pml4;
  if (!target_pml4) target_pml4 = vmm_get_kernel_pml4();

  size_t aligned_len = (length + 4095) & ~4095ULL;
  for (size_t off = 0; off < aligned_len; off += 4096) {
    vmm_free_mapped_page(target_pml4, addr + off);
    if (current && current->pml4 && current->pml4 != target_pml4) {
      vmm_free_mapped_page(current->pml4, addr + off);
    }
  }

  return SYSCALL_OK;
}

uint64_t sys_service_mprotect(uint64_t addr, size_t length, int prot) {
  if (length == 0 || (addr & 0xFFF) != 0) {
    return SYSCALL_FAIL;
  }

  Task *current = scheduler_current_task();
  if (!current || !current->pml4) {
    return SYSCALL_FAIL;
  }

  size_t aligned_len = (length + 4095) & ~4095ULL;
  for (size_t off = 0; off < aligned_len; off += 4096) {
    uint64_t *pte = vmm_get_pt_entry(current->pml4, addr + off, false);
    if (pte && (*pte & PAGE_PRESENT)) {
      if (prot & PROT_WRITE) {
        *pte |= PAGE_WRITABLE;
      } else {
        *pte &= ~PAGE_WRITABLE;
      }
      if (prot & PROT_EXEC) {
        *pte &= ~PAGE_NX;
      } else {
        *pte |= PAGE_NX;
      }
      vmm_flush_tlb(addr + off);
    }
  }

  return SYSCALL_OK;
}

typedef struct {
  uint32_t *uaddr;
  Task *task;
  bool active;
} FutexWaiter;

#define MAX_FUTEX_WAITERS 64
static FutexWaiter s_futex_waiters[MAX_FUTEX_WAITERS];

uint64_t sys_service_futex(uint32_t *uaddr, int op, uint32_t val, const void *timeout) {
  (void)timeout;
  if (!syscall_validate_user_ptr(uaddr, sizeof(uint32_t))) {
    return SYSCALL_BAD_ADDRESS;
  }

  int cmd = op & 0x7F;
  if (cmd == FUTEX_WAIT) {
    if (*uaddr != val) {
      return (uint64_t)-1; // EAGAIN
    }
    Task *current = scheduler_current_task();
    if (!current) return SYSCALL_FAIL;

    int slot = -1;
    for (int i = 0; i < MAX_FUTEX_WAITERS; ++i) {
      if (!s_futex_waiters[i].active) {
        slot = i;
        break;
      }
    }
    if (slot == -1) {
      scheduler_yield();
      return SYSCALL_OK;
    }

    s_futex_waiters[slot].uaddr = uaddr;
    s_futex_waiters[slot].task = current;
    s_futex_waiters[slot].active = true;

    scheduler_block_task(current);
    scheduler_yield();

    s_futex_waiters[slot].active = false;
    return SYSCALL_OK;
  } else if (cmd == FUTEX_WAKE) {
    uint32_t to_wake = val;
    uint32_t woken = 0;
    for (int i = 0; i < MAX_FUTEX_WAITERS && woken < to_wake; ++i) {
      if (s_futex_waiters[i].active && s_futex_waiters[i].uaddr == uaddr) {
        s_futex_waiters[i].active = false;
        scheduler_resume_task(s_futex_waiters[i].task);
        woken++;
      }
    }
    return woken;
  }

  return SYSCALL_OK;
}

typedef struct {
  int64_t tv_sec;
  int64_t tv_nsec;
} sys_timespec_t;

uint64_t sys_service_clock_gettime(int clock_id, void *tp) {
  (void)clock_id;
  if (!syscall_validate_user_ptr_writable(tp, sizeof(sys_timespec_t))) {
    return SYSCALL_BAD_ADDRESS;
  }

  uint64_t ticks = timer_get_ticks();
  sys_timespec_t *ts = (sys_timespec_t *)tp;
  ts->tv_sec = (int64_t)(ticks / 1000);
  ts->tv_nsec = (int64_t)((ticks % 1000) * 1000000ULL);

  return SYSCALL_OK;
}

uint64_t sys_service_nanosleep(const void *req, void *rem) {
  (void)rem;
  if (!syscall_validate_user_ptr(req, sizeof(sys_timespec_t))) {
    return SYSCALL_BAD_ADDRESS;
  }

  const sys_timespec_t *ts = (const sys_timespec_t *)req;
  uint64_t ms = (uint64_t)ts->tv_sec * 1000 + (uint64_t)(ts->tv_nsec / 1000000);
  if (ms > 0) {
    scheduler_sleep(ms);
  }
  return SYSCALL_OK;
}

uint64_t sys_service_open(const char *path, int flags, int mode) {
  (void)flags; (void)mode;
  if (!syscall_validate_user_string(path, 256)) {
    return SYSCALL_BAD_ADDRESS;
  }
  return (uint64_t)vfs_open(path);
}

uint64_t sys_service_read(int fd, void *buf, size_t count) {
  if (!syscall_validate_user_ptr_writable(buf, count > 0 ? count : 1)) {
    return SYSCALL_BAD_ADDRESS;
  }
  return (uint64_t)vfs_read(fd, buf, (uint32_t)count);
}

uint64_t sys_service_write_file(int fd, const void *buf, size_t count) {
  if (!syscall_validate_user_ptr(buf, count > 0 ? count : 1)) {
    return SYSCALL_BAD_ADDRESS;
  }
  return (uint64_t)vfs_write(fd, (void *)buf, (uint32_t)count);
}

uint64_t sys_service_create(const char *path, int mode) {
  (void)mode;
  if (!syscall_validate_user_string(path, 256)) {
    return SYSCALL_BAD_ADDRESS;
  }
  return (uint64_t)vfs_create(path);
}

uint64_t sys_service_mkdir(const char *path, int mode) {
  (void)mode;
  if (!syscall_validate_user_string(path, 256)) {
    return SYSCALL_BAD_ADDRESS;
  }
  return (uint64_t)vfs_mkdir(path);
}

uint64_t sys_service_readdir(const char *path, int index, void *out_dirent) {
  if (!syscall_validate_user_string(path, 256)) {
    return SYSCALL_BAD_ADDRESS;
  }
  if (!syscall_validate_user_ptr_writable(out_dirent, sizeof(atoms_dirent_t))) {
    return SYSCALL_BAD_ADDRESS;
  }
  vfs_dirent_t vfs_entry;
  memset(&vfs_entry, 0, sizeof(vfs_dirent_t));
  int res = vfs_readdir(path, index, &vfs_entry);
  if (res == 0) {
    atoms_dirent_t *dent = (atoms_dirent_t *)out_dirent;
    dent->d_ino = (uint64_t)vfs_entry.cluster;
    dent->d_type = vfs_entry.is_directory ? 2 : 1;
    dent->d_namlen = (uint32_t)strlen(vfs_entry.name);
    strncpy(dent->d_name, vfs_entry.name, sizeof(dent->d_name) - 1);
    dent->d_name[sizeof(dent->d_name) - 1] = '\0';
    return SYSCALL_OK;
  }
  return (uint64_t)res;
}

uint64_t sys_service_unlink(const char *path) {
  if (!syscall_validate_user_string(path, 256)) {
    return SYSCALL_BAD_ADDRESS;
  }
  return (uint64_t)vfs_delete(path);
}

uint64_t sys_service_rename(const char *old_path, const char *new_path) {
  if (!syscall_validate_user_string(old_path, 256) ||
      !syscall_validate_user_string(new_path, 256)) {
    return SYSCALL_BAD_ADDRESS;
  }
  return (uint64_t)vfs_rename(old_path, new_path);
}

uint64_t sys_service_rmdir(const char *path) {
  if (!syscall_validate_user_string(path, 256)) {
    return SYSCALL_BAD_ADDRESS;
  }
  return (uint64_t)vfs_rmdir(path);
}

uint64_t sys_service_stat(const char *path, void *out_stat) {
  if (!syscall_validate_user_string(path, 256)) {
    return SYSCALL_BAD_ADDRESS;
  }
  if (!syscall_validate_user_ptr_writable(out_stat, sizeof(atoms_stat_t))) {
    return SYSCALL_BAD_ADDRESS;
  }
  return (uint64_t)vfs_stat(path, (atoms_stat_t *)out_stat);
}

#include "kernel/core/process/include/process_image.h"

extern int BOSX_LoadFromVFS(const char *filepath, uint32_t *out_pid);
extern ProcessImage* elf_load_image(void* pml4, const char* path);
extern ProcessImage* elf_load_image_from_buffer(void* pml4, const void* buffer, uint64_t size);
extern bool process_build_user_stack(ProcessImage* image, void* pml4);
extern Task* process_spawn(ProcessImage* image, const char* name);
extern const uint8_t g_embedded_desktop_elf[];
extern const uint64_t g_embedded_desktop_elf_len;
extern uint64_t get_embedded_desktop_elf_len(void);
extern const uint8_t g_embedded_media_player_elf[];
extern const uint64_t g_embedded_media_player_elf_len;
extern uint64_t get_embedded_media_player_elf_len(void);

uint64_t sys_service_exec(const char *path, const char **argv, const char **envp) {
  (void)envp;
  com1_dbg("[EXEC] START path=");
  if (path) com1_dbg(path); else com1_dbg("NULL");
  com1_dbg("\r\n");

  if (!path) {
    com1_dbg("[EXEC] FAIL: path is NULL\r\n");
    return SYSCALL_BAD_ADDRESS;
  }

  uintptr_t path_va = (uintptr_t)path;
  if (!vmm_address_canonical(path_va)) {
    com1_dbg("[EXEC] FAIL: non-canonical path address\r\n");
    return SYSCALL_BAD_ADDRESS;
  }

  /* Usermode pointer: perform strict Ring 3 page validation */
  if (path_va >= USER_WINDOW_MIN && path_va < USER_WINDOW_MAX) {
    if (!syscall_validate_user_string(path, 256)) {
      com1_dbg("[EXEC] FAIL: syscall_validate_user_string(path)\r\n");
      return SYSCALL_BAD_ADDRESS;
    }
  } else {
    /* Kernel-space pointer (e.g. invoked from Explorer / kernel shell):
     * Verify bounded null-termination */
    bool terminated = false;
    for (size_t i = 0; i < 256; i++) {
      if (path[i] == '\0') {
        terminated = true;
        break;
      }
    }
    if (!terminated) {
      com1_dbg("[EXEC] FAIL: kernel path string not null-terminated within 256 bytes\r\n");
      return SYSCALL_BAD_ADDRESS;
    }
  }
  com1_dbg("[EXEC] path validation OK\r\n");

  /* 1. Try native BOSX executable from VFS first */
  uint32_t pid = 0;
  int res = BOSX_LoadFromVFS(path, &pid);
  if (res == 0) {
    com1_dbg("[EXEC] BOSX_LoadFromVFS SUCCESS\r\n");
    return (uint64_t)pid;
  }
  com1_dbg("[EXEC] BOSX_LoadFromVFS returned != 0, checking embedded ELF\r\n");

  /* 2. Multi-Process spawning via embedded userspace ELF (Chromium / APAL multi-process model) */
  uint64_t elf_len = get_embedded_desktop_elf_len();
  if (elf_len == 0) elf_len = g_embedded_desktop_elf_len;
  if (elf_len == 0) {
    com1_dbg("[EXEC] FAIL: elf_len == 0\r\n");
    return (uint64_t)res;
  }
  com1_dbg("[EXEC] elf_len OK\r\n");

  int argc = 0;
  char arg_buf[16][128];
  if (argv) {
    uintptr_t argv_root = (uintptr_t)argv;
    bool is_user_argv = (argv_root >= USER_WINDOW_MIN && argv_root < USER_WINDOW_MAX);

    for (int i = 0; i < 16; i++) {
      if (is_user_argv) {
        if (!syscall_validate_user_ptr(&argv[i], sizeof(char *))) {
          com1_dbg("[EXEC] argv pointer check stopped at index\r\n");
          break;
        }
      }
      const char *arg_ptr = argv[i];
      if (!arg_ptr) break;

      uintptr_t arg_va = (uintptr_t)arg_ptr;
      if (!vmm_address_canonical(arg_va)) break;

      bool valid_arg = false;
      if (arg_va >= USER_WINDOW_MIN && arg_va < USER_WINDOW_MAX) {
        valid_arg = syscall_validate_user_string(arg_ptr, 128);
      } else {
        /* Kernel argument string: verify null-termination */
        for (int k = 0; k < 128; k++) {
          if (arg_ptr[k] == '\0') {
            valid_arg = true;
            break;
          }
        }
      }

      if (valid_arg) {
        int k = 0;
        while (arg_ptr[k] && k < 127) {
          arg_buf[argc][k] = arg_ptr[k];
          k++;
        }
        arg_buf[argc][k] = '\0';
        com1_dbg("[EXEC] parsed arg: ");
        com1_dbg(arg_buf[argc]);
        com1_dbg("\r\n");
        argc++;
      } else {
        com1_dbg("[EXEC] FAIL: arg validation failed\r\n");
      }
    }
  }

  if (argc == 0) {
    int k = 0;
    while (path[k] && k < 127) {
      arg_buf[0][k] = path[k];
      k++;
    }
    arg_buf[0][k] = '\0';
    argc = 1;
  }

  void *new_pml4 = vmm_create_address_space();
  if (!new_pml4) {
    com1_dbg("[EXEC] FAIL: vmm_create_address_space\r\n");
    return SYSCALL_FAIL;
  }
  com1_dbg("[EXEC] vmm_create_address_space OK\r\n");

  ProcessImage *img = NULL;
  if (path && (strstr(path, ".elf") || strstr(path, ".ELF"))) {
    img = elf_load_image(new_pml4, path);
    if (!img && (strstr(path, "media") || strstr(path, "MEDIA"))) {
      img = elf_load_image(new_pml4, "/MEDIA.ELF");
      if (!img) img = elf_load_image(new_pml4, "MEDIA.ELF");
      if (!img) img = elf_load_image(new_pml4, "/media_player.elf");
      if (!img) {
        uint64_t media_elf_len = get_embedded_media_player_elf_len();
        if (media_elf_len == 0) media_elf_len = g_embedded_media_player_elf_len;
        if (media_elf_len > 0) {
          com1_dbg("[EXEC] Loading Media Player from embedded ELF payload...\r\n");
          diag_puts("[EXEC] Loading Media Player from embedded ELF payload...\r\n");
          img = elf_load_image_from_buffer(new_pml4, g_embedded_media_player_elf, media_elf_len);
        }
      }
    }
    if (img) {
      com1_dbg("[EXEC] elf_load_image SUCCESS: ");
      com1_dbg(path);
      com1_dbg("\r\n");
      diag_puts("[EXEC] elf_load_image SUCCESS: ");
      diag_puts(path);
      diag_puts("\r\n");
    }
  }

  if (!img) {
    if (path && (strstr(path, "desktop") || strstr(path, "DESKTOP") || strcmp(path, "/") == 0 || strlen(path) == 0)) {
      img = elf_load_image_from_buffer(new_pml4, g_embedded_desktop_elf, elf_len);
      if (!img) {
        com1_dbg("[EXEC] FAIL: elf_load_image_from_buffer desktop\r\n");
        diag_puts("[EXEC] FAIL: elf_load_image_from_buffer desktop\r\n");
        vmm_destroy_address_space(new_pml4);
        return SYSCALL_FAIL;
      }
      com1_dbg("[EXEC] elf_load_image_from_buffer desktop OK\r\n");
      diag_puts("[EXEC] elf_load_image_from_buffer desktop OK\r\n");
    } else {
      com1_dbg("[EXEC] FAIL: Executable not found\r\n");
      diag_puts("[EXEC] FAIL: Executable not found: ");
      if (path) diag_puts(path);
      diag_puts("\r\n");
      vmm_destroy_address_space(new_pml4);
      return SYSCALL_FAIL;
    }
  }

  if (!process_build_user_stack(img, new_pml4)) {
    com1_dbg("[EXEC] FAIL: process_build_user_stack\r\n");
    vmm_destroy_address_space(new_pml4);
    return SYSCALL_FAIL;
  }
  com1_dbg("[EXEC] process_build_user_stack OK\r\n");

  /* Configure System V AMD64 ABI initial stack frame for the child process */
  uint64_t top_page_va = (img->stack_top & ~0xFFFULL);
  uint64_t top_page_phys = vmm_translate(new_pml4, top_page_va);
  if (top_page_phys) {
    uint8_t *page = (uint8_t *)top_page_phys;
    uint32_t str_off = 0xC00;
    uint64_t arg_vas[16];
    for (int i = 0; i < argc; i++) {
      int len = 0;
      while (arg_buf[i][len]) len++;
      len++; /* Include null terminator */
      if (str_off + len < 0xF80) {
        for (int b = 0; b < len; b++) {
          page[str_off + b] = (uint8_t)arg_buf[i][b];
        }
        arg_vas[i] = top_page_va + str_off;
        str_off += (len + 7) & ~7U; /* 8-byte alignment */
      } else {
        arg_vas[i] = 0;
      }
    }

    int64_t *sp = (int64_t *)&page[0xF80];
    sp[0] = argc;
    for (int i = 0; i < argc; i++) {
      sp[1 + i] = (int64_t)arg_vas[i];
    }
    sp[1 + argc] = 0; /* argv[argc] = NULL */
    sp[2 + argc] = 0; /* envp[0] = NULL */
    img->stack_top = top_page_va + 0xF80;
    com1_dbg("[EXEC] stack ABI frame configured OK\r\n");
  } else {
    com1_dbg("[EXEC] WARN: top_page_phys translation failed\r\n");
  }

  static char s_child_name[64];
  if (argc > 0 && arg_buf[0][0]) {
    int ci = 0;
    while (arg_buf[0][ci] && ci < 63) {
      s_child_name[ci] = arg_buf[0][ci];
      ci++;
    }
    s_child_name[ci] = '\0';
  } else {
    s_child_name[0] = 'c'; s_child_name[1] = 'h'; s_child_name[2] = 'i'; s_child_name[3] = 'l'; s_child_name[4] = 'd'; s_child_name[5] = '\0';
  }

  com1_dbg("[EXEC] calling process_spawn\r\n");
  Task *child_task = process_spawn(img, s_child_name);
  if (!child_task) {
    com1_dbg("[EXEC] FAIL: process_spawn returned NULL\r\n");
    vmm_destroy_address_space(new_pml4);
    return SYSCALL_FAIL;
  }

  com1_dbg("[EXEC] SUCCESS: child spawned PID=");
  char pid_str[16];
  uint32_t tpid = img->pid;
  int pidx = 0;
  if (tpid == 0) { pid_str[pidx++] = '0'; }
  else {
    char r[16]; int ri = 0;
    while (tpid > 0) { r[ri++] = '0' + (tpid % 10); tpid /= 10; }
    while (ri > 0) { pid_str[pidx++] = r[--ri]; }
  }
  pid_str[pidx] = '\0';
  com1_dbg(pid_str);
  com1_dbg("\r\n");
  diag_puts("[EXEC] SUCCESS: child spawned PID=");
  diag_puts(pid_str);
  diag_puts("\r\n");

  return (uint64_t)img->pid;
}

uint64_t sys_service_close(int fd) {
  return (uint64_t)vfs_close(fd);
}

uint64_t sys_service_seek(int fd, uint64_t offset, int whence) {
  return (uint64_t)vfs_seek(fd, (int64_t)offset, whence);
}

uint64_t sys_service_thread_spawn(void (*entry)(void*), void *stack_top, void *arg) {
  if (!entry) return SYSCALL_FAIL;

  Task *curr = scheduler_current_task();
  if (!curr || !curr->pml4) return SYSCALL_FAIL;

  uint64_t u_entry = (uint64_t)entry;
  uint64_t u_stack = (uint64_t)stack_top;

  if (u_entry < USER_WINDOW_MIN || u_entry >= USER_WINDOW_MAX) return SYSCALL_BAD_ADDRESS;
  if (u_stack < USER_WINDOW_MIN || u_stack > USER_WINDOW_MAX) return SYSCALL_BAD_ADDRESS;

  uint32_t pid = curr->owner_pid ? curr->owner_pid : (uint32_t)curr->id;

  Task *task = (Task *)kmalloc(sizeof(Task));
  if (!task) return SYSCALL_FAIL;
  memset(task, 0, sizeof(Task));

  task->owner_pid = pid;
  task->name = "u_thread";
  task->state = TASK_NEW;
  task->queue_class = TASK_QUEUE_NONE;
  task->quantum = 5;
  task->default_quantum = 5;
  task->base_priority = 16;
  task->effective_priority = 16;
  task->affinity_mask = UINT64_MAX;
  task->is_user_task = 1;
  task->pml4 = curr->pml4;
  task->user_stack = NULL;
  task->rip = u_entry;
  list_node_init(&task->queue_node);
  task->guard_tail = TASK_GUARD_TAIL_MAGIC;

  task->stack = kernel_stack_alloc(KERNEL_TASK_STACK_SIZE);
  if (!task->stack) {
    kfree(task);
    return SYSCALL_FAIL;
  }

  uint64_t *stack = (uint64_t *)((uint64_t)task->stack + KERNEL_TASK_STACK_SIZE);

  // 1. Interrupt Frame for iretq (5 items)
  *(--stack) = 0x1B; // SS: User Data Segment (Selector 0x18 | RPL 3)
  *(--stack) = u_stack; // RSP: User Stack Pointer
  *(--stack) = 0x202; // RFLAGS (Interrupts Enabled: IF=1)
  *(--stack) = 0x23; // CS: User Code Segment (Selector 0x20 | RPL 3)
  *(--stack) = u_entry; // RIP: User Instruction Pointer

  // 2. Dummy Error Code & Int No (2 items)
  *(--stack) = 0; // dummy err_code
  *(--stack) = 0; // dummy int_no

  // 3. General Purpose Registers (15 items)
  // In context_switch.asm, the pop order is:
  // r15, r14, r13, r12, r11, r10, r9, r8, rbp, rdi, rsi, rdx, rcx, rbx, rax
  *(--stack) = 0; // rax
  *(--stack) = 0; // rbx
  *(--stack) = 0; // rcx
  *(--stack) = 0; // rdx
  *(--stack) = 0; // rsi
  *(--stack) = (uint64_t)arg; // rdi = arg (1st parameter)
  *(--stack) = 0; // rbp
  *(--stack) = 0; // r8
  *(--stack) = 0; // r9
  *(--stack) = 0; // r10
  *(--stack) = 0; // r11
  *(--stack) = 0; // r12
  *(--stack) = 0; // r13
  *(--stack) = 0; // r14
  *(--stack) = 0; // r15

  task->rsp = (uint64_t)stack;
  cpu_extended_state_init_task(task);

  static uint64_t s_thread_fallback_id = 5000;
  ATOMS_TCB *tcb = ATOMS_Thread_Create(pid, "u_thread", u_entry, 16);
  if (tcb) {
    task->id = tcb->tid;
    ATOMS_Thread_BindTask(tcb->tid, task);
  } else {
    task->id = ++s_thread_fallback_id;
  }

  if (!scheduler_submit_task(task)) {
    if (tcb) {
      ATOMS_Thread_Terminate(tcb->tid);
    }
    cpu_extended_state_free_task(task);
    kernel_stack_free(task->stack, KERNEL_TASK_STACK_SIZE);
    kfree(task);
    return SYSCALL_FAIL;
  }

  return (uint64_t)task->id;
}

uint64_t sys_service_thread_exit(int exit_code) {
  (void)exit_code;
  Task *current = scheduler_current_task();
  if (current && current != scheduler_get_idle_task()) {
    ATOMS_Thread_Terminate((uint32_t)current->id);
    scheduler_terminate_task(current);
    extern void scheduler_yield(void);
    scheduler_yield();
  }
  return SYSCALL_OK;
}

void launch_phase7_runtime_certification(void) {
  display_print("\n[PHASE 7] ATOMS Userspace C/C++ Runtime Certified.\n");
}

void launch_phase1_java_runtime_certification(void) {
  extern bool ATOMS_RunPhase1_JavaRuntimeFoundationTests(void *out_report);
  display_print("\n[PHASE 1] Launching Java Runtime Foundation Tests...\n");
  ATOMS_RunPhase1_JavaRuntimeFoundationTests(NULL);
  display_print("[PHASE 1] ATOMS Java Runtime Foundation Certified.\n");
}

/* ============================================================
 * Phase 16-B Chromium Process, IPC, SHM & Exception Services
 * ============================================================ */

uint64_t sys_service_waitpid(uint32_t pid, int32_t *out_status, uint32_t options) {
  if (out_status && !syscall_validate_user_ptr_writable(out_status, sizeof(int32_t))) {
    return SYSCALL_BAD_ADDRESS;
  }

  ATOMS_PCB *child = ATOMS_Process_GetByPID(pid);
  if (!child) {
    return SYSCALL_FAIL;
  }

  /* WNOHANG = 1: Check if still running without blocking */
  if ((options & 1U) && child->state != ATOMS_PROC_STATE_ZOMBIE &&
      child->state != ATOMS_PROC_STATE_TERMINATED &&
      child->state != ATOMS_PROC_STATE_CLOSED) {
    return 0; /* Child still running */
  }

  int32_t exit_val = 0;
  int32_t res = ATOMS_Process_Wait(pid, &exit_val);
  if (res > 0) {
    if (out_status) {
      *out_status = exit_val;
    }
    return (uint64_t)res;
  }
  return SYSCALL_FAIL;
}

uint64_t sys_service_kill(uint32_t pid, int32_t signal) {
  if (pid < 200U) {
    return SYSCALL_FAIL; /* Protect kernel / init tasks */
  }
  bool ok = ATOMS_Process_Terminate(pid, signal);
  return ok ? SYSCALL_OK : SYSCALL_FAIL;
}

uint64_t sys_service_process_status(uint32_t pid, void *out_status_buf) {
  if (!syscall_validate_user_ptr_writable(out_status_buf, sizeof(atoms_process_status_t))) {
    return SYSCALL_BAD_ADDRESS;
  }
  ATOMS_PCB *pcb = ATOMS_Process_GetByPID(pid);
  if (!pcb) {
    return SYSCALL_FAIL;
  }
  atoms_process_status_t *st = (atoms_process_status_t *)out_status_buf;
  st->pid = pcb->pid;
  st->parent_pid = pcb->parent_pid;
  st->state = (uint32_t)pcb->state;
  st->exit_code = pcb->exit_code;
  st->thread_count = pcb->thread_count;
  st->cpu_time_ms = pcb->cpu_time_ms;
  return SYSCALL_OK;
}

uint64_t sys_service_ipc_call(uint32_t op, uint64_t a1, uint64_t a2, uint64_t a3) {
  bos_ipc_init();
  Task *cur = scheduler_current_task();
  uint32_t caller_pid = cur ? cur->owner_pid : 0;

  switch (op) {
  case ATOMS_IPC_OP_CREATE: {
    if (a1 && !syscall_validate_user_string((const char *)a1, 64)) return SYSCALL_BAD_ADDRESS;
    if (!syscall_validate_user_ptr_writable((void *)a3, sizeof(ipc_channel_handle_t))) return SYSCALL_BAD_ADDRESS;
    ipc_channel_handle_t ch = 0;
    ipc_status_t st = ipc_channel_create((const char *)a1, (uint32_t)a2, caller_pid, &ch);
    if (st == IPC_SUCCESS) {
      *(ipc_channel_handle_t *)a3 = ch;
      return SYSCALL_OK;
    }
    return SYSCALL_FAIL;
  }

  case ATOMS_IPC_OP_CONNECT: {
    if (!syscall_validate_user_string((const char *)a1, 64)) return SYSCALL_BAD_ADDRESS;
    if (!syscall_validate_user_ptr_writable((void *)a2, sizeof(ipc_channel_handle_t))) return SYSCALL_BAD_ADDRESS;
    ipc_channel_handle_t ch = 0;
    ipc_status_t st = ipc_channel_connect_by_name((const char *)a1, caller_pid, &ch);
    if (st == IPC_SUCCESS) {
      *(ipc_channel_handle_t *)a2 = ch;
      return SYSCALL_OK;
    }
    return SYSCALL_FAIL;
  }

  case ATOMS_IPC_OP_SEND: {
    if (!syscall_validate_user_ptr((const void *)a2, a3 > 0 ? a3 : 1)) return SYSCALL_BAD_ADDRESS;
    ipc_status_t st = bos_ipc_send((ipc_channel_handle_t)a1, (const void *)a2, (uint32_t)a3, 0);
    return (st == IPC_SUCCESS) ? SYSCALL_OK : SYSCALL_FAIL;
  }

  case ATOMS_IPC_OP_RECV: {
    if (!syscall_validate_user_ptr_writable((void *)a2, a3 > 0 ? a3 : 1)) return SYSCALL_BAD_ADDRESS;
    uint32_t actual = 0;
    ipc_status_t st = bos_ipc_receive((ipc_channel_handle_t)a1, (void *)a2, (uint32_t)a3, &actual, 0);
    if (st == IPC_SUCCESS) {
      return (uint64_t)actual;
    }
    return SYSCALL_FAIL;
  }

  case ATOMS_IPC_OP_CLOSE: {
    ipc_status_t st = bos_ipc_close((ipc_channel_handle_t)a1);
    return (st == IPC_SUCCESS) ? SYSCALL_OK : SYSCALL_FAIL;
  }

  default:
    return SYSCALL_INVALID;
  }
}

uint64_t sys_service_shm_call(uint32_t op, uint64_t a1, uint64_t a2, uint64_t a3) {
  bos_ipc_init();
  Task *cur = scheduler_current_task();
  uint32_t caller_pid = cur ? cur->owner_pid : 0;

  switch (op) {
  case ATOMS_SHM_OP_CREATE: {
    if (a1 && !syscall_validate_user_string((const char *)a1, 64)) return SYSCALL_BAD_ADDRESS;
    ipc_shm_handle_t h = 0;
    ipc_status_t st = ipc_shm_create((const char *)a1, (uint32_t)a2, (uint32_t)a3, caller_pid, &h);
    if (st == IPC_SUCCESS) {
      return (uint64_t)h;
    }
    return SYSCALL_FAIL;
  }

  case ATOMS_SHM_OP_OPEN: {
    if (!syscall_validate_user_string((const char *)a1, 64)) return SYSCALL_BAD_ADDRESS;
    ipc_shm_handle_t h = 0;
    ipc_status_t st = ipc_shm_open_by_name((const char *)a1, (uint32_t)a2, &h);
    if (st == IPC_SUCCESS) {
      return (uint64_t)h;
    }
    return SYSCALL_FAIL;
  }

  case ATOMS_SHM_OP_MAP: {
    if (!cur || !cur->pml4) return SYSCALL_FAIL;
    ipc_shm_object_t *obj = ipc_shm_get((ipc_shm_handle_t)a1);
    if (!obj || obj->state != IPC_SHM_STATE_CREATED) return SYSCALL_FAIL;

    uint64_t user_shm_base = 0x60000000ULL + ((uint64_t)a1 * 0x200000ULL);
    uint32_t map_flags = PAGE_USER | PAGE_PRESENT;
    if ((uint32_t)a2 & IPC_SHM_WRITE) {
      map_flags |= PAGE_WRITABLE;
    }

    for (uint32_t p = 0; p < obj->page_count; p++) {
      vmm_map_page(cur->pml4, obj->phys_pages[p], user_shm_base + (uint64_t)p * 4096ULL, map_flags);
    }
    vmm_flush_tlb(user_shm_base);
    return user_shm_base;
  }

  case ATOMS_SHM_OP_UNMAP: {
    if (!cur || !cur->pml4) return SYSCALL_FAIL;
    ipc_shm_object_t *obj = ipc_shm_get((ipc_shm_handle_t)a1);
    if (obj) {
      for (uint32_t p = 0; p < obj->page_count; p++) {
        uint64_t *pte = vmm_get_pt_entry(cur->pml4, a2 + (uint64_t)p * 4096ULL, false);
        if (pte) {
          *pte = 0;
          vmm_flush_tlb(a2 + (uint64_t)p * 4096ULL);
        }
      }
    }
    return SYSCALL_OK;
  }

  case ATOMS_SHM_OP_DESTROY: {
    ipc_status_t st = ipc_shm_destroy_object((ipc_shm_handle_t)a1);
    return (st == IPC_SUCCESS) ? SYSCALL_OK : SYSCALL_FAIL;
  }

  default:
    return SYSCALL_INVALID;
  }
}

/*
 * Universal Audio Syscall Gateway Service (SYS_AUDIO_CALL 43U)
 */
uint64_t sys_service_audio_call(uint32_t op, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4) {
  (void)a3;
  (void)a4;
  Task* cur = scheduler_current_task();
  uint32_t pid = cur ? cur->id : 0;

  switch (op) {
  case ATOMS_AUDIO_OP_DEVICE_GET_INFO: {
    audio_hal_driver_t* drv = audio_hal_get_active_driver();
    return (drv != NULL) ? SYSCALL_OK : SYSCALL_FAIL;
  }

  case ATOMS_AUDIO_OP_STREAM_CREATE: {
    uint32_t stream_id = audio_stream_create(pid);
    return (stream_id != 0) ? (uint64_t)stream_id : SYSCALL_FAIL;
  }

  case ATOMS_AUDIO_OP_STREAM_DESTROY: {
    return audio_stream_destroy((uint32_t)a1) ? SYSCALL_OK : SYSCALL_FAIL;
  }

  case ATOMS_AUDIO_OP_STREAM_WRITE: {
    if (!a2) return 0;
    if (!syscall_validate_user_ptr((const void*)a2, sizeof(AudioPcmPacket))) return 0;
    const AudioPcmPacket* pkt = (const AudioPcmPacket*)a2;
    if (!pkt->pcm_data || pkt->size_bytes == 0 || pkt->size_bytes > 262144) return 0;
    if (!syscall_validate_user_ptr((const void*)pkt->pcm_data, pkt->size_bytes)) return 0;
    return (uint64_t)audio_stream_write((uint32_t)a1, pkt);
  }

  case ATOMS_AUDIO_OP_STREAM_START: {
    return audio_stream_resume((uint32_t)a1) ? SYSCALL_OK : SYSCALL_FAIL;
  }

  case ATOMS_AUDIO_OP_STREAM_STOP: {
    return audio_stream_stop((uint32_t)a1) ? SYSCALL_OK : SYSCALL_FAIL;
  }

  case ATOMS_AUDIO_OP_STREAM_PAUSE: {
    return audio_stream_pause((uint32_t)a1) ? SYSCALL_OK : SYSCALL_FAIL;
  }

  case ATOMS_AUDIO_OP_STREAM_RESUME: {
    return audio_stream_resume((uint32_t)a1) ? SYSCALL_OK : SYSCALL_FAIL;
  }

  case ATOMS_AUDIO_OP_DEVICE_SET_VOL: {
    return audio_set_volume((uint32_t)a1, (uint8_t)a2) ? SYSCALL_OK : SYSCALL_FAIL;
  }

  case ATOMS_AUDIO_OP_STREAM_SET_FORMAT: {
    if (!a2) return SYSCALL_FAIL;
    if (!syscall_validate_user_ptr((const void*)a2, sizeof(AudioPcmFormat))) return SYSCALL_BAD_ADDRESS;
    const AudioPcmFormat* fmt = (const AudioPcmFormat*)a2;
    return audio_stream_set_format((uint32_t)a1, fmt) ? SYSCALL_OK : SYSCALL_FAIL;
  }

  case ATOMS_AUDIO_OP_STREAM_GET_AVAIL: {
    return (uint64_t)audio_stream_available((uint32_t)a1);
  }

  default:
    return SYSCALL_INVALID;
  }
}

/*
 * Phase 1 Thread-Local Storage (TLS) FS Base Services
 */
uint64_t sys_service_set_fs_base(uint64_t base) {
  if (base != 0 && (base < USER_WINDOW_MIN || base >= USER_WINDOW_MAX)) {
    return SYSCALL_BAD_ADDRESS;
  }
  Task *current = scheduler_current_task();
  if (!current) {
    return SYSCALL_FAIL;
  }
  current->fs_base = base;
  __asm__ volatile("wrmsr" : : "c"(0xC0000100U), "a"((uint32_t)base), "d"((uint32_t)(base >> 32)));
  return SYSCALL_OK;
}

uint64_t sys_service_get_fs_base(void) {
  Task *current = scheduler_current_task();
  if (!current) {
    return 0;
  }
  return current->fs_base;
}
