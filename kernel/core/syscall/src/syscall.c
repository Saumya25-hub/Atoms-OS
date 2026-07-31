#include "kernel/core/syscall/include/syscall.h"
#include "kernel/core/interrupt/include/isr.h"
#include "kernel/core/lib/include/crash_log.h"
#include "kernel/core/loader/elf/include/elf_loader.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/process/include/process.h"
#include "kernel/core/process/include/process_builder.h"
#include "kernel/core/process/process_manager.h"
#include "kernel/core/scheduler/include/scheduler.h"
#include "kernel/core/timer/include/timer.h"
#include "kernel/core/usermode/user_mode.h"
#include "kernel/debug/phase7_reliability.h"
#include "kernel/drivers/display/display.h"
#include "kernel/drivers/keyboard/include/keyboard.h"
#include "kernel/shell/conhost/conhost.h"
#include "kernel/ui/events/gui_events.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/wm/bwe/include/bwe_process_queue.h"
#include "kernel/wm/bwe/include/bwe.h"


volatile uint64_t g_sys_get_input_event_calls;
volatile uint64_t g_sys_get_input_event_empty;
volatile uint32_t g_sys_get_input_event_last_pid;

static ATOMS_SyscallDiagnostics g_syscall_diag;

static void bytes_zero(void *p, uint64_t size) {
  uint8_t *b = (uint8_t *)p;
  while (size--)
    *b++ = 0;
}

static bool copy_user_string(uint64_t address, char *out, uint64_t capacity) {
  if (!address || !out || capacity < 2)
    return false;
  for (uint64_t i = 0; i < capacity; ++i) {
    if (!ATOMS_UserMode_CopyFromUser(&out[i], (const void *)(address + i), 1))
      return false;
    if (out[i] == '\0')
      return true;
  }
  out[capacity - 1] = '\0';
  return false;
}

static uint64_t reject_pointer(uint32_t id) {
  ++g_syscall_diag.bad_user_pointers;
  if (id < MAX_SYSCALL)
    ++g_syscall_diag.errors_per_id[id];
  return SYSCALL_BAD_ADDRESS;
}

static uint64_t reject_large(uint32_t id) {
  ++g_syscall_diag.oversized_arguments;
  if (id < MAX_SYSCALL)
    ++g_syscall_diag.errors_per_id[id];
  return SYSCALL_TOO_LARGE;
}

static int copy_in_path(uint32_t id, uint64_t user,
                        char path[ATOMS_SYSCALL_MAX_STRING]) {
  return copy_user_string(user, path, ATOMS_SYSCALL_MAX_STRING)
             ? 1
             : (int)reject_pointer(id);
}

static uint64_t copy_read_to_user(uint32_t id, int fd, uint64_t user_buffer,
                                  uint64_t size) {
  if (size > ATOMS_SYSCALL_MAX_IO)
    return reject_large(id);
  if (size == 0)
    return 0;
  uint8_t *chunk = (uint8_t *)kmalloc(ATOMS_SYSCALL_IO_CHUNK);
  if (!chunk)
    return SYSCALL_FAIL;
  uint64_t done = 0;
  while (done < size) {
    uint32_t wanted = (uint32_t)(size - done);
    if (wanted > ATOMS_SYSCALL_IO_CHUNK)
      wanted = ATOMS_SYSCALL_IO_CHUNK;
    int got = vfs_read(fd, chunk, wanted);
    if (got < 0) {
      kfree(chunk);
      return done ? done : (uint64_t)got;
    }
    if (got == 0)
      break;
    if (!ATOMS_UserMode_CopyToUser((void *)(user_buffer + done), chunk,
                                   (uint64_t)got)) {
      kfree(chunk);
      return reject_pointer(id);
    }
    done += (uint64_t)got;
    if ((uint32_t)got < wanted)
      break;
  }
  kfree(chunk);
  return done;
}

static uint64_t copy_write_from_user(uint32_t id, int fd, uint64_t user_buffer,
                                     uint64_t size) {
  if (size > ATOMS_SYSCALL_MAX_IO)
    return reject_large(id);
  if (size == 0)
    return 0;
  uint8_t *chunk = (uint8_t *)kmalloc(ATOMS_SYSCALL_IO_CHUNK);
  if (!chunk)
    return SYSCALL_FAIL;
  uint64_t done = 0;
  while (done < size) {
    uint32_t wanted = (uint32_t)(size - done);
    if (wanted > ATOMS_SYSCALL_IO_CHUNK)
      wanted = ATOMS_SYSCALL_IO_CHUNK;
    if (!ATOMS_UserMode_CopyFromUser(chunk, (const void *)(user_buffer + done),
                                     wanted)) {
      kfree(chunk);
      return reject_pointer(id);
    }
    int put = vfs_write(fd, chunk, wanted);
    if (put < 0) {
      kfree(chunk);
      return done ? done : (uint64_t)put;
    }
    done += (uint64_t)put;
    if ((uint32_t)put < wanted)
      break;
  }
  kfree(chunk);
  return done;
}

static uint64_t dispatch_syscall(ATOMS_SyscallFrame *frame) {
  const uint32_t id = (uint32_t)frame->number;
  const uint64_t a1 = frame->args[0], a2 = frame->args[1];
  const uint64_t a3 = frame->args[2], a4 = frame->args[3], a5 = frame->args[4];
  char path[ATOMS_SYSCALL_MAX_STRING];
  char path2[ATOMS_SYSCALL_MAX_STRING];

  switch (id) {
  case SYS_YIELD:
    scheduler_yield();
    return SYSCALL_OK;
  case SYS_WRITE:
    if (!copy_user_string(a1, path, sizeof(path)))
      return reject_pointer(id);
    if (scheduler_current_task() &&
        conhost_write_pid(scheduler_current_task()->id, path))
      return SYSCALL_OK;
    display_print(path);
    return SYSCALL_OK;
  case SYS_SLEEP:
    scheduler_sleep(a1);
    return SYSCALL_OK;
  case SYS_UPTIME:
    return timer_get_ticks();
  case SYS_GETPID:
    return scheduler_current_task() ? scheduler_current_task()->id : 0;
  case SYS_EXIT: {
    Task *current = scheduler_current_task();
    if (!current || current == scheduler_get_idle_task())
      return SYSCALL_FAIL;
    ATOMS_Process_Terminate(current->owner_pid ? current->owner_pid
                                               : (uint32_t)current->id,
                            (int32_t)a1);
    frame->terminated = 1;
    scheduler_yield();
    return SYSCALL_OK;
  }
  case SYS_OPEN:
    if (copy_in_path(id, a1, path) != 1)
      return SYSCALL_BAD_ADDRESS;
    return (uint64_t)vfs_open(path);
  case SYS_READ:
    return copy_read_to_user(id, (int)a1, a2, a3);
  case SYS_CLOSE:
    return (uint64_t)vfs_close((int)a1);
  case SYS_GETC:
    return keyboard_getc();
  case SYS_SPAWN: {
    if (copy_in_path(id, a1, path) != 1)
      return SYSCALL_BAD_ADDRESS;
    void *new_pml4 = vmm_create_address_space();
    ProcessImage *image = new_pml4 ? elf_load_image(new_pml4, path) : 0;
    if (!image || !process_build_user_stack(image, new_pml4))
      return SYSCALL_FAIL;
    Task *current = scheduler_current_task();
    ATOMS_PCB *pcb = ATOMS_Process_Create(
        path, path, current ? (uint32_t)current->id : 0, 0);
    if (!pcb)
      return SYSCALL_FAIL;
    pcb->pml4_phys = (uint64_t)new_pml4;
    image->pid = pcb->pid;
    Task *task = process_spawn(image, path);
    if (!task) {
      ATOMS_Process_Terminate(pcb->pid, -1);
      return SYSCALL_FAIL;
    }
    return task->id;
  }
  case SYS_READDIR: {
    if (copy_in_path(id, a1, path) != 1)
      return SYSCALL_BAD_ADDRESS;
    vfs_dirent_t entry;
    int result = vfs_readdir(path, (int)a2, &entry);
    if (result >= 0 &&
        !ATOMS_UserMode_CopyToUser((void *)a3, &entry, sizeof(entry)))
      return reject_pointer(id);
    return (uint64_t)result;
  }
  case SYS_PS:
    scheduler_dump_tasks();
    return SYSCALL_OK;
  case SYS_GET_KEY_EVENT: {
    KeyboardEvent event;
    bool found = false;
    Task *current = scheduler_current_task();
    if (current && conhost_get_session_by_pid(current->id))
      found = conhost_pop_key_pid(current->id, &event);
    else
      found = keyboard_poll_event(&event);
    if (found && !ATOMS_UserMode_CopyToUser((void *)a1, &event, sizeof(event)))
      return reject_pointer(id);
    return found ? 1 : 0;
  }
  case SYS_GET_HEAP_STATS: {
    HeapStats stats;
    heap_get_stats(&stats);
    display_print("\n--- Kernel Heap Info ---\nUsed: ");
    display_print_dec(stats.used_size);
    display_print(" bytes\n");
    return SYSCALL_OK;
  }
  case SYS_HEAP_DUMP:
    heap_dump_blocks();
    return SYSCALL_OK;
  case SYS_MEMMAP:
    pmm_print_memmap();
    return SYSCALL_OK;
  case SYS_DMESG:
    crash_log_dump();
    return SYSCALL_OK;
  case SYS_TASK_INFO:
    scheduler_dump_task_info(a1);
    return SYSCALL_OK;
  case SYS_STRESS_HEAP:
    heap_stress_test();
    return SYSCALL_OK;
  case SYS_HEAP_VALIDATE:
    heap_validate();
    return SYSCALL_OK;
  case SYS_HEAP_WALK:
    heap_walk();
    return SYSCALL_OK;
  case SYS_HEAP_TRACE_TOGGLE:
    heap_trace_toggle();
    return SYSCALL_OK;
  case SYS_WRITE_FILE:
    return copy_write_from_user(id, (int)a1, a2, a3);
  case SYS_MKDIR:
  case SYS_CREATE:
  case SYS_DELETE:
    if (copy_in_path(id, a1, path) != 1)
      return SYSCALL_BAD_ADDRESS;
    if (id == SYS_MKDIR)
      return (uint64_t)vfs_mkdir(path);
    if (id == SYS_CREATE)
      return (uint64_t)vfs_create(path);
    return (uint64_t)vfs_delete(path);
  case SYS_RENAME:
    if (copy_in_path(id, a1, path) != 1 || copy_in_path(id, a2, path2) != 1)
      return SYSCALL_BAD_ADDRESS;
    return (uint64_t)vfs_rename(path, path2);
  case SYS_CLEAR_SCREEN:
    if (scheduler_current_task() &&
        conhost_clear_pid(scheduler_current_task()->id))
      return SYSCALL_OK;
    display_clear();
    return SYSCALL_OK;
  case SYS_SET_CURSOR:
    if (scheduler_current_task() &&
        conhost_set_cursor_pid(scheduler_current_task()->id, (uint16_t)a1,
                               (uint16_t)a2))
      return SYSCALL_OK;
    display_set_cursor((uint16_t)a1, (uint16_t)a2);
    return SYSCALL_OK;
  case SYS_GUI_CREATE_WINDOW: {
    if (!copy_user_string(a5, path, sizeof(path)))
      return reject_pointer(id);
    extern uint32_t g_current_creating_pid;
    Task *current = scheduler_current_task();
    g_current_creating_pid = current ? current->id : 0;
    uint32_t window = 0;
    bwe_error_t error = BOS_CreateWindow((int32_t)a1, (int32_t)a2, (int32_t)a3,
                                         (int32_t)a4, path, &window);
    g_current_creating_pid = 0;
    return error == BWE_SUCCESS ? window : SYSCALL_FAIL;
  }
  case SYS_GUI_CREATE_BUTTON: {
    uint64_t ext[3];
    if (!ATOMS_UserMode_CopyFromUser(ext, (const void *)a5, sizeof(ext)) ||
        !copy_user_string(ext[1], path, sizeof(path)))
      return reject_pointer(id);
    uint32_t button = 0;
    bwe_error_t error =
        BOS_CreateButton((uint32_t)a1, (uint32_t)a2, (uint32_t)a3, (uint32_t)a4,
                         (uint32_t)ext[0], path, 0, &button);
    if (error == BWE_SUCCESS) {
      BWE_Window *win = BWE_GetWindow(button);
      Task *current = scheduler_current_task();
      if (win && current) {
        win->owner_pid = current->id;
        win->control_data.button.user_callback = ext[2];
      }
    }
    return error == BWE_SUCCESS ? button : SYSCALL_FAIL;
  }
  case SYS_GUI_CREATE_LABEL: {
    if (!copy_user_string(a4, path, sizeof(path)))
      return reject_pointer(id);
    uint32_t label = 0;
    bwe_error_t error = BOS_CreateLabel(
        (uint32_t)a1, (uint32_t)a2, (uint32_t)a3, path, (uint32_t)a5, &label);
    if (error == BWE_SUCCESS && scheduler_current_task()) {
      BWE_Window *win = BWE_GetWindow(label);
      if (win)
        win->owner_pid = scheduler_current_task()->id;
    }
    return error == BWE_SUCCESS ? label : SYSCALL_FAIL;
  }
  case SYS_GUI_CREATE_PANEL: {
    uint64_t ext[2];
    if (!ATOMS_UserMode_CopyFromUser(ext, (const void *)a5, sizeof(ext)))
      return reject_pointer(id);
    uint32_t panel = 0;
    bwe_error_t error =
        BOS_CreatePanel((uint32_t)a1, (uint32_t)a2, (uint32_t)a3, (uint32_t)a4,
                        (uint32_t)ext[0], (uint32_t)ext[1], &panel);
    if (error == BWE_SUCCESS && scheduler_current_task()) {
      BWE_Window *win = BWE_GetWindow(panel);
      if (win)
        win->owner_pid = scheduler_current_task()->id;
    }
    return error == BWE_SUCCESS ? panel : SYSCALL_FAIL;
  }
  case SYS_GUI_SHOW_WINDOW:
    return BOS_Show((uint32_t)a1) == BWE_SUCCESS &&
                   BOS_SetFocus((uint32_t)a1) == BWE_SUCCESS
               ? SYSCALL_OK
               : SYSCALL_FAIL;
  case SYS_GUI_SET_TEXT:
    if (!copy_user_string(a2, path, sizeof(path)))
      return reject_pointer(id);
    BOS_SetText((uint32_t)a1, path);
    return SYSCALL_OK;
  case SYS_GUI_GET_EVENT: {
    Task *current = scheduler_current_task();
    BOS_GUIEvent event;
    int found = current ? bos_gui_event_pop(current->id, &event) : 0;
    if (found && !ATOMS_UserMode_CopyToUser((void *)a1, &event, sizeof(event)))
      return reject_pointer(id);
    return (uint64_t)found;
  }
  case SYS_GUI_SET_CORNER_RADIUS: {
    uint32_t target_id = (uint32_t)a1;
    uint32_t radius = (uint32_t)a2;
    BWE_Window *win = BWE_GetWindow(target_id);
    if (win) {
      win->corner_radius = radius;
      BWE_InvalidateWindow(target_id);
      return SYSCALL_OK;
    }
    return SYSCALL_FAIL;
  }
  case SYS_GUI_SET_GRADIENT: {
    uint32_t target_id = (uint32_t)a1;
    uint32_t color_start = (uint32_t)a2;
    uint32_t color_end = (uint32_t)a3;
    uint8_t mode = (uint8_t)a4;
    BWE_Window *win = BWE_GetWindow(target_id);
    if (win) {
      if (win->type == BWE_TYPE_PANEL) {
        win->control_data.panel.bg_color = color_start;
      } else if (win->type == BWE_TYPE_BUTTON) {
        win->control_data.button.bg_color = color_start;
      }
      win->gradient_color_end = color_end;
      win->gradient_mode = mode;
      BWE_InvalidateWindow(target_id);
      return SYSCALL_OK;
    }
    return SYSCALL_FAIL;
  }
  case SYS_SEEK:
    return (uint64_t)vfs_seek((int)a1, a2, (int)a3);
  case SYS_SURFACE_PRESENT: {
    uint64_t pixels;
    if (a3 == 0 || a4 == 0 || a3 > UINT64_MAX / a4 ||
        a3 * a4 > UINT64_MAX / sizeof(uint32_t))
      return reject_large(id);
    pixels = a3 * a4 * sizeof(uint32_t);
    if (pixels > ATOMS_SYSCALL_MAX_SURFACE_BYTES)
      return reject_large(id);
    uint32_t *copy = (uint32_t *)kmalloc((size_t)pixels);
    if (!copy)
      return SYSCALL_FAIL;
    if (!ATOMS_UserMode_CopyFromUser(copy, (const void *)a2, pixels)) {
      kfree(copy);
      return reject_pointer(id);
    }
    extern bwe_error_t BOS_SurfacePresent(uint32_t, const uint32_t *, uint32_t,
                                          uint32_t);
    bwe_error_t error =
        BOS_SurfacePresent((uint32_t)a1, copy, (uint32_t)a3, (uint32_t)a4);
    kfree(copy);
    return error == BWE_SUCCESS ? SYSCALL_OK : SYSCALL_FAIL;
  }
  case SYS_GET_INPUT_EVENT: {
    ++g_sys_get_input_event_calls;
    Task *current = scheduler_current_task();
    BOS_InputEvent event;
    bool found = current && bwe_process_queue_pop(current->id, &event);
    if (current)
      g_sys_get_input_event_last_pid = (uint32_t)current->id;
    if (found && !ATOMS_UserMode_CopyToUser((void *)a1, &event, sizeof(event)))
      return reject_pointer(id);
    if (!found)
      ++g_sys_get_input_event_empty;
    return found ? 1 : 0;
  }
  case SYS_GUI_SET_BOUNDS: {
    uint32_t target_id = (uint32_t)a1;
    uint32_t x = (uint32_t)a2;
    uint32_t y = (uint32_t)a3;
    uint32_t w = (uint32_t)a4;
    uint32_t h = (uint32_t)a5;
    bwe_error_t err = BOS_SetBounds(target_id, x, y, w, h);
    return err == BWE_SUCCESS ? SYSCALL_OK : SYSCALL_FAIL;
  }
  case SYS_GUI_CREATE_TEXTBOX:
  case SYS_GUI_DESTROY:
  default:
    ++g_syscall_diag.not_implemented;
    ++g_syscall_diag.errors_per_id[id];
    return SYSCALL_NOT_IMPLEMENTED;
  }
}

uint64_t syscall_handler(ATOMS_SyscallFrame *frame) {
  uint64_t start_cycles = 0;
  __asm__ volatile("rdtsc"
                   : "=a"(*(uint32_t *)&start_cycles),
                     "=d"(*((uint32_t *)&start_cycles + 1)));
  if (!frame)
    return SYSCALL_INVALID;
  ++g_syscall_diag.total_entries;
  if (frame->number >= MAX_SYSCALL) {
    ++g_syscall_diag.invalid_numbers;
    frame->result = SYSCALL_INVALID;
    return frame->result;
  }
  ++g_syscall_diag.per_id[frame->number];
  Task *task = scheduler_current_task();
  frame->entry_task = (uint64_t)task;
  frame->pid = task ? task->owner_pid : 0;
  frame->tid = task ? (uint32_t)task->id : 0;
  if (frame->nesting > 1)
    ++g_syscall_diag.nested_entries;
  frame->result = dispatch_syscall(frame);
  bool error = (int64_t)frame->result < 0;
  if (error)
    ++g_syscall_diag.errors_per_id[frame->number];
  ++g_syscall_diag.total_exits;
  uint64_t end_cycles = 0;
  __asm__ volatile("rdtsc"
                   : "=a"(*(uint32_t *)&end_cycles),
                     "=d"(*((uint32_t *)&end_cycles + 1)));
  atoms_p7_note_syscall((uint32_t)frame->number, end_cycles - start_cycles,
                        error, frame->user_rip);
  return frame->result;
}

uint64_t syscall_prepare_return(ATOMS_SyscallFrame *frame) {
  if (!frame)
    return ATOMS_SYSCALL_RETURN_BLOCK;
  Task *task = scheduler_current_task();
  if (frame->terminated || !task || (uint64_t)task != frame->entry_task ||
      task->state == TASK_TERMINATED || !task->is_user_task ||
      !ATOMS_UserMode_IsUserRange(frame->user_rip, 1) ||
      !ATOMS_UserMode_IsUserRange(frame->user_rsp - 1, 1) ||
      !ATOMS_UserMode_ValidateAddress(task->pml4, frame->user_rip, 1,
                                      VMM_ACCESS_READ | VMM_ACCESS_EXECUTE) ||
      !ATOMS_UserMode_ValidateAddress(task->pml4, frame->user_rsp - 1, 1,
                                      VMM_ACCESS_READ | VMM_ACCESS_WRITE)) {
    ++g_syscall_diag.rejected_returns;
    frame->return_mode = ATOMS_SYSCALL_RETURN_BLOCK;
    if (task && task != scheduler_get_idle_task())
      scheduler_terminate_task(task);
    return frame->return_mode;
  }
  const uint64_t unsafe = (1ULL << 8) | (1ULL << 10) | (3ULL << 12) |
                          (1ULL << 14) | (1ULL << 16) | (1ULL << 17) |
                          (1ULL << 18);
  if (frame->user_rflags & unsafe) {
    frame->user_rflags &= ~unsafe;
    frame->user_rflags |= 0x202ULL;
    frame->return_mode = ATOMS_SYSCALL_RETURN_IRET;
    return frame->return_mode;
  }
  frame->user_rflags |= 0x202ULL;
  frame->return_mode = ATOMS_SYSCALL_RETURN_SYSRET;
  return frame->return_mode;
}

void syscall_get_diagnostics(ATOMS_SyscallDiagnostics *out) {
  if (out)
    *out = g_syscall_diag;
}

bool syscall_phase5_self_test(void) {
  ATOMS_SyscallFrame frame;
  bytes_zero(&frame, sizeof(frame));
  frame.number = MAX_SYSCALL;
  bool bounds = syscall_handler(&frame) == SYSCALL_INVALID;
  bool canonical = ATOMS_UserMode_IsCanonical(0x00007FFFFFFFFFFFULL) &&
                   !ATOMS_UserMode_IsCanonical(0x0000800000000000ULL);
  bool ranges = ATOMS_UserMode_IsUserRange(ATOMS_USER_MIN_ADDRESS, 1) &&
                !ATOMS_UserMode_IsUserRange(0, 1) &&
                !ATOMS_UserMode_IsUserRange(UINT64_MAX - 1, 8);
  return bounds && canonical && ranges &&
         sizeof(ATOMS_SyscallFrame) == ATOMS_SYSCALL_FRAME_SIZE;
}

/* Kernel-only compatibility namespace: 0=yield, 1=sleep, 2=uptime, 3=getpid. */
static uint64_t syscall_dispatcher_legacy(registers_t *regs) {
  ++g_syscall_diag.legacy_entries;
  switch (regs->rax) {
  case 0:
    scheduler_yield();
    regs->rax = 0;
    break;
  case 1:
    scheduler_sleep(regs->rdi);
    regs->rax = 0;
    break;
  case 2:
    regs->rax = timer_get_ticks();
    break;
  case 3:
    regs->rax = scheduler_current_task() ? scheduler_current_task()->id : 0;
    break;
  default:
    regs->rax = SYSCALL_INVALID;
    break;
  }
  return 0;
}

void syscall_init(void) {
  bytes_zero(&g_syscall_diag, sizeof(g_syscall_diag));
  syscall_init_asm();
  isr_register_handler(128, syscall_dispatcher_legacy);
}
