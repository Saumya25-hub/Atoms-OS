#include "kernel/core/interrupt/include/exception.h"
#include "kernel/core/interrupt/include/isr.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/scheduler/include/task.h"
#include "kernel/core/scheduler/include/scheduler.h"
#include "kernel/core/lib/include/crash_log.h"
#include <stdbool.h>

// Page Fault exception handler (Interrupt 14)
static uint64_t page_fault_handler(registers_t* regs) {
    uint64_t faulting_address;
    __asm__ volatile("mov %%cr2, %0" : "=r" (faulting_address));

    // Check if the fault occurred in Ring 3
    if ((regs->cs & 3) == 3) {
        display_print("\n[USER FAULT] Page Fault at RIP 0x");
        display_print_hex(regs->rip);
        display_print(" accessing 0x");
        display_print_hex(faulting_address);
        
        Task* current = scheduler_current_task();
        if (current) {
            display_print("\nPID "); display_print_dec(current->id);
            display_print(" "); display_print(current->name);
            display_print(" terminated safely.\n");
            task_transition(current, TASK_TERMINATED);
        }
        
        extern void scheduler_on_tick(void);
        scheduler_on_tick();
        Task* next = scheduler_current_task();
        if (next) {
            return next->rsp;
        }
        while(1) { __asm__ volatile("cli; hlt"); }
    }

    display_print("\n======================================================\n");
    display_print("             BOS KERNEL PANIC: PAGE FAULT             \n");
    display_print("======================================================\n");
    display_print("Faulting Virtual Address: 0x"); display_print_hex(faulting_address); display_print("\n");
    display_print("Error Code: 0x"); display_print_hex(regs->err_code); display_print("\n");

    Task* current = scheduler_current_task();
    if (current) {
        display_print("\nPID       : "); display_print_dec(current->id); display_print("\n");
        display_print("Task Name : "); display_print(current->name); display_print("\n");
    }

    uint64_t cr3_val;
    __asm__ volatile("mov %%cr3, %0" : "=r" (cr3_val));
    display_print("\nRIP       : 0x"); display_print_hex(regs->rip); display_print("\n");
    display_print("RSP       : 0x"); display_print_hex(regs->rsp); display_print("\n");
    display_print("CR3       : 0x"); display_print_hex(cr3_val); display_print("\n");

    display_print("\n[PAGE FAULT]\nPossible Causes:\n");
    display_print(regs->err_code & 0x1 ? " - Protection Violation (Page Present)\n" : " - Non-Present Page\n");
    display_print(regs->err_code & 0x2 ? " - Write Operation on Read-Only Page\n" : " - Read Operation on Invalid Page\n");
    display_print(regs->err_code & 0x4 ? " - Access from User Mode to Supervisor Page\n" : " - Supervisor Mode Access\n");
    display_print(regs->err_code & 0x8 ? " - Reserved Bit Violation in Page Table\n" : "");
    display_print(regs->err_code & 0x10 ? " - Instruction Fetch Violation\n" : "");
    
    // crash_log_dump();
    
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
static uint64_t exception_dispatch(registers_t* regs) {
    // 1. Disable Interrupts
    __asm__ volatile("cli");

    // 2. Set Panic Colors (Red Background, White Text)
    // VGA attributes: 0x4 = Red, 0xF = White
    display_set_color(0x4F);
    display_clear();

    // 3. Display Panic Screen
    display_print("======================================================\n");
    display_print("                  BOS KERNEL PANIC\n");
    display_print("======================================================\n\n");

    display_print("Exception : ");
    char vec_str[10];
    itoa_dec(regs->int_no, vec_str);
    display_print(vec_str);
    display_print(" (");
    if (regs->int_no < 32) {
        display_print(exception_messages[regs->int_no]);
    } else {
        display_print("Unknown Exception");
    }
    display_print(")\n\n");

    Task* current = scheduler_current_task();
    if (current) {
        display_print("PID       : "); display_print_dec(current->id); display_print("\n");
        display_print("Task Name : "); display_print(current->name); display_print("\n\n");
    }

    uint64_t cr3_val;
    __asm__ volatile("mov %%cr3, %0" : "=r" (cr3_val));
    char temp_str[20];
    
    display_print("RIP       : 0x"); itoa_hex(regs->rip, temp_str); display_print(temp_str); display_print("\n");
    display_print("RSP       : 0x"); itoa_hex(regs->rsp, temp_str); display_print(temp_str); display_print("\n");
    display_print("CR3       : 0x"); itoa_hex(cr3_val, temp_str); display_print(temp_str); display_print("\n\n");

    display_print("Error Code: 0x"); itoa_hex(regs->err_code, temp_str); display_print(temp_str); display_print("\n");
    
    // crash_log_dump();

    display_print("\nSystem Halted.\n");
    display_print("======================================================\n");

    // 4. Halt Forever
    while (1) {
        __asm__ volatile("hlt");
    }
}

static uint64_t gpf_handler(registers_t* regs) {
    // Check if the fault occurred in Ring 3
    if ((regs->cs & 3) == 3) {
        display_print("\n[USER FAULT] GPF at RIP 0x");
        display_print_hex(regs->rip);
        
        extern void scheduler_on_tick(void);
        
        Task* current = scheduler_current_task();
        if (current) {
            display_print("\nPID "); display_print_dec(current->id);
            display_print(" "); display_print(current->name);
            display_print(" terminated safely.\n");
            task_transition(current, TASK_TERMINATED);
        }
        
        // Pick a new task
        scheduler_on_tick();
        
        Task* next = scheduler_current_task();
        if (next) {
            return next->rsp; // Return new RSP to assembly stub to perform context switch
        }
        
        // If no task (should not happen due to idle task), just halt
        while(1) { __asm__ volatile("cli; hlt"); }
    }
    
    // If it's a kernel GPF, we intercept it to print detailed possible causes before dispatch
    display_set_color(0x4F);
    display_clear();
    display_print("======================================================\n");
    display_print("                  BOS KERNEL PANIC\n");
    display_print("======================================================\n\n");
    display_print("Exception : 13 (General Protection Fault)\n\n");
    
    Task* current = scheduler_current_task();
    if (current) {
        display_print("PID       : "); display_print_dec(current->id); display_print("\n");
        display_print("Task Name : "); display_print(current->name); display_print("\n\n");
    }

    uint64_t cr3_val;
    __asm__ volatile("mov %%cr3, %0" : "=r" (cr3_val));
    char temp_str[20];
    display_print("RIP       : 0x"); itoa_hex(regs->rip, temp_str); display_print(temp_str); display_print("\n");
    display_print("RSP       : 0x"); itoa_hex(regs->rsp, temp_str); display_print(temp_str); display_print("\n");
    display_print("CR3       : 0x"); itoa_hex(cr3_val, temp_str); display_print(temp_str); display_print("\n\n");
    display_print("Error Code: 0x"); itoa_hex(regs->err_code, temp_str); display_print(temp_str); display_print("\n\n");
    
    display_print("[GPF] Possible Causes:\n");
    display_print("- Invalid Segment Descriptor\n");
    display_print("- Bad Privilege Transition (Executing CLI in Ring 3)\n");
    display_print("- Corrupted Stack\n");
    display_print("- Invalid Return\n\n");
    
    // crash_log_dump();
    
    display_print("\nSystem Halted.\n");
    display_print("======================================================\n");
    while (1) {
        __asm__ volatile("hlt");
    }
    
    return 0;
}

void exception_init(void) {
    // Register exception_dispatch for vectors 0 to 31
    for (int i = 0; i < 32; i++) {
        isr_register_handler(i, exception_dispatch);
    }

    // Phase 19: Register dedicated GPF handler
    isr_register_handler(13, gpf_handler);
    
    // Phase 11: Register dedicated Page Fault handler
    isr_register_handler(14, page_fault_handler);
}

void kernel_panic_assert(const char* file, int line, const char* func) {
    __asm__ volatile("cli");
    display_set_color(0x4F);
    display_clear();
    display_print("======================================================\n");
    display_print("                  ASSERT FAILED\n");
    display_print("======================================================\n\n");
    
    display_print("File    : "); display_print(file); display_print("\n");
    display_print("Line    : "); display_print_dec(line); display_print("\n");
    display_print("Function: "); display_print(func); display_print("\n\n");
    
    crash_log_dump();
    
    display_print("\nSystem Halted.\n");
    display_print("======================================================\n");
    while (1) {
        __asm__ volatile("hlt");
    }
}
