#include "abe_js_objects.h"
#include "../diagnostics/abe_diagnostics.h"

extern void display_print(const char* s);

static bool g_js_objects_initialized = false;

ABE_Error ABE_JSObjects_Init(void) {
    g_js_objects_initialized = true;
    ABE_Log(ABE_LOG_INFO, "JSOBJECTS", "ABE JavaScript Standard Built-in Objects initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_JSObjects_Shutdown(void) {
    g_js_objects_initialized = false;
    return ABE_SUCCESS;
}

double ABE_JSMath_Floor(double v) {
    int i = (int)v;
    return (double)i;
}

double ABE_JSMath_Ceil(double v) {
    int i = (int)v;
    if ((double)i < v) i++;
    return (double)i;
}

double ABE_JSMath_Round(double v) {
    return ABE_JSMath_Floor(v + 0.5);
}

double ABE_JSMath_Max(double a, double b) {
    return (a >= b) ? a : b;
}

double ABE_JSMath_Min(double a, double b) {
    return (a <= b) ? a : b;
}

void ABE_JSConsole_Log(const char* msg) {
    display_print("[CONSOLE.LOG] ");
    display_print(msg ? msg : "");
    display_print("\n");
}
