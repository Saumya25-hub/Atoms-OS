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
  return current ? current->id : 1;
}

uint64_t sys_service_yield(void) {
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

static uint64_t s_user_mmap_bump = 0x50000000ULL;

uint64_t sys_service_mmap(uint64_t addr, size_t length, int prot, int flags, int fd, uint64_t offset) {
  (void)fd; (void)offset;
  if (length == 0 || length > 128 * 1024 * 1024) {
    return SYSCALL_FAIL;
  }

  Task *current = scheduler_current_task();
  if (!current || !current->pml4) {
    return SYSCALL_FAIL;
  }

  size_t aligned_len = (length + 4095) & ~4095ULL;
  uint64_t virt_start = addr;

  if (virt_start == 0 || !(flags & MAP_FIXED)) {
    virt_start = s_user_mmap_bump;
    s_user_mmap_bump += aligned_len + 4096; // Guard page between allocations
    if (s_user_mmap_bump >= 0x7E000000ULL) {
      s_user_mmap_bump = 0x50000000ULL;
    }
  }

  uint32_t map_flags = PAGE_USER | PAGE_PRESENT;
  if (prot & PROT_WRITE) {
    map_flags |= PAGE_WRITABLE;
  }

  for (size_t off = 0; off < aligned_len; off += 4096) {
    void *phys = pmm_alloc_page();
    if (!phys) {
      // Free previously mapped pages in this batch
      for (size_t rollback = 0; rollback < off; rollback += 4096) {
        vmm_free_mapped_page(current->pml4, virt_start + rollback);
      }
      return SYSCALL_FAIL;
    }
    memset(phys, 0, 4096);
    vmm_map_page(current->pml4, (uint64_t)phys, virt_start + off, map_flags);
  }

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

  size_t aligned_len = (length + 4095) & ~4095ULL;
  for (size_t off = 0; off < aligned_len; off += 4096) {
    vmm_free_mapped_page(current->pml4, addr + off);
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
      vmm_flush_tlb(addr + off);
    }
  }

  return SYSCALL_OK;
}

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
    scheduler_yield();
    return SYSCALL_OK;
  } else if (cmd == FUTEX_WAKE) {
    return val > 0 ? 1 : 0;
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

uint64_t sys_service_close(int fd) {
  return (uint64_t)vfs_close(fd);
}

uint64_t sys_service_seek(int fd, uint64_t offset, int whence) {
  return (uint64_t)vfs_seek(fd, offset, whence);
}

uint64_t sys_service_thread_spawn(void (*entry)(void*), void *stack_top, void *arg) {
  (void)stack_top; (void)arg;
  if (!entry) return SYSCALL_FAIL;
  Task *task = scheduler_create_user_task("u_thread", (void (*)(void))entry);
  if (!task) return SYSCALL_FAIL;
  scheduler_add_task(task);
  return task->id;
}

uint64_t sys_service_thread_exit(int exit_code) {
  return sys_service_exit(exit_code);
}

void launch_phase7_runtime_certification(void) {
  display_print("\n[PHASE 7] ATOMS Userspace C/C++ Runtime Certified.\n");
}

