#include "kernel/core/syscall/include/syscall.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/memory/vmm/include/paging.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/scheduler/include/scheduler.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/timer/include/timer.h"
#include "kernel/core/lib/include/string.h"

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
  if (!syscall_validate_user_ptr(msg, 1)) {
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
  uint32_t head;
  uint32_t tail;
} WinEventQueue;

static WinEventQueue s_win_event_queues[BWE_MAX_WINDOWS];

void sys_gui_post_event(uint32_t win_id, const BOS_GUIEvent *ev) {
  if (win_id >= BWE_MAX_WINDOWS || !ev) return;
  WinEventQueue *q = &s_win_event_queues[win_id];
  uint32_t next = (q->head + 1) % MAX_GUI_EVENTS_PER_WIN;
  if (next != q->tail) {
    q->events[q->head] = *ev;
    q->head = next;
  }
}

uint64_t sys_service_gui_create_window(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t flags, const char *title) {
  (void)flags;
  if (!syscall_validate_user_string(title, 128)) {
    return 0;
  }
  if (w <= 0) w = 320;
  if (h <= 0) h = 240;
  if (w > 1920) w = 1920;
  if (h > 1080) h = 1080;

  char safe_title[128];
  strncpy(safe_title, title, 127);
  safe_title[127] = '\0';

  uint32_t win_id = 0;
  bwe_error_t err = BOS_CreateWindow(x, y, w, h, safe_title, &win_id);
  if (err != BWE_SUCCESS || win_id == 0) {
    return 0;
  }

  BWE_Window *win = BWE_GetWindow(win_id);
  if (win) {
    Task *cur = scheduler_current_task();
    win->owner_pid = cur ? cur->id : 1;
    win->flags = flags;
    if (flags & 1) win->flags |= BWE_WINDOW_BORDERLESS;
  }

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
  if (!BWE_ValidateWindow(win_id)) return SYSCALL_FAIL;
  BWE_Window *win = BWE_GetWindow(win_id);
  Task *cur = scheduler_current_task();
  if (win && cur && win->owner_pid != cur->id && cur->id != 0) {
    return SYSCALL_FAIL;
  }
  if (visible) {
    BOS_Show(win_id);
  } else {
    BOS_Hide(win_id);
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
  if (w <= 0 || h <= 0 || w > 1920 || h > 1080) return SYSCALL_FAIL;
  BOS_SetBounds(win_id, (uint32_t)x, (uint32_t)y, (uint32_t)w, (uint32_t)h);
  return SYSCALL_OK;
}

uint64_t sys_service_gui_map_surface(uint32_t win_id, uint64_t *out_user_surface_ptr, uint32_t *out_stride_bytes) {
  if (!syscall_validate_user_ptr(out_user_surface_ptr, sizeof(uint64_t))) return SYSCALL_BAD_ADDRESS;
  if (!syscall_validate_user_ptr(out_stride_bytes, sizeof(uint32_t))) return SYSCALL_BAD_ADDRESS;

  if (!BWE_ValidateWindow(win_id)) return SYSCALL_FAIL;
  BWE_Window *win = BWE_GetWindow(win_id);
  Task *cur = scheduler_current_task();
  if (win && cur && win->owner_pid != cur->id && cur->id != 0) {
    return SYSCALL_FAIL;
  }

  if (!win->control_data.canvas.pixel_buffer) {
    uint32_t w = win->screen_bounds.width > 0 ? (uint32_t)win->screen_bounds.width : 320;
    uint32_t h = win->screen_bounds.height > 0 ? (uint32_t)win->screen_bounds.height : 240;
    win->control_data.canvas.buffer_w = w;
    win->control_data.canvas.buffer_h = h;

    uint64_t total_bytes = (uint64_t)w * h * sizeof(uint32_t);
    uint64_t pages_needed = (total_bytes + 4095) / 4096;
    uint64_t user_virt_base = 0x50000000ULL + ((uint64_t)win_id * 0x1000000ULL);

    void* kbuf = kmalloc(total_bytes);
    if (!kbuf) return SYSCALL_FAIL;
    memset(kbuf, 0, total_bytes);

    void* kpml4 = vmm_get_kernel_pml4();
    if (cur && cur->pml4) {
      for (uint64_t p = 0; p < pages_needed; p++) {
        uint64_t vaddr = user_virt_base + p * 4096;
        uint64_t kvaddr = (uint64_t)kbuf + p * 4096;
        uint64_t phys = vmm_translate(kpml4, kvaddr);
        if (phys == 0) phys = kvaddr;
        vmm_map_page(cur->pml4, phys, vaddr, PAGE_USER | PAGE_WRITABLE | PAGE_PRESENT);
      }
      win->control_data.canvas.pixel_buffer = (uint32_t*)kbuf;
    }
  }

  uint64_t user_virt_base = 0x50000000ULL + ((uint64_t)win_id * 0x1000000ULL);
  *out_user_surface_ptr = user_virt_base;
  *out_stride_bytes = win->control_data.canvas.buffer_w * 4;
  return SYSCALL_OK;
}

uint64_t sys_service_gui_invalidate(uint32_t win_id, int32_t x, int32_t y, int32_t w, int32_t h) {
  (void)x; (void)y; (void)w; (void)h;
  if (!BWE_ValidateWindow(win_id)) return SYSCALL_FAIL;
  BWE_InvalidateWindow(win_id);
  extern void BWE_Compose(void);
  BWE_Compose();
  return SYSCALL_OK;
}

uint64_t sys_service_gui_poll_event(uint32_t win_id, BOS_GUIEvent *out_user_event, uint32_t event_struct_size) {
  if (!syscall_validate_user_ptr(out_user_event, sizeof(BOS_GUIEvent))) return SYSCALL_BAD_ADDRESS;
  if (event_struct_size != sizeof(BOS_GUIEvent)) return SYSCALL_FAIL;

  if (win_id >= BWE_MAX_WINDOWS) return 0;

  extern void xhci_poll(void);
  xhci_poll();
  extern void input_core_dispatch_events(void);
  input_core_dispatch_events();
  extern void BWE_PumpEvents(void);
  BWE_PumpEvents();

  WinEventQueue *q = &s_win_event_queues[win_id];
  if (q->head == q->tail) {
    return 0;
  }

  *out_user_event = q->events[q->tail];
  q->tail = (q->tail + 1) % MAX_GUI_EVENTS_PER_WIN;
  return 1;
}

uint64_t sys_service_gui_get_screen_info(uint32_t *out_w, uint32_t *out_h, uint32_t *out_bpp) {
  if (!syscall_validate_user_ptr(out_w, sizeof(uint32_t))) return SYSCALL_BAD_ADDRESS;
  if (!syscall_validate_user_ptr(out_h, sizeof(uint32_t))) return SYSCALL_BAD_ADDRESS;
  if (!syscall_validate_user_ptr(out_bpp, sizeof(uint32_t))) return SYSCALL_BAD_ADDRESS;

  extern uint32_t g_kernel_screen_width;
  extern uint32_t g_kernel_screen_height;

  *out_w = g_kernel_screen_width > 0 ? g_kernel_screen_width : 1920;
  *out_h = g_kernel_screen_height > 0 ? g_kernel_screen_height : 1080;
  *out_bpp = 32;
  return SYSCALL_OK;
}
