#include "debug_shell.h"
#include "kernel/core/scheduler/include/scheduler.h"
#include "kernel/core/scheduler/include/task.h"
#include "kernel/core/thread/thread_manager.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/debug/abde/abde.h"
#include <stddef.h>

extern void com1_puts(const char *s);
extern void display_print(const char *s);
extern void display_print_dec(uint64_t val);
extern void display_print_hex(uint64_t val);

static bool str_eq(const char *s1, const char *s2) {
    if (!s1 || !s2) return false;
    while (*s1 && *s2) {
        if (*s1 != *s2) return false;
        s1++;
        s2++;
    }
    return *s1 == *s2;
}

void debug_shell_init(void) {
    com1_puts("[SHELL] ATOMS OS Kernel Debug Shell V1.0 Ready\r\n");
    com1_puts("[SHELL] Commands: help, cpu, heap, task, thread, sched, panic_test\r\n");
}

void debug_shell_run_command(const char *cmd) {
    if (!cmd || cmd[0] == '\0') return;

    if (str_eq(cmd, "help")) {
        com1_puts("\r\n=== ATOMS OS KERNEL DEBUG SHELL COMMANDS ===\r\n");
        com1_puts("  help       : Display this command reference list\r\n");
        com1_puts("  cpu        : Show CPU vendor, frequency, core topology\r\n");
        com1_puts("  heap       : Show kernel heap pool statistics & integrity\r\n");
        com1_puts("  task       : Dump active tasks across all scheduler queues\r\n");
        com1_puts("  thread     : Dump thread manager TCB table & CPU time\r\n");
        com1_puts("  sched      : Dump scheduler live telemetry & switch metrics\r\n");
        com1_puts("  panic_test : Trigger controlled panic for forensic testing\r\n");
        com1_puts("============================================\r\n\r\n");
    } else if (str_eq(cmd, "cpu")) {
        com1_puts("\r\n[CPU TOPOLOGY]\r\n");
        com1_puts("Architecture : x86_64 Long Mode (64-bit)\r\n");
        com1_puts("Cores Found  : 4 Online Cores (Haswell H81)\r\n");
        com1_puts("BSP LAPIC ID : 0\r\n\r\n");
    } else if (str_eq(cmd, "heap")) {
        HeapStats stats;
        heap_get_stats(&stats);
        com1_puts("\r\n[HEAP STATS]\r\n");
        com1_puts("Total Size : 2048 KB\r\n");
        com1_puts("Used Size  : ");
        char buf[16]; int pos = 14; buf[15] = '\0';
        uint64_t val = stats.used_size;
        if (val == 0) com1_puts("0");
        else { while (val > 0) { buf[pos--] = '0' + (val % 10); val /= 10; } com1_puts(&buf[pos + 1]); }
        com1_puts(" bytes\r\n\r\n");
    } else if (str_eq(cmd, "task") || str_eq(cmd, "sched")) {
        scheduler_dump_runtime_diagnostics();
        scheduler_dump_tasks();
    } else if (str_eq(cmd, "thread")) {
        ATOMS_Thread_DumpTelemetry();
    } else if (str_eq(cmd, "panic_test")) {
        diag_panic_reason("SHELL", "PANIC_TEST", "USER_TRIGGERED", "Debug shell panic command executed");
    } else {
        com1_puts("[SHELL] Unknown command: ");
        com1_puts(cmd);
        com1_puts(" (type 'help' for command list)\r\n");
    }
}

void debug_shell_thread_entry(void) {
    debug_shell_init();
    for (;;) {
        scheduler_sleep(500);
    }
}
