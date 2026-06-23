#include "kernel/lib/include/crash_log.h"
#include "kernel/display/display.h"

static char crash_logs[CRASH_LOG_SIZE][CRASH_LOG_MSG_MAX];
static int crash_log_head = 0;
static int crash_log_count = 0;

static void strncpy_custom(char* dest, const char* src, int n) {
    int i;
    for (i = 0; i < n - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

void crash_log_add(const char* event) {
    strncpy_custom(crash_logs[crash_log_head], event, CRASH_LOG_MSG_MAX);
    crash_log_head = (crash_log_head + 1) % CRASH_LOG_SIZE;
    if (crash_log_count < CRASH_LOG_SIZE) {
        crash_log_count++;
    }
}

void crash_log_dump(void) {
    display_print("\n---- LAST EVENTS ----\n\n");
    
    if (crash_log_count == 0) {
        display_print("No events recorded.\n");
        return;
    }

    int start_idx = 0;
    if (crash_log_count == CRASH_LOG_SIZE) {
        start_idx = crash_log_head;
    }

    for (int i = 0; i < crash_log_count; i++) {
        int idx = (start_idx + i) % CRASH_LOG_SIZE;
        display_print(crash_logs[idx]);
        display_print("\n");
    }
}
