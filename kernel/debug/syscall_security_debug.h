#ifndef ATOMS_SYSCALL_SECURITY_DEBUG_H
#define ATOMS_SYSCALL_SECURITY_DEBUG_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/core/core_legacy/boot/include/boot_info.h"

#define SYSCALL_SEC_TOTAL_DISCOVERED 30
#define SYSCALL_SEC_POINTER_SYSCALLS 13

typedef enum {
    TEST_STATE_IDLE = 0,
    TEST_STATE_RUNNING,
    TEST_STATE_PASS,
    TEST_STATE_FAIL,
    TEST_STATE_VULN_PROVEN
} SyscallTestState;

typedef struct {
    const char *test_id;          /* "TEST A", "TEST B", etc. */
    const char *test_name;        /* "VALID USER POINTER", etc. */
    uint32_t syscall_id;          /* 0, 7, 11, etc. */
    const char *syscall_name;     /* "SYS_WRITE", etc. */
    uint64_t pointer_val;         /* The virtual address passed */
    uint64_t size_val;            /* Size argument */
    const char *expected_desc;    /* "PASS (SYSCALL_OK)", "REJECT (BAD_ADDRESS)" */
    const char *observed_desc;    /* "PASS", "CRASH_PREVENTED", etc. */
    SyscallTestState state;
    bool range_valid;
    bool is_canonical;
    bool is_mapped;
    bool is_user;
    bool is_writable;
    bool fault_occurred;
    uint32_t fault_cpl;
    uint64_t fault_cr2;
    uint64_t fault_rip;
} SyscallSecurityTestCase;

typedef struct {
    /* 1. Inventory */
    uint32_t discovered_count;
    uint32_t pointer_syscalls_count;
    uint32_t validated_count;
    uint32_t unsafe_candidates_count;

    /* 2. Active Test Telemetry */
    const char *current_test_name;
    const char *target_syscall_name;
    uint64_t current_pointer;
    uint64_t current_size;
    const char *current_expected;
    const char *current_observed;

    /* 3. Memory Validation Matrix */
    bool user_range_pass;
    bool is_mapped;
    bool is_present;
    bool is_user_perm;
    bool is_writable;
    bool is_canonical;
    bool range_overflow;
    bool crosses_page_boundary;

    /* 4. Fault Monitor */
    uint32_t pf_count;
    uint32_t cpl0_faults;
    uint32_t cpl3_faults;
    uint32_t kernel_panics;
    uint32_t process_terminated;
    uint32_t recovered_safely;

    /* 5. Live State */
    uint64_t uptime_sec;
    uint64_t heartbeat_tick;
    const char *last_result_str;
    bool all_tests_passed;
    bool vulnerability_proven;
} SyscallSecurityStats;

extern SyscallSecurityStats g_syscall_sec_stats;
extern volatile bool g_syscall_fault_catch_active;
extern volatile uint64_t g_syscall_fault_recovery_rip;
extern volatile uint64_t g_syscall_last_fault_cr2;
extern volatile uint32_t g_syscall_last_fault_cpl;
extern volatile uint32_t g_syscall_cpl0_fault_count;

void syscall_security_debug_init(boot_info_t *boot_info);
void syscall_security_debug_render(void);
void syscall_security_debug_run(boot_info_t *boot_info);

#endif /* ATOMS_SYSCALL_SECURITY_DEBUG_H */
