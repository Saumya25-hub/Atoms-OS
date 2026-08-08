#ifndef ATOMS_SYSCALL_H
#define ATOMS_SYSCALL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ATOMS_SYSCALL_ABI_VERSION 1U

/* Syscall Return Codes */
#define SYSCALL_OK 0ULL
#define SYSCALL_FAIL ((uint64_t)-1)
#define SYSCALL_INVALID ((uint64_t)-2)
#define SYSCALL_NOT_IMPLEMENTED ((uint64_t)-3)
#define SYSCALL_BAD_ADDRESS ((uint64_t)-4)
#define SYSCALL_TOO_LARGE ((uint64_t)-5)

/* Phase C Mandatory Syscall Numbers */
#define SYS_WRITE 0U
#define SYS_EXIT 1U
#define SYS_GETPID 2U
#define SYS_YIELD 3U
#define SYS_UPTIME 4U
#define SYS_ALLOC 5U
#define SYS_FREE 6U
#define SYS_DEBUG_PRINT 7U
#define MAX_SYSCALL 8U

/* Usermode Window Bounds for Security Validation */
#define USER_WINDOW_MIN 0x40000000ULL
#define USER_WINDOW_MAX 0x80000000ULL

/* Syscall Frame Layout (ABI-matched with syscall_entry.asm) */
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

/* MSR Constants */
#define IA32_EFER_MSR 0xC0000080U
#define IA32_STAR_MSR 0xC0000081U
#define IA32_LSTAR_MSR 0xC0000082U
#define IA32_FMASK_MSR 0xC0000084U

/* Function Prototypes */
bool syscall_phase5_self_test(void);

void syscall_init_msrs(void);
void syscall_init(void);

bool syscall_validate_user_ptr(const void *ptr, size_t size);

uint64_t syscall_dispatch(uint64_t id, uint64_t a1, uint64_t a2, uint64_t a3,
                         uint64_t a4, uint64_t a5, uint64_t a6);

uint64_t syscall_handler(ATOMS_SyscallFrame *frame);
uint64_t syscall_prepare_return(ATOMS_SyscallFrame *frame);

/* Legacy/Internal Helper Functions */
void sys_yield(void);
void sys_sleep(uint64_t ticks);
uint64_t sys_uptime(void);
uint64_t sys_getpid(void);

/* Syscall Services */
uint64_t sys_service_write(const char *user_str, size_t len);
uint64_t sys_service_exit(int code);
uint64_t sys_service_getpid(void);
uint64_t sys_service_yield(void);
uint64_t sys_service_uptime(void);
uint64_t sys_service_alloc(size_t size);
uint64_t sys_service_free(void *ptr);
uint64_t sys_service_debug_print(const char *msg);

/* Certification Routine */
void launch_phase_c_certification(void);

#endif
