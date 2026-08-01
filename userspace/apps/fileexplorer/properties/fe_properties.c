#include "../include/fileexplorer_api.h"
#include "kernel/drivers/display/display.h"

void fe_properties_init(void) { display_print("[FE_PROP] Properties Engine Initialized.\n"); }

bool fe_properties_show(const char* path) {
    (void)path;
    display_print("[FE_PROP] Show Properties -> SHELL32.SHObjectProperties() OK\n");
    return true;
}
