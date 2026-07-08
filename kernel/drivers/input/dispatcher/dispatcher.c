#include "dispatcher.h"
#include "kernel/drivers/display/display.h"

void dispatcher_init(void) {
    display_print("[DISPATCHER] Initializing ATOMS OS Event Dispatcher (Phase 4)...\n");

    dispatcher_diag_init();
    dispatcher_queue_init();
    dispatcher_consumers_init();
    dispatcher_filters_init();
    dispatcher_router_init();

    display_print("[DISPATCHER] Event Dispatcher V2.0 Initialized Successfully.\n");
}

bool dispatcher_push_event(const DispatcherEvent* event) {
    return dispatcher_queue_push(event);
}

void dispatcher_pump_events(void) {
    DispatcherEvent ev;
    while (dispatcher_queue_pop(&ev)) {
        dispatcher_router_route(&ev);
    }
}
