#ifndef PLAYBACK_STATE_H
#define PLAYBACK_STATE_H

#include "../include/bospectra_playback_types.h"
#include "../../include/bospectra_errors.h"

typedef struct {
    bospectra_playback_state_t current_state;
    bospectra_playback_state_t previous_state;
} BOSPECTRA_StateMachine;

void              playback_state_init(BOSPECTRA_StateMachine* sm);
bospectra_error_t playback_state_transition(BOSPECTRA_StateMachine* sm, bospectra_playback_state_t new_state);
bool              playback_state_is_valid_transition(bospectra_playback_state_t current, bospectra_playback_state_t next);

#endif // PLAYBACK_STATE_H
