#include "loader_debug.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* s);
extern void display_print_hex(uint64_t val);

static loader_log_level_t g_current_log_level = LOG_LEVEL_WARN;
static char g_last_error_buffer[256] = "No error";

void loader_debug_init(loader_log_level_t level) {
    g_current_log_level = level;
}

void loader_debug_set_level(loader_log_level_t level) {
    g_current_log_level = level;
}

void loader_debug_log(loader_log_level_t level, const char* category, const char* message) {
    if (level > g_current_log_level || !message) return;

    if (level == LOG_LEVEL_ERROR) {
        uint32_t i = 0;
        while (message[i] && i < sizeof(g_last_error_buffer) - 1) {
            g_last_error_buffer[i] = message[i];
            i++;
        }
        g_last_error_buffer[i] = '\0';
    }

    display_print("[LOADER_DEBUG] [");
    if (category) display_print(category);
    else display_print("GENERAL");
    display_print("] ");
    display_print(message);
    display_print("\n");
}

void loader_debug_trace_symbol(const char* symbol_name, void* address, bool resolved) {
    if (g_current_log_level < LOG_LEVEL_VERBOSE) return;

    display_print("[LOADER_TRACE] Symbol: ");
    if (symbol_name) display_print(symbol_name);
    display_print(resolved ? " -> RESOLVED at 0x" : " -> UNRESOLVED (0x");
    display_print_hex((uint64_t)(uintptr_t)address);
    display_print(")\n");
}

void loader_debug_trace_reloc(uint32_t reloc_type, uint64_t target_addr, uint64_t val) {
    if (g_current_log_level < LOG_LEVEL_VERBOSE) return;

    display_print("[LOADER_TRACE] Reloc type ");
    display_print_hex((uint64_t)reloc_type);
    display_print(" at 0x");
    display_print_hex(target_addr);
    display_print(" value 0x");
    display_print_hex(val);
    display_print("\n");
}

void loader_debug_dump_memory_map(uint64_t vaddr, uint64_t size, uint32_t flags) {
    if (g_current_log_level < LOG_LEVEL_INFO) return;

    display_print("[LOADER_MMAP] VAddr: 0x");
    display_print_hex(vaddr);
    display_print(" Size: 0x");
    display_print_hex(size);
    display_print(" Flags: 0x");
    display_print_hex((uint64_t)flags);
    display_print("\n");
}

const char* bos_dlerror(void) {
    return g_last_error_buffer;
}
