#include "adf_core.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void display_print(const char* s);
extern void display_print_dec(uint32_t val);
extern uint64_t timer_get_ticks(void);

static bool           g_adf_enabled = true;
static ABE_DebugLevel g_adf_level = ABE_DEBUG_INFO;
static bool           g_subsystem_toggles[ABE_SUB_COUNT];

static ABE_LogEntry s_ring_buffer[ABE_RING_BUFFER_SIZE];
static uint32_t     s_ring_head = 0;
static uint32_t     s_ring_count = 0;

static const char* s_subsystem_names[ABE_SUB_COUNT] = {
    "HTML", "CSS", "LAYOUT", "PAINT", "GPU", "CANVAS", "SVG", "WEBGL",
    "WASM", "JS", "NETWORKING", "STORAGE", "MEMORY", "SCHEDULER", "OBSERVER"
};

void ABE_DebugInitialize(void) {
    g_adf_enabled = true;
    g_adf_level = ABE_DEBUG_INFO;
    for (int i = 0; i < ABE_SUB_COUNT; i++) {
        g_subsystem_toggles[i] = true;
    }
    s_ring_head = 0;
    s_ring_count = 0;
    display_print("[ADF] ABE Production Debug & Diagnostics Framework Initialized!\n");
}

void ABE_DebugEnable(void) { g_adf_enabled = true; }
void ABE_DebugDisable(void) { g_adf_enabled = false; }
void ABE_DebugSetLevel(ABE_DebugLevel level) { g_adf_level = level; }

void ABE_DebugEnableSubsystem(ABE_SubsystemID sub) {
    if (sub < ABE_SUB_COUNT) g_subsystem_toggles[sub] = true;
}

void ABE_DebugDisableSubsystem(ABE_SubsystemID sub) {
    if (sub < ABE_SUB_COUNT) g_subsystem_toggles[sub] = false;
}

void ABE_DebugLog(ABE_SubsystemID sub, ABE_DebugLevel severity, const char* msg) {
    if (!g_adf_enabled || !msg) return;
    if (severity > g_adf_level) return;
    if (sub < ABE_SUB_COUNT && !g_subsystem_toggles[sub]) return;

    uint32_t idx = (s_ring_head + s_ring_count) % ABE_RING_BUFFER_SIZE;
    if (s_ring_count == ABE_RING_BUFFER_SIZE) {
        s_ring_head = (s_ring_head + 1) % ABE_RING_BUFFER_SIZE;
    } else {
        s_ring_count++;
    }

    ABE_LogEntry* entry = &s_ring_buffer[idx];
    entry->timestamp = timer_get_ticks();
    entry->subsystem = sub;
    entry->thread_id = 1;
    entry->severity = severity;
    strncpy(entry->message, msg, sizeof(entry->message) - 1);
}

void ABE_DebugDumpLog(void) {
    display_print("\n=========================================================\n");
    display_print(" [ADF] Ring Buffer Debug Log Dump (Entries: ");
    display_print_dec(s_ring_count);
    display_print(")\n");
    display_print("=========================================================\n");

    for (uint32_t i = 0; i < s_ring_count; i++) {
        uint32_t idx = (s_ring_head + i) % ABE_RING_BUFFER_SIZE;
        ABE_LogEntry* e = &s_ring_buffer[idx];
        display_print("[");
        if (e->subsystem < ABE_SUB_COUNT) {
            display_print(s_subsystem_names[e->subsystem]);
        } else {
            display_print("UNKNOWN");
        }
        display_print("] ");
        display_print(e->message);
        display_print("\n");
    }
    display_print("=========================================================\n\n");
}
