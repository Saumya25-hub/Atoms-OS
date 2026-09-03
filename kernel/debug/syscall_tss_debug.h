#ifndef SYSCALL_TSS_DEBUG_H
#define SYSCALL_TSS_DEBUG_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/core/core_legacy/boot/include/boot_info.h"

#define SYSCALL_MAX_DISPLAY_CPUS 8U

typedef struct {
    char        cpu_name[64];
    uint32_t    discovered_cpus;
    uint32_t    online_cpus;
    uint32_t    current_cpu;
    uint64_t    active_cr3;
    uint16_t    active_tr;
    uint64_t    gdtr_base;
    uint16_t    gdtr_limit;
    
    // MSR Registers
    uint64_t    msr_efer;
    uint64_t    msr_star;
    uint64_t    msr_lstar;
    uint64_t    msr_fmask;
    
    // TSS & RSP0
    const char  *tss_model_str;
    uint64_t    global_tss_addr;
    uint64_t    current_tss_addr;
    uint64_t    rsp0_val;
    uint64_t    kernel_stack_top;
    const char  *stack_owner_str;
    
    // Per-CPU state
    struct {
        uint32_t logical_id;
        uint32_t apic_id;
        bool     online;
        bool     is_bsp;
        uint64_t tss_addr;
        uint64_t rsp0;
        uint64_t heartbeat_count;
        uint64_t kernel_entries;
        uint64_t syscalls_handled;
    } cpus[SYSCALL_MAX_DISPLAY_CPUS];
    
    // Syscall test
    bool        syscall_test_executed;
    bool        syscall_test_passed;
    uint32_t    syscall_test_cpu;
    uint64_t    syscall_test_num;
    uint64_t    syscall_test_arg1;
    uint64_t    syscall_test_result;
    
    // Summary
    const char  *smp_status_str;
    const char  *smp_verdict_str;
    const char  *tss_verdict_str;
    const char  *syscall_verdict_str;
    const char  *arch_status_str;
} SyscallTSSDebugStats;

void syscall_tss_debug_init(boot_info_t *boot_info);
void syscall_tss_debug_render(void);
void syscall_tss_debug_run(boot_info_t *boot_info);

#endif /* SYSCALL_TSS_DEBUG_H */
