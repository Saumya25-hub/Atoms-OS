#include "horse_engine.h"

/* External debug print (stub for now) */
extern void debug_print(const char* msg);

void horse_init(void) {
    debug_print("[Horse] Engine Initialized\n");
}

void horse_dispatch(void) {
    /* 
     * Core run loop stub.
     * In the future, this will handle fast lookups and resource balancing.
     */
}

void horse_search(const char* query) {
    debug_print("[Horse] Search Request\n");
    /* TODO: Intercept and query indexed VFS metadata */
}

void horse_launch(uint32_t app_id) {
    /* 
     * Start Menu calls this. 
     * UI remains completely decoupled from actual process spawning.
     */
    debug_print("[Horse] Launch Request\n");
    
    // TODO: Determine binary path from app_id
    // TODO: Apply Resource Management limits
    // TODO: Hand over to Scheduler for execution
    
    debug_print("[Horse] Application Started\n");
}
