#include "../include/explorer_api.h"
#include "kernel/drivers/display/display.h"

static EXPLORER_SESSION_STATE g_session_state = EXPLORER_SESSION_STOPPED;

bool ExplorerStartSession(void) {
    display_print("[EXPLORER_SESSION] Starting Desktop Environment Session Host (PID 101)...\n");
    g_session_state = EXPLORER_SESSION_RUNNING;
    display_print("[EXPLORER_SESSION] Desktop Session Running.\n");
    return true;
}

void ExplorerLogout(void) {
    display_print("[EXPLORER_SESSION] Logging out user session...\n");
    g_session_state = EXPLORER_SESSION_LOGGING_OUT;
}
