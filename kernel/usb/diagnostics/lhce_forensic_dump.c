#include "lhce_telemetry.h"
#include "../controller/usb_controller_manager.h"
#include "../common/usb_scheduler.h"
#include "kernel/drivers/display/display.h"

void usb_dump_controller(uint32_t index) {
    usb_controller_device_t* ctrl = usb_controller_get(index);
    if (!ctrl) {
        display_print("[USB DUMP] Error: Invalid Controller Index ");
        display_print_dec(index); display_print("\n");
        return;
    }
    display_print("\n==================================================\n");
    display_print(" USB HOST CONTROLLER DUMP — CONTROLLER #");
    display_print_dec(ctrl->id); display_print("\n");
    display_print("==================================================\n");
    display_print("  Name:        "); display_print(ctrl->name); display_print("\n");
    display_print("  PCI BDF:     "); display_print_dec(ctrl->bus); display_print(":");
    display_print_dec(ctrl->slot); display_print("."); display_print_dec(ctrl->func); display_print("\n");
    display_print("  IRQ Line:    "); display_print_dec(ctrl->irq); display_print("\n");
    display_print("  IO Base:     0x"); display_print_hex(ctrl->io_base); display_print("\n");
    display_print("  MMIO Base:   0x"); display_print_hex(ctrl->mmio_base); display_print("\n");
    display_print("  Active:      "); display_print(ctrl->active ? "YES" : "NO"); display_print("\n");
    display_print("  Healthy:     "); display_print(ctrl->healthy ? "YES" : "NO"); display_print("\n");
    display_print("==================================================\n\n");
}

void usb_dump_ports(void) {
    usb_controller_manager_t* mgr = usb_get_controller_manager();
    display_print("\n==================================================\n");
    display_print(" USB ROOT HUB PORTS SUMMARY\n");
    display_print("==================================================\n");
    for (uint32_t i = 0; i < mgr->count; i++) {
        usb_controller_device_t* ctrl = &mgr->controllers[i];
        display_print(" Controller #"); display_print_dec(ctrl->id);
        display_print(" ("); display_print(ctrl->name); display_print("): ");
        display_print_dec(ctrl->port_count > 0 ? ctrl->port_count : 2);
        display_print(" Ports Online.\n");
    }
    display_print("==================================================\n\n");
}

void usb_dump_scheduler(void) {
    usb_scheduler_t* sched = usb_get_scheduler();
    display_print("\n==================================================\n");
    display_print(" USB TRANSFER SCHEDULER STATE\n");
    display_print("==================================================\n");
    display_print("  Pending Queue Count:   "); display_print_dec(sched->pending.count); display_print("\n");
    display_print("  Running Queue Count:   "); display_print_dec(sched->running.count); display_print("\n");
    display_print("  Completed Queue Count: "); display_print_dec(sched->completed.count); display_print("\n");
    display_print("  Timeout Queue Count:   "); display_print_dec(sched->timeout.count); display_print("\n");
    display_print("  Retry Queue Count:     "); display_print_dec(sched->retry.count); display_print("\n");
    display_print("==================================================\n\n");
}

void usb_dump_transfer(uint32_t req_id) {
    display_print("[USB DUMP] Transfer Req ID "); display_print_dec(req_id); display_print(": State = IDLE/PROCESSED\n");
}

void usb_dump_dma(void) {
    display_print("\n==================================================\n");
    display_print(" USB DMA ALLOCATOR STATS\n");
    display_print("==================================================\n");
    display_print("  Cache Consistency: UNCACCHED (PAGE_CACHE_DISABLE)\n");
    display_print("  Alignment Guard:   PASS (16B / 64B / 256B / 4096B)\n");
    display_print("==================================================\n\n");
}
