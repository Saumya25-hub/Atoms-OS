#include "kernel/core/interrupt/include/exception.h"
#include "kernel/core/interrupt/include/isr.h"
#include "kernel/debug/abde/abde.h"
#include <stdbool.h>

extern uint32_t atoms_cpu_id(void);

static const char *exception_names[32] = {
    "#DE Divide Error",
    "#DB Debug Exception",
    "#NMI Non-Maskable Interrupt",
    "#BP Breakpoint Exception",
    "#OF Overflow Exception",
    "#BR BOUND Range Exceeded",
    "#UD Invalid Opcode",
    "#NM Device Not Available",
    "#DF Double Fault",
    "#CPSO Coprocessor Segment Overrun",
    "#TS Invalid TSS Exception",
    "#NP Segment Not Present",
    "#SS Stack Fault Exception",
    "#GP General Protection Fault",
    "#PF Page Fault",
    "#RES Reserved Exception",
    "#MF x87 FPU Floating-Point Error",
    "#AC Alignment Check Exception",
    "#MC Machine Check Exception",
    "#XM SIMD Floating-Point Exception",
    "#VE Virtualization Exception",
    "#CP Control Protection Exception",
    "#RES Reserved", "#RES Reserved", "#RES Reserved", "#RES Reserved",
    "#RES Reserved", "#RES Reserved", "#RES Reserved", "#RES Reserved",
    "#RES Reserved", "#RES Reserved"
};

// Common Forensic Exception Dispatcher
static uint64_t exception_dispatch(registers_t *regs) {
    uint32_t cpu = atoms_cpu_id();
    uint64_t cr2_val = 0;
    if (regs->int_no == 14) {
        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2_val));
    }

    const char *name = "#EX Unknown Exception";
    if (regs->int_no < 32) {
        name = exception_names[regs->int_no];
    }

    diag_set_fail("IDT");
    diag_set_step(name);
    diag_set_error(name);

    char detail_buf[64] = "RIP: 0x";
    // Convert RIP hex
    uint64_t val = regs->rip;
    const char hex_chars[] = "0123456789ABCDEF";
    int pos = 7;
    for (int i = 15; i >= 0; i--) {
        detail_buf[pos + i] = hex_chars[(val >> ((15 - i) * 4)) & 0xF];
    }
    detail_buf[23] = '\0';

    diag_set_fault(name, detail_buf);

    diag_set_idt_telemetry(256, g_abde.idt_base, 256, true, name, g_abde.fault_count + 1);

    // Active Forensic Ticker Loop so photo can be taken safely
    for (;;) {
        diag_heartbeat_tick();
        for (volatile int i = 0; i < 5000000; i++) {
            __asm__ __volatile__("nop");
        }
    }
    return 0;
}

void exception_init(void) {
    diag_set_step("ARM EXCEPTION HANDLERS");
    for (int i = 0; i < 32; i++) {
        isr_register_handler(i, exception_dispatch);
    }
    diag_set_idt_telemetry(256, g_abde.idt_base, 256, true, "ARMED 0-31", 0);
    diag_set_step("EXCEPTIONS ARMED 100%");
}

void kernel_panic_assert(const char *file, int line, const char *func) {
    (void)file; (void)line; (void)func;
    diag_set_fail("ASSERT");
    diag_set_step("ASSERT FAILED");
    diag_set_fault("ASSERT_FAIL", file ? file : "UNKNOWN");
    for (;;) {
        diag_heartbeat_tick();
        for (volatile int i = 0; i < 5000000; i++) {
            __asm__ __volatile__("nop");
        }
    }
}
