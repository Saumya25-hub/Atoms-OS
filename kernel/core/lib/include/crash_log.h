#pragma once

#define CRASH_LOG_SIZE 50
#define CRASH_LOG_MSG_MAX 64

// Add an event to the crash log buffer
void crash_log_add(const char* event);

// Dump the crash log to the display
void crash_log_dump(void);
