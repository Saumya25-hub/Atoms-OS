#include "dispatcher_router.h"
#include "dispatcher_consumers.h"
#include "dispatcher_filters.h"
#include "dispatcher_diag.h"
#include "kernel/debug/step14_telemetry.h"

void dispatcher_router_init(void) {
    // Stateless initialization
}

void dispatcher_router_route(const DispatcherEvent* event) {
    if (!event) return;
    extern void display_print(const char*);
    display_print("[INPUT TRACE] Dispatcher\n");

    // Stage 1: Evaluate modular filters
    if (!dispatcher_filters_evaluate(event)) {
        return; // Dropped by filter
    }

    // Stage 2: Priority-ordered consumer traversal
    uint64_t start_tsc = step14_rdtsc();
    uint32_t count = dispatcher_consumers_get_count();

    for (uint32_t i = 0; i < count; i++) {
        const DispatcherConsumer* c = dispatcher_consumers_get_by_index(i);
        if (!c || !c->enabled || !c->callback) {
            continue;
        }

        // Check if consumer is interested in this event type
        if (!(c->event_type_mask & (1 << event->type))) {
            continue;
        }

        // Deliver immutable event to consumer callback
        DispatchResult res = c->callback(event, c->context);

        // Check propagation control
        if (res == DISPATCH_CONSUME) {
            dispatcher_diag_record_stop();
            break; // Stop propagation to lower priority tiers
        }
    }

    uint64_t cycles_spent = step14_rdtsc() - start_tsc;
    dispatcher_diag_record_routed(cycles_spent);
}
