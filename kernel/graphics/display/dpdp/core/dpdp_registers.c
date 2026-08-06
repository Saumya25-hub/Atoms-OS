#include "kernel/graphics/display/dpdp/include/dpdp_registers.h"

extern void display_print(const char* str);
extern void display_print_dec(uint64_t val);
extern void display_print_hex(uint64_t val);

void dpdp_capture_gpu_registers(dpdp_register_watch_t* regs) {
    if (!regs) return;

    regs->width = 1920;
    regs->height = 1080;
    regs->enable = 1;
    regs->sync = 1;
    regs->busy = 0;
    regs->bytes_per_line = 7680;
    regs->fifo_next_cmd = 0x00001280;
    regs->fifo_stop = 0x00001280;
}
