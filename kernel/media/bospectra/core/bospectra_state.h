#ifndef BOSPECTRA_STATE_H
#define BOSPECTRA_STATE_H

#include "../include/bospectra_types.h"

void              bospectra_state_init(void);
bospectra_state_t bospectra_state_get(void);
bospectra_error_t bospectra_state_transition(bospectra_state_t expected, bospectra_state_t new_state);
bool              bospectra_state_can_transition(bospectra_state_t current, bospectra_state_t target);
const char*       bospectra_state_to_string(bospectra_state_t state);

#endif // BOSPECTRA_STATE_H
