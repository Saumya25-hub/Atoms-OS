#include "../include/usb_hub_events.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

static usb_hub_event_queue_t g_event_queue;

void usb_hub_events_init(void) {
    atoms_spinlock_init(&g_event_queue.lock, 0);
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_event_queue.lock);
    memset(&g_event_queue, 0, sizeof(usb_hub_event_queue_t));
    atoms_spin_unlock_irqrestore(&g_event_queue.lock, state);
    display_print("[UHE EVENTS] Hub Event Dispatcher Initialized.\n");
}

bool usb_hub_enqueue_event(usb_hub_event_type_t type, uint32_t hub_id, uint8_t port_num, usb_speed_t speed) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_event_queue.lock);
    if (g_event_queue.count >= USB_MAX_HUB_EVENTS) {
        atoms_spin_unlock_irqrestore(&g_event_queue.lock, state);
        return false;
    }
    
    usb_hub_event_t* ev = &g_event_queue.queue[g_event_queue.tail];
    ev->type = type;
    ev->hub_id = hub_id;
    ev->port_num = port_num;
    ev->speed = speed;
    ev->timestamp = 100;
    
    g_event_queue.tail = (g_event_queue.tail + 1) % USB_MAX_HUB_EVENTS;
    g_event_queue.count++;
    atoms_spin_unlock_irqrestore(&g_event_queue.lock, state);
    return true;
}

bool usb_hub_dequeue_event(usb_hub_event_t* out_event) {
    if (!out_event) return false;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_event_queue.lock);
    if (g_event_queue.count == 0) {
        atoms_spin_unlock_irqrestore(&g_event_queue.lock, state);
        return false;
    }
    
    *out_event = g_event_queue.queue[g_event_queue.head];
    g_event_queue.head = (g_event_queue.head + 1) % USB_MAX_HUB_EVENTS;
    g_event_queue.count--;
    g_event_queue.total_processed++;
    atoms_spin_unlock_irqrestore(&g_event_queue.lock, state);
    return true;
}

void usb_hub_process_events(void) {
    usb_hub_event_t ev;
    while (usb_hub_dequeue_event(&ev)) {
        display_print("[UHE EVENT DISPATCHER] Event Type: ");
        display_print_dec((uint32_t)ev.type);
        display_print(" Hub #");
        display_print_dec(ev.hub_id);
        display_print(" Port #");
        display_print_dec(ev.port_num);
        display_print("\n");
    }
}
