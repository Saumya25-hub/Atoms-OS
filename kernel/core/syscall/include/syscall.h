#ifndef ATOMS_SYSCALL_H
#define ATOMS_SYSCALL_H

#include <stdbool.h>
#include <stdint.h>

#define ATOMS_SYSCALL_ABI_VERSION 1U

#define SYSCALL_OK 0ULL
#define SYSCALL_FAIL ((uint64_t)-1)
#define SYSCALL_INVALID ((uint64_t)-2)
#define SYSCALL_NOT_IMPLEMENTED ((uint64_t)-3)
#define SYSCALL_BAD_ADDRESS ((uint64_t)-4)
#define SYSCALL_TOO_LARGE ((uint64_t)-5)

/* Stable public Ring-3 ABI numbers. Keep userspace wrappers synchronized. */
#define SYS_YIELD 0U
#define SYS_WRITE 1U
#define SYS_SLEEP 2U
#define SYS_UPTIME 3U
#define SYS_GETPID 4U
#define SYS_EXIT 5U
#define SYS_OPEN 6U
#define SYS_READ 7U
#define SYS_CLOSE 8U
#define SYS_GETC 9U
#define SYS_SPAWN 10U
#define SYS_READDIR 11U
#define SYS_PS 12U
#define SYS_GET_KEY_EVENT 13U
#define SYS_GET_HEAP_STATS 14U
#define SYS_HEAP_DUMP 15U
#define SYS_MEMMAP 16U
#define SYS_DMESG 17U
#define SYS_TASK_INFO 18U
#define SYS_STRESS_HEAP 19U
#define SYS_HEAP_VALIDATE 20U
#define SYS_HEAP_WALK 21U
#define SYS_HEAP_TRACE_TOGGLE 22U
#define SYS_WRITE_FILE 23U
#define SYS_MKDIR 24U
#define SYS_CREATE 25U
#define SYS_RENAME 26U
#define SYS_DELETE 27U
#define SYS_CLEAR_SCREEN 28U
#define SYS_SET_CURSOR 29U
#define SYS_GUI_CREATE_WINDOW 30U
#define SYS_GUI_CREATE_BUTTON 31U
#define SYS_GUI_CREATE_LABEL 32U
#define SYS_GUI_CREATE_TEXTBOX 33U
#define SYS_GUI_CREATE_PANEL 34U
#define SYS_GUI_SHOW_WINDOW 35U
#define SYS_GUI_SET_TEXT 36U
#define SYS_GUI_SET_BOUNDS 37U
#define SYS_GUI_DESTROY 38U
#define SYS_GUI_GET_EVENT 39U
#define SYS_SEEK 40U
#define SYS_SURFACE_PRESENT 41U
#define SYS_GET_INPUT_EVENT 42U
#define MAX_SYSCALL 43U

#define ATOMS_SYSCALL_MAX_STRING 256U
#define ATOMS_SYSCALL_IO_CHUNK 4096U
#define ATOMS_SYSCALL_MAX_IO (1024U * 1024U)
#define ATOMS_SYSCALL_MAX_SURFACE_BYTES (16U * 1024U * 1024U)

/* Layout is consumed directly by syscall_entry.asm; offsets are ABI-critical.
 */
typedef struct ATOMS_SyscallFrame {
  uint64_t user_rsp;    /* 0 */
  uint64_t user_rip;    /* 8 */
  uint64_t user_rflags; /* 16 */
  uint64_t number;      /* 24 */
  uint64_t args[6];     /* 32..79 */
  uint64_t result;      /* 80 */
  uint64_t entry_task;  /* 88 */
  uint32_t pid;         /* 96 */
  uint32_t tid;         /* 100 */
  uint32_t cpu_id;      /* 104 */
  uint16_t nesting;     /* 108 */
  uint8_t terminated;   /* 110 */
  uint8_t return_mode;  /* 111 */
} ATOMS_SyscallFrame;

#define ATOMS_SYSCALL_FRAME_SIZE 112U
#define ATOMS_SYSCALL_RETURN_SYSRET 0U
#define ATOMS_SYSCALL_RETURN_IRET 1U
#define ATOMS_SYSCALL_RETURN_BLOCK 2U

typedef struct ATOMS_SyscallDiagnostics {
  uint64_t total_entries;
  uint64_t total_exits;
  uint64_t invalid_numbers;
  uint64_t not_implemented;
  uint64_t bad_user_pointers;
  uint64_t oversized_arguments;
  uint64_t rejected_returns;
  uint64_t nested_entries;
  uint64_t legacy_entries;
  uint64_t per_id[MAX_SYSCALL];
  uint64_t errors_per_id[MAX_SYSCALL];
} ATOMS_SyscallDiagnostics;

extern void syscall_init_asm(void);
void syscall_init(void);
uint64_t syscall_handler(ATOMS_SyscallFrame *frame);
uint64_t syscall_prepare_return(ATOMS_SyscallFrame *frame);
void syscall_get_diagnostics(ATOMS_SyscallDiagnostics *out);
bool syscall_phase5_self_test(void);

/* Kernel-task-only INT 0x80 compatibility wrappers. */
void sys_yield(void);
void sys_sleep(uint64_t ticks);
uint64_t sys_uptime(void);
uint64_t sys_getpid(void);

_Static_assert(sizeof(ATOMS_SyscallFrame) == ATOMS_SYSCALL_FRAME_SIZE,
               "syscall frame assembly/C layout drift");

#endif
