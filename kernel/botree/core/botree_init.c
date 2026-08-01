#include "../include/botree.h"
#include "kernel/drivers/display/display.h"

static bool s_botree_inited = false;

int32_t BDe_Init(void) {
    if (s_botree_inited) return 0;

    display_print("[BO-TREE] Initializing BO-TREE Engine V1.0 (Single Directory Authority)...\n");
    display_print("[BO-TREE] Path Engine, Navigation Engine, Namespace Engine & Transaction Engine Ready.\n");

    s_botree_inited = true;
    return 0;
}

void BDe_Shutdown(void) {
    if (!s_botree_inited) return;
    display_print("[BO-TREE] Shutting down BO-TREE Engine...\n");
    s_botree_inited = false;
}
