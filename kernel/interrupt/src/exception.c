#include "kernel/interrupt/include/exception.h"
#include "kernel/interrupt/include/isr.h"
#include "kernel/display/display.h"

// Page Fault exception handler (Interrupt 14)
static void page_fault_handler(registers_t* regs) {
    uint64_t faulting_address;
    __asm__ volatile("mov %%cr2, %0" : "=r" (faulting_address));

    display_print("\n======================================================\n");
    display_print("             BOS KERNEL PANIC: PAGE FAULT             \n");
    display_print("======================================================\n");
    display_print("Faulting Virtual Address: "); display_print_hex(faulting_address); display_print("\n");
    display_print("Error Code: "); display_print_hex(regs->err_code); display_print("\n");

    display_print("\nFlags:\n");
    display_print(regs->err_code & 0x1 ? " - Protection Violation (Page Present)\n" : " - Non-Present Page\n");
    display_print(regs->err_code & 0x2 ? " - Write Operation\n" : " - Read Operation\n");
    display_print(regs->err_code & 0x4 ? " - User Mode\n" : " - Supervisor Mode\n");
    display_print(regs->err_code & 0x8 ? " - Reserved Bit Violation\n" : "");
    display_print(regs->err_code & 0x10 ? " - Instruction Fetch\n" : "");

    display_print("\nSystem Halted.\n");
    while (1) {
        __asm__ volatile("cli; hlt");
    }
}


static const char* exception_messages[32] = {
    "Divide By Zero",
    "Debug Exception",
    "Non Maskable Interrupt Exception",
    "Int 3 Exception",
    "INTO Exception",
    "Out of Bounds Exception",
    "Invalid Opcode",
    "Coprocessor Not Available Exception",
    "Double Fault",
    "Coprocessor Segment Overrun Exception",
    "Bad TSS Exception",
    "Segment Not Present Exception",
    "Stack Fault Exception",
    "General Protection Fault",
    "Page Fault",
    "Unknown Exception",
    "Floating Point Exception",
    "Alignment Check Exception",
    "Machine Check Exception",
    "SIMD Floating Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Unknown Exception",
    "Unknown Exception",
    "Unknown Exception",
    "Unknown Exception",
    "Unknown Exception",
    "Unknown Exception",
    "Unknown Exception",
    "Unknown Exception",
    "Unknown Exception",
    "Unknown Exception"
};

static void itoa_dec(uint64_t val, char* buf) {
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    
    char temp[20];
    int tpos = 0;
    while (val > 0) {
        temp[tpos++] = (val % 10) + '0';
        val /= 10;
    }
    
    int pos = 0;
    for (int i = tpos - 1; i >= 0; i--) {
        buf[pos++] = temp[i];
    }
    buf[pos] = '\0';
}

static void itoa_hex(uint64_t val, char* buf) {
    const char hex_chars[] = "0123456789ABCDEF";
    int pos = 0;
    
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    
    char temp[20];
    int tpos = 0;
    while (val > 0) {
        temp[tpos++] = hex_chars[val % 16];
        val /= 16;
    }
    
    for (int i = tpos - 1; i >= 0; i--) {
        buf[pos++] = temp[i];
    }
    buf[pos] = '\0';
}

// Reusable BOS Panic Routine
static void exception_dispatch(registers_t* regs) {
    // 1. Disable Interrupts
    __asm__ volatile("cli");

    // 2. Set Panic Colors (Red Background, White Text)
    // VGA attributes: 0x4 = Red, 0xF = White
    display_set_color(0x4F);
    display_clear();

    // 3. Display Panic Screen
    display_print("========================================\n\n");
    display_print("        BOS KERNEL PANIC\n\n");
    display_print("========================================\n\n");

    display_print("Exception\n\n");
    if (regs->int_no < 32) {
        display_print(exception_messages[regs->int_no]);
    } else {
        display_print("Unknown Exception");
    }
    display_print("\n\nVector\n\n");

    char vec_str[10];
    itoa_dec(regs->int_no, vec_str);
    display_print(vec_str);
    display_print("\n\nRIP\n\n0x");

    char rip_str[20];
    itoa_hex(regs->rip, rip_str);
    display_print(rip_str);
    display_print("\n\nSystem Halted\n\n");
    display_print("========================================\n");

    // 4. Halt Forever
    while (1) {
        __asm__ volatile("hlt");
    }
}

void exception_init(void) {
    // Register exception_dispatch for vectors 0 to 31
    for (int i = 0; i < 32; i++) {
        isr_register_handler(i, exception_dispatch);
    }

    // Phase 11: Register dedicated Page Fault handler
    isr_register_handler(14, page_fault_handler);
}
