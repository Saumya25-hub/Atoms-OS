#include "bospectra_state.h"
#include "../include/bospectra_errors.h"

static bospectra_state_t g_bospectra_current_state = BOSPECTRA_STATE_UNINITIALIZED;

void bospectra_state_init(void) {
    g_bospectra_current_state = BOSPECTRA_STATE_UNINITIALIZED;
}

bospectra_state_t bospectra_state_get(void) {
    return g_bospectra_current_state;
}

bool bospectra_state_can_transition(bospectra_state_t current, bospectra_state_t target) {
    switch (current) {
        case BOSPECTRA_STATE_UNINITIALIZED:
            return (target == BOSPECTRA_STATE_INITIALIZING);
        case BOSPECTRA_STATE_INITIALIZING:
            return (target == BOSPECTRA_STATE_READY || target == BOSPECTRA_STATE_ERROR);
        case BOSPECTRA_STATE_READY:
            return (target == BOSPECTRA_STATE_RUNNING || target == BOSPECTRA_STATE_STOPPING);
        case BOSPECTRA_STATE_RUNNING:
            return (target == BOSPECTRA_STATE_PAUSED || target == BOSPECTRA_STATE_READY || target == BOSPECTRA_STATE_STOPPING || target == BOSPECTRA_STATE_ERROR);
        case BOSPECTRA_STATE_PAUSED:
            return (target == BOSPECTRA_STATE_RUNNING || target == BOSPECTRA_STATE_READY || target == BOSPECTRA_STATE_STOPPING);
        case BOSPECTRA_STATE_STOPPING:
            return (target == BOSPECTRA_STATE_SHUTDOWN || target == BOSPECTRA_STATE_ERROR);
        case BOSPECTRA_STATE_ERROR:
            return (target == BOSPECTRA_STATE_STOPPING || target == BOSPECTRA_STATE_SHUTDOWN);
        case BOSPECTRA_STATE_SHUTDOWN:
            return (target == BOSPECTRA_STATE_UNINITIALIZED);
        default:
            return false;
    }
}

bospectra_error_t bospectra_state_transition(bospectra_state_t expected, bospectra_state_t new_state) {
    if (g_bospectra_current_state != expected) {
        return BOSPECTRA_ERR_STATE_INVALID;
    }
    if (!bospectra_state_can_transition(g_bospectra_current_state, new_state)) {
        return BOSPECTRA_ERR_STATE_INVALID;
    }
    g_bospectra_current_state = new_state;
    return BOSPECTRA_SUCCESS;
}

const char* bospectra_state_to_string(bospectra_state_t state) {
    switch (state) {
        case BOSPECTRA_STATE_UNINITIALIZED: return "UNINITIALIZED";
        case BOSPECTRA_STATE_INITIALIZING:  return "INITIALIZING";
        case BOSPECTRA_STATE_READY:         return "READY";
        case BOSPECTRA_STATE_RUNNING:       return "RUNNING";
        case BOSPECTRA_STATE_PAUSED:        return "PAUSED";
        case BOSPECTRA_STATE_STOPPING:      return "STOPPING";
        case BOSPECTRA_STATE_ERROR:         return "ERROR";
        case BOSPECTRA_STATE_SHUTDOWN:      return "SHUTDOWN";
        default:                            return "UNKNOWN";
    }
}
