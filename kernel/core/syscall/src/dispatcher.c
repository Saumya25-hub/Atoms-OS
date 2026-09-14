#include "kernel/core/syscall/include/syscall.h"
#include "kernel/drivers/display/display.h"

extern void com1_dbg(const char *msg);

uint64_t syscall_dispatch(uint64_t id, uint64_t a1, uint64_t a2, uint64_t a3,
                         uint64_t a4, uint64_t a5, uint64_t a6) {
  (void)a4; (void)a5; (void)a6;

  /* Forensic Logging: ENTER ID=X (suppress high-frequency polling syscalls) */
  bool is_noisy = (id == SYS_YIELD || id == SYS_UPTIME || id == SYS_GUI_POLL_EVENT);
  char id_buf[16];
  if (!is_noisy) {
    com1_dbg("[SYSCALL] ENTER ID=");
    int idx = 0;
    uint64_t temp_id = id;
    if (temp_id == 0) {
      id_buf[idx++] = '0';
    } else {
      char rev[16];
      int r = 0;
      while (temp_id > 0) {
        rev[r++] = '0' + (temp_id % 10);
        temp_id /= 10;
      }
      while (r > 0) {
        id_buf[idx++] = rev[--r];
      }
    }
    id_buf[idx++] = '\n';
    id_buf[idx] = '\0';
    com1_dbg(id_buf);
  }

  uint64_t result = SYSCALL_INVALID;

  switch (id) {
  case SYS_WRITE:
    result = sys_service_write((const char *)a1, (size_t)a2);
    break;

  case SYS_EXIT:
    result = sys_service_exit((int)a1);
    break;

  case SYS_GETPID:
    result = sys_service_getpid();
    break;

  case SYS_YIELD:
    result = sys_service_yield();
    break;

  case SYS_UPTIME:
    result = sys_service_uptime();
    break;

  case SYS_ALLOC:
    result = sys_service_alloc((size_t)a1);
    break;

  case SYS_FREE:
    result = sys_service_free((void *)a1);
    break;

  case SYS_DEBUG_PRINT:
    result = sys_service_debug_print((const char *)a1);
    break;

  case SYS_MMAP:
    result = sys_service_mmap(a1, (size_t)a2, (int)a3, (int)a4, (int)a5, a6);
    break;

  case SYS_MUNMAP:
    result = sys_service_munmap(a1, (size_t)a2);
    break;

  case SYS_MPROTECT:
    result = sys_service_mprotect(a1, (size_t)a2, (int)a3);
    break;

  case SYS_FUTEX:
    result = sys_service_futex((uint32_t *)a1, (int)a2, (uint32_t)a3, (const void *)a4);
    break;

  case SYS_CLOCK_GETTIME:
    result = sys_service_clock_gettime((int)a1, (void *)a2);
    break;

  case SYS_NANOSLEEP:
    result = sys_service_nanosleep((const void *)a1, (void *)a2);
    break;

  case SYS_OPEN:
    result = sys_service_open((const char *)a1, (int)a2, (int)a3);
    break;

  case SYS_READ:
    result = sys_service_read((int)a1, (void *)a2, (size_t)a3);
    break;

  case SYS_CLOSE:
    result = sys_service_close((int)a1);
    break;

  case SYS_SEEK:
    result = sys_service_seek((int)a1, a2, (int)a3);
    break;

  case SYS_THREAD_SPAWN:
    result = sys_service_thread_spawn((void (*)(void *))a1, (void *)a2, (void *)a3);
    break;

  case SYS_THREAD_EXIT:
    result = sys_service_thread_exit((int)a1);
    break;

  case SYS_WRITE_FILE:
    result = sys_service_write_file((int)a1, (const void *)a2, (size_t)a3);
    break;

  case SYS_GUI_CREATE_WINDOW:
    result = sys_service_gui_create_window((int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (uint32_t)a5, (const char *)a6);
    break;

  case SYS_GUI_DESTROY_WINDOW:
    result = sys_service_gui_destroy_window((uint32_t)a1);
    break;

  case SYS_GUI_SHOW_WINDOW:
    result = sys_service_gui_show_window((uint32_t)a1, (uint32_t)a2);
    break;

  case SYS_GUI_SET_BOUNDS:
    result = sys_service_gui_set_bounds((uint32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5);
    break;

  case SYS_GUI_MAP_SURFACE:
    result = sys_service_gui_map_surface((uint32_t)a1, (uint64_t *)a2, (uint32_t *)a3);
    break;

  case SYS_GUI_INVALIDATE:
    result = sys_service_gui_invalidate((uint32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5);
    break;

  case SYS_GUI_POLL_EVENT:
    result = sys_service_gui_poll_event((uint32_t)a1, (BOS_GUIEvent *)a2, (uint32_t)a3);
    break;

  case SYS_GUI_GET_SCREEN_INFO:
    result = sys_service_gui_get_screen_info((uint32_t *)a1, (uint32_t *)a2, (uint32_t *)a3);
    break;

  case SYS_GUI_DRAW_WALLPAPER:
    result = sys_service_gui_draw_wallpaper((uint32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5);
    break;

  case SYS_CREATE:
    result = sys_service_create((const char *)a1, (int)a2);
    break;

  case SYS_MKDIR:
    result = sys_service_mkdir((const char *)a1, (int)a2);
    break;

  case SYS_READDIR:
    result = sys_service_readdir((const char *)a1, (int)a2, (void *)a3);
    break;

  case SYS_UNLINK:
    result = sys_service_unlink((const char *)a1);
    break;

  case SYS_RENAME:
    result = sys_service_rename((const char *)a1, (const char *)a2);
    break;

  case SYS_RMDIR:
    result = sys_service_rmdir((const char *)a1);
    break;

  case SYS_STAT:
    result = sys_service_stat((const char *)a1, (void *)a2);
    break;

  case SYS_EXEC:
    result = sys_service_exec((const char *)a1, (const char **)a2, (const char **)a3);
    break;

  case SYS_WAITPID:
    result = sys_service_waitpid((uint32_t)a1, (int32_t *)a2, (uint32_t)a3);
    break;

  case SYS_IPC_CALL:
    result = sys_service_ipc_call((uint32_t)a1, a2, a3, a4);
    break;

  case SYS_SHM_CALL:
    result = sys_service_shm_call((uint32_t)a1, a2, a3, a4);
    break;

  case SYS_KILL:
    result = sys_service_kill((uint32_t)a1, (int32_t)a2);
    break;

  case SYS_PROCESS_STATUS:
    result = sys_service_process_status((uint32_t)a1, (void *)a2);
    break;

  case SYS_AUDIO_CALL:
    result = sys_service_audio_call((uint32_t)a1, a2, a3, a4, a5);
    break;

  default:
    result = SYSCALL_INVALID;
    break;
  }


  /* Forensic Logging: EXIT ID=X RESULT=Y */
  if (!is_noisy) {
    com1_dbg("[SYSCALL] EXIT ID=");
    com1_dbg(id_buf);
  }

  return result;
}
