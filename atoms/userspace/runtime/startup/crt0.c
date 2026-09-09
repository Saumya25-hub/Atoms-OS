#include "atoms/userspace/runtime/include/atoms_syscall.h"

extern int main(int argc, char **argv, char **envp);
extern void atoms_heap_init(void);

/* Weak hooks for global constructors and destructors */
extern void (*__init_array_start[])(void) __attribute__((weak));
extern void (*__init_array_end[])(void) __attribute__((weak));
extern void (*__fini_array_start[])(void) __attribute__((weak));
extern void (*__fini_array_end[])(void) __attribute__((weak));

/* Stack protector canary */
uint64_t __stack_chk_guard = 0x595e9fbd94fda766ULL;

void __stack_chk_fail(void) {
    const char msg[] = "[ATOMS USERSPACE] FATAL: Stack smashing detected!\n";
    atoms_sys_write(1, msg, sizeof(msg) - 1);
    atoms_sys_exit(127);
}

static void call_init_array(void) {
    if (__init_array_start && __init_array_end) {
        size_t count = __init_array_end - __init_array_start;
        for (size_t i = 0; i < count; i++) {
            if (__init_array_start[i]) {
                __init_array_start[i]();
            }
        }
    }
}

static void call_fini_array(void) {
    if (__fini_array_start && __fini_array_end) {
        size_t count = __fini_array_end - __fini_array_start;
        for (size_t i = count; i > 0; i--) {
            if (__fini_array_start[i - 1]) {
                __fini_array_start[i - 1]();
            }
        }
    }
}

__attribute__((noreturn))
void _start_c(int64_t *sp) {
    int argc = 1;
    char *default_argv[] = { "atoms_app", NULL };
    char **argv = default_argv;
    char *default_envp[] = { "PATH=/bin", "OS=ATOMS", NULL };
    char **envp = default_envp;

    if (sp != NULL && (uint64_t)sp >= 0x40000000ULL) {
        argc = (int)sp[0];
        if (argc > 0 && argc < 1024) {
            argv = (char **)&sp[1];
            envp = &argv[argc + 1];
        } else {
            argc = 1;
        }
    }

    /* 1. Initialize userspace heap allocator */
    atoms_heap_init();

    /* 2. Invoke static global constructors (.init_array) */
    call_init_array();

    /* 3. Execute application main entry point */
    int exit_code = main(argc, argv, envp);

    /* 4. Invoke static global destructors (.fini_array) */
    call_fini_array();

    /* 5. Clean exit via SYS_EXIT */
    atoms_sys_exit(exit_code);
}

__attribute__((naked, noreturn))
void _start(void) {
    __asm__ volatile (
        "xor %%rbp, %%rbp\n"         /* Clear frame pointer for stack traces */
        "mov %%rsp, %%rdi\n"         /* Pass original stack pointer as first argument (sp) */
        "and $-16, %%rsp\n"          /* 16-byte stack alignment per System V ABI */
        "call _start_c\n"            /* Enter C runtime startup (call pushes 8 bytes) */
        "hlt\n"                      /* Safety trap */
        ::: "memory"
    );
}
