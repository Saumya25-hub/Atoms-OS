#include "kernel/core/syscall/include/syscall.h"
#include "kernel/drivers/display/display.h"

extern void com1_dbg(const char *msg);

uint64_t syscall_dispatch(uint64_t id, uint64_t a1, uint64_t a2, uint64_t a3,
                         uint64_t a4, uint64_t a5, uint64_t a6) {
  (void)a4; (void)a5; (void)a6;

  /* Forensic Logging: ENTER ID=X */
  com1_dbg("[SYSCALL] ENTER ID=");
  char id_buf[16];
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
    extern void launch_phase_b_test1(void);
    launch_phase_b_test1();
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

  default:
    result = SYSCALL_INVALID;
    break;
  }

  /* Forensic Logging: EXIT ID=X RESULT=Y */
  com1_dbg("[SYSCALL] EXIT ID=");
  com1_dbg(id_buf);

  return result;
}
