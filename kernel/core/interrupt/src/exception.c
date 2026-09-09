#include "kernel/core/interrupt/include/exception.h"
#include "kernel/core/interrupt/include/isr.h"
#include "kernel/core/scheduler/include/task.h"
#include "kernel/core/scheduler/include/scheduler.h"
#include "kernel/core/scheduler/include/context.h"
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

    if ((regs->cs & 0x03) != 0x03) {
        diag_set_fail("IDT");
        diag_set_step(name);
        diag_set_error(name);
    }

    uint64_t cr3_val = 0;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3_val));

    const char hex_chars[] = "0123456789ABCDEF";
    static char formatted[128];
    int idx = 0;
    // RIP
    formatted[idx++] = 'R'; formatted[idx++] = 'I'; formatted[idx++] = 'P'; formatted[idx++] = ':';
    for (int i = 15; i >= 0; i--) formatted[idx++] = hex_chars[(regs->rip >> (i * 4)) & 0xF];
    formatted[idx++] = ' ';
    // CR2
    formatted[idx++] = 'C'; formatted[idx++] = 'R'; formatted[idx++] = '2'; formatted[idx++] = ':';
    for (int i = 15; i >= 0; i--) formatted[idx++] = hex_chars[(cr2_val >> (i * 4)) & 0xF];
    formatted[idx++] = ' ';
    // ERR
    formatted[idx++] = 'E'; formatted[idx++] = 'R'; formatted[idx++] = 'R'; formatted[idx++] = ':';
    for (int i = 7; i >= 0; i--) formatted[idx++] = hex_chars[(regs->err_code >> (i * 4)) & 0xF];
    formatted[idx] = '\0';

    if ((regs->cs & 0x03) != 0x03) {
        diag_set_fault(name, formatted);
        diag_set_idt_telemetry(256, g_abde.idt_base, 256, true, name, g_abde.fault_count + 1);
    }

    extern void com1_puts(const char *s);
    char hx[] = "0123456789ABCDEF";
    com1_puts("\r\n========================================\r\n");
    if ((regs->cs & 0x03) == 0x03) {
        com1_puts("[USERMODE FAULT (CPL 3)]\r\n");
    } else {
        com1_puts("[KERNEL FAULT (CPL 0)]\r\n");
    }
    com1_puts("Vector     : "); com1_puts(name); com1_puts("\r\n");
    com1_puts("Error Code : 0x");
    for (int i = 60; i >= 0; i -= 4) { char c[2] = { hx[(regs->err_code >> i) & 0xF], '\0' }; com1_puts(c); }
    com1_puts("\r\nCPU ID     : ");
    { char c[2] = { (char)('0' + (cpu % 10)), '\0' }; com1_puts(c); }
    com1_puts("\r\nRIP        : 0x");
    for (int i = 60; i >= 0; i -= 4) { char c[2] = { hx[(regs->rip >> i) & 0xF], '\0' }; com1_puts(c); }
    com1_puts("\r\nRSP        : 0x");
    for (int i = 60; i >= 0; i -= 4) { char c[2] = { hx[(regs->rsp >> i) & 0xF], '\0' }; com1_puts(c); }
    com1_puts("\r\nRAX        : 0x");
    for (int i = 60; i >= 0; i -= 4) { char c[2] = { hx[(regs->rax >> i) & 0xF], '\0' }; com1_puts(c); }
    com1_puts("\r\nRBX        : 0x");
    for (int i = 60; i >= 0; i -= 4) { char c[2] = { hx[(regs->rbx >> i) & 0xF], '\0' }; com1_puts(c); }
    com1_puts("\r\nRCX        : 0x");
    for (int i = 60; i >= 0; i -= 4) { char c[2] = { hx[(regs->rcx >> i) & 0xF], '\0' }; com1_puts(c); }
    com1_puts("\r\nRDX        : 0x");
    for (int i = 60; i >= 0; i -= 4) { char c[2] = { hx[(regs->rdx >> i) & 0xF], '\0' }; com1_puts(c); }
    com1_puts("\r\nRSI        : 0x");
    for (int i = 60; i >= 0; i -= 4) { char c[2] = { hx[(regs->rsi >> i) & 0xF], '\0' }; com1_puts(c); }
    com1_puts("\r\nRDI        : 0x");
    for (int i = 60; i >= 0; i -= 4) { char c[2] = { hx[(regs->rdi >> i) & 0xF], '\0' }; com1_puts(c); }
    com1_puts("\r\nRBP        : 0x");
    for (int i = 60; i >= 0; i -= 4) { char c[2] = { hx[(regs->rbp >> i) & 0xF], '\0' }; com1_puts(c); }
    com1_puts("\r\nCS         : 0x");
    for (int i = 60; i >= 0; i -= 4) { char c[2] = { hx[(regs->cs >> i) & 0xF], '\0' }; com1_puts(c); }
    com1_puts("\r\nSS         : 0x");
    for (int i = 60; i >= 0; i -= 4) { char c[2] = { hx[(regs->ss >> i) & 0xF], '\0' }; com1_puts(c); }
    com1_puts("\r\nRFLAGS     : 0x");
    for (int i = 60; i >= 0; i -= 4) { char c[2] = { hx[(regs->rflags >> i) & 0xF], '\0' }; com1_puts(c); }
    com1_puts("\r\nCR2 (Fault): 0x");
    for (int i = 60; i >= 0; i -= 4) { char c[2] = { hx[(cr2_val >> i) & 0xF], '\0' }; com1_puts(c); }
    com1_puts("\r\nCR3 (PML4) : 0x");
    for (int i = 60; i >= 0; i -= 4) { char c[2] = { hx[(cr3_val >> i) & 0xF], '\0' }; com1_puts(c); }
    extern uint64_t vmm_translate(void *pml4, uint64_t virt_addr);
    uint64_t rip_phys = vmm_translate((void*)cr3_val, regs->rip);
    com1_puts("\r\nRIP_PHYS   : 0x");
    for (int i = 60; i >= 0; i -= 4) { char c[2] = { hx[(rip_phys >> i) & 0xF], '\0' }; com1_puts(c); }
    if (rip_phys && rip_phys < 0x100000000ULL) {
        com1_puts(" [BYTES:");
        uint8_t *code = (uint8_t*)rip_phys;
        for (int b = 0; b < 10; b++) {
            char cb[4] = { ' ', hx[(code[b] >> 4) & 0xF], hx[code[b] & 0xF], '\0' };
            com1_puts(cb);
        }
        com1_puts(" ]");
    }
    com1_puts("\r\n");

    if (regs->int_no == 14) {
        uint64_t cr4_val = 0, efer_val = 0;
        __asm__ volatile("mov %%cr4, %0" : "=r"(cr4_val));
        uint32_t efer_lo = 0, efer_hi = 0;
        __asm__ volatile("rdmsr" : "=a"(efer_lo), "=d"(efer_hi) : "c"(0xC0000080));
        efer_val = ((uint64_t)efer_hi << 32) | efer_lo;

        com1_puts("CR4        : 0x");
        for (int i = 60; i >= 0; i -= 4) { char c[2] = { hx[(cr4_val >> i) & 0xF], '\0' }; com1_puts(c); }
        com1_puts("\r\nEFER       : 0x");
        for (int i = 60; i >= 0; i -= 4) { char c[2] = { hx[(efer_val >> i) & 0xF], '\0' }; com1_puts(c); }
        com1_puts("\r\n");

        uint64_t fault_va = cr2_val;
        uint64_t pml4_idx = (fault_va >> 39) & 0x1FF;
        uint64_t pdp_idx  = (fault_va >> 30) & 0x1FF;
        uint64_t pd_idx   = (fault_va >> 21) & 0x1FF;
        uint64_t pt_idx   = (fault_va >> 12) & 0x1FF;
        uint64_t offset   = fault_va & 0xFFF;

        com1_puts("[FAULT_PAGEWALK]\r\n");
        com1_puts("  VA: 0x");
        for (int i = 60; i >= 0; i -= 4) { char c[2] = { hx[(fault_va >> i) & 0xF], '\0' }; com1_puts(c); }
        com1_puts(" (PML4_IDX=");
        { char b[8]; int p = 6; b[7] = '\0'; uint64_t v = pml4_idx; if (v==0) com1_puts("0"); else { while(v>0){b[p--]='0'+(v%10); v/=10;} com1_puts(&b[p+1]); } }
        com1_puts(" PDP_IDX=");
        { char b[8]; int p = 6; b[7] = '\0'; uint64_t v = pdp_idx; if (v==0) com1_puts("0"); else { while(v>0){b[p--]='0'+(v%10); v/=10;} com1_puts(&b[p+1]); } }
        com1_puts(" PD_IDX=");
        { char b[8]; int p = 6; b[7] = '\0'; uint64_t v = pd_idx; if (v==0) com1_puts("0"); else { while(v>0){b[p--]='0'+(v%10); v/=10;} com1_puts(&b[p+1]); } }
        com1_puts(" PT_IDX=");
        { char b[8]; int p = 6; b[7] = '\0'; uint64_t v = pt_idx; if (v==0) com1_puts("0"); else { while(v>0){b[p--]='0'+(v%10); v/=10;} com1_puts(&b[p+1]); } }
        com1_puts(")\r\n");

        uint64_t* pml4_tbl = (uint64_t*)(cr3_val & 0x000FFFFFFFFFF000ULL);
        uint64_t pml4e = pml4_tbl[pml4_idx];
        com1_puts("  PML4E: 0x");
        for (int i = 60; i >= 0; i -= 4) { char c[2] = { hx[(pml4e >> i) & 0xF], '\0' }; com1_puts(c); }
        com1_puts(" (P="); com1_puts((pml4e & 1) ? "1" : "0");
        com1_puts(" W="); com1_puts((pml4e & 2) ? "1" : "0");
        com1_puts(" U="); com1_puts((pml4e & 4) ? "1" : "0");
        com1_puts(")\r\n");

        if (pml4e & 1) {
            uint64_t pdp_phys = pml4e & 0x000FFFFFFFFFF000ULL;
            if (pdp_phys < 0x100000000ULL) {
                uint64_t* pdp_tbl = (uint64_t*)pdp_phys;
                uint64_t pdpe = pdp_tbl[pdp_idx];
                com1_puts("  PDPE:  0x");
                for (int i = 60; i >= 0; i -= 4) { char c[2] = { hx[(pdpe >> i) & 0xF], '\0' }; com1_puts(c); }
                com1_puts(" (P="); com1_puts((pdpe & 1) ? "1" : "0");
                com1_puts(" W="); com1_puts((pdpe & 2) ? "1" : "0");
                com1_puts(" U="); com1_puts((pdpe & 4) ? "1" : "0");
                com1_puts(")\r\n");

                if (pdpe & 1) {
                    if (pdpe & 0x80) {
                        com1_puts("  -> 1GB Huge Page!\r\n");
                    } else {
                        uint64_t pd_phys = pdpe & 0x000FFFFFFFFFF000ULL;
                        if (pd_phys < 0x100000000ULL) {
                            uint64_t* pd_tbl = (uint64_t*)pd_phys;
                            uint64_t pde = pd_tbl[pd_idx];
                            com1_puts("  PDE:   0x");
                            for (int i = 60; i >= 0; i -= 4) { char c[2] = { hx[(pde >> i) & 0xF], '\0' }; com1_puts(c); }
                            com1_puts(" (P="); com1_puts((pde & 1) ? "1" : "0");
                            com1_puts(" W="); com1_puts((pde & 2) ? "1" : "0");
                            com1_puts(" U="); com1_puts((pde & 4) ? "1" : "0");
                            com1_puts(")\r\n");

                            if (pde & 1) {
                                if (pde & 0x80) {
                                    com1_puts("  -> 2MB Huge Page!\r\n");
                                } else {
                                    uint64_t pt_phys = pde & 0x000FFFFFFFFFF000ULL;
                                    if (pt_phys < 0x100000000ULL) {
                                        uint64_t* pt_tbl = (uint64_t*)pt_phys;
                                        uint64_t pte = pt_tbl[pt_idx];
                                        com1_puts("  PTE:   0x");
                                        for (int i = 60; i >= 0; i -= 4) { char c[2] = { hx[(pte >> i) & 0xF], '\0' }; com1_puts(c); }
                                        com1_puts(" (P="); com1_puts((pte & 1) ? "1" : "0");
                                        com1_puts(" W="); com1_puts((pte & 2) ? "1" : "0");
                                        com1_puts(" U="); com1_puts((pte & 4) ? "1" : "0");
                                        com1_puts(" PHYS=0x");
                                        for (int i = 60; i >= 0; i -= 4) { char c[2] = { hx[((pte & 0x000FFFFFFFFFF000ULL) >> i) & 0xF], '\0' }; com1_puts(c); }
                                        com1_puts(")\r\n");
                                    } else {
                                        com1_puts("  PTE:   [High Physical Frame >= 4GB]\r\n");
                                    }
                                }
                            }
                        } else {
                            com1_puts("  PDE:   [High Physical Frame >= 4GB]\r\n");
                        }
                    }
                }
            } else {
                com1_puts("  PDPE:  [High Physical Frame >= 4GB]\r\n");
            }
        }
    }
    com1_puts("========================================\r\n");

    extern void debuglan_log_subsys(const char* subsys, const char* fmt, ...);
    extern void debuglan_flush(void);
    extern bool atoms_screenshot_capture_sync(uint32_t session_id);
    extern void atoms_trace_dump_to_lan(void);

    debuglan_log_subsys("CRASH", "=== FAULT DETECTED: %s (Vector %u, Code 0x%llx) CPL=%u CPU=%u ===",
                        name, (uint32_t)regs->int_no, (uint64_t)regs->err_code, (uint32_t)(regs->cs & 3), cpu);
    debuglan_log_subsys("CRASH", "RIP=0x%016llx RSP=0x%016llx RFLAGS=0x%016llx CR2=0x%016llx CR3=0x%016llx",
                        (uint64_t)regs->rip, (uint64_t)regs->rsp, (uint64_t)regs->rflags, (uint64_t)cr2_val, (uint64_t)cr3_val);
    debuglan_log_subsys("CRASH", "RAX=0x%016llx RBX=0x%016llx RCX=0x%016llx RDX=0x%016llx",
                        (uint64_t)regs->rax, (uint64_t)regs->rbx, (uint64_t)regs->rcx, (uint64_t)regs->rdx);
    debuglan_log_subsys("CRASH", "RSI=0x%016llx RDI=0x%016llx RBP=0x%016llx",
                        (uint64_t)regs->rsi, (uint64_t)regs->rdi, (uint64_t)regs->rbp);
    debuglan_flush();

    // User-Mode Fault Containment: Terminate the faulting process cleanly and keep OS alive
    if ((regs->cs & 0x03) == 0x03) {
        com1_puts("[USERMODE FAULT CONTAINMENT] Terminating faulting Ring 3 process.\r\n");
        debuglan_log_subsys("CRASH_USER", "Ring 3 fault vector %u handled cleanly by containment", (uint32_t)regs->int_no);
        debuglan_flush();
        Task *cur = scheduler_current_task();
        if (cur) {
            if (cur->owner_pid) {
                extern bool ATOMS_Process_Terminate(uint32_t pid, int32_t exit_code);
                ATOMS_Process_Terminate(cur->owner_pid, -(int32_t)regs->int_no);
            }
            scheduler_terminate_task(cur);
        }
        scheduler_on_tick();
        Task *next = scheduler_current_task();
        uint64_t next_rsp = next ? context_restore_state(next) : 0;
        return next_rsp;
    }

    // Kernel-Mode Panic: Stream pre-panic framebuffer and dump rolling events over LAN
    atoms_screenshot_capture_sync(0xDEAD0000 | (uint32_t)(regs->int_no & 0xFF));
    atoms_trace_dump_to_lan();

    // Kernel-Mode Panic Loop (CPL 0 only)
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
    extern void debuglan_log_subsys(const char* subsys, const char* fmt, ...);
    extern void debuglan_flush(void);
    extern bool atoms_screenshot_capture_sync(uint32_t session_id);
    extern void atoms_trace_dump_to_lan(void);

    debuglan_log_subsys("PANIC", "ASSERT FAILED at %s:%d (%s)", file ? file : "UNKNOWN", line, func ? func : "UNKNOWN");
    debuglan_flush();

    diag_set_fail("ASSERT");
    diag_set_step("ASSERT FAILED");
    diag_set_fault("ASSERT_FAIL", file ? file : "UNKNOWN");

    atoms_screenshot_capture_sync(0xDEADA557);
    atoms_trace_dump_to_lan();

    for (;;) {
        diag_heartbeat_tick();
        for (volatile int i = 0; i < 5000000; i++) {
            __asm__ __volatile__("nop");
        }
    }
}
