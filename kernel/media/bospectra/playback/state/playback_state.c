#include "playback_state.h"

void playback_state_init(BOSPECTRA_StateMachine* sm) {
    if (!sm) return;
    sm->current_state = BOSPECTRA_PLAYBACK_STATE_IDLE;
    sm->previous_state = BOSPECTRA_PLAYBACK_STATE_IDLE;
}

bool playback_state_is_valid_transition(bospectra_playback_state_t current, bospectra_playback_state_t next) {
    if (current == next) return true;

    switch (current) {
        case BOSPECTRA_PLAYBACK_STATE_IDLE:
            return (next == BOSPECTRA_PLAYBACK_STATE_OPENING || next == BOSPECTRA_PLAYBACK_STATE_ERROR);

        case BOSPECTRA_PLAYBACK_STATE_OPENING:
            return (next == BOSPECTRA_PLAYBACK_STATE_READY || next == BOSPECTRA_PLAYBACK_STATE_ERROR || next == BOSPECTRA_PLAYBACK_STATE_IDLE);

        case BOSPECTRA_PLAYBACK_STATE_READY:
            return (next == BOSPECTRA_PLAYBACK_STATE_PLAYING || next == BOSPECTRA_PLAYBACK_STATE_STOPPED || next == BOSPECTRA_PLAYBACK_STATE_IDLE || next == BOSPECTRA_PLAYBACK_STATE_ERROR);

        case BOSPECTRA_PLAYBACK_STATE_PLAYING:
            return (next == BOSPECTRA_PLAYBACK_STATE_PAUSED || next == BOSPECTRA_PLAYBACK_STATE_SEEKING || next == BOSPECTRA_PLAYBACK_STATE_BUFFERING || next == BOSPECTRA_PLAYBACK_STATE_STOPPED || next == BOSPECTRA_PLAYBACK_STATE_ENDED || next == BOSPECTRA_PLAYBACK_STATE_ERROR);

        case BOSPECTRA_PLAYBACK_STATE_PAUSED:
            return (next == BOSPECTRA_PLAYBACK_STATE_PLAYING || next == BOSPECTRA_PLAYBACK_STATE_SEEKING || next == BOSPECTRA_PLAYBACK_STATE_STOPPED || next == BOSPECTRA_PLAYBACK_STATE_IDLE || next == BOSPECTRA_PLAYBACK_STATE_ERROR);

        case BOSPECTRA_PLAYBACK_STATE_SEEKING:
            return (next == BOSPECTRA_PLAYBACK_STATE_PLAYING || next == BOSPECTRA_PLAYBACK_STATE_PAUSED || next == BOSPECTRA_PLAYBACK_STATE_READY || next == BOSPECTRA_PLAYBACK_STATE_ERROR);

        case BOSPECTRA_PLAYBACK_STATE_BUFFERING:
            return (next == BOSPECTRA_PLAYBACK_STATE_PLAYING || next == BOSPECTRA_PLAYBACK_STATE_PAUSED || next == BOSPECTRA_PLAYBACK_STATE_ERROR);

        case BOSPECTRA_PLAYBACK_STATE_STOPPED:
            return (next == BOSPECTRA_PLAYBACK_STATE_READY || next == BOSPECTRA_PLAYBACK_STATE_IDLE || next == BOSPECTRA_PLAYBACK_STATE_PLAYING || next == BOSPECTRA_PLAYBACK_STATE_ERROR);

        case BOSPECTRA_PLAYBACK_STATE_ENDED:
            return (next == BOSPECTRA_PLAYBACK_STATE_READY || next == BOSPECTRA_PLAYBACK_STATE_STOPPED || next == BOSPECTRA_PLAYBACK_STATE_IDLE || next == BOSPECTRA_PLAYBACK_STATE_PLAYING || next == BOSPECTRA_PLAYBACK_STATE_ERROR);

        case BOSPECTRA_PLAYBACK_STATE_ERROR:
            return (next == BOSPECTRA_PLAYBACK_STATE_IDLE || next == BOSPECTRA_PLAYBACK_STATE_OPENING);

        default:
            return false;
    }
}

bospectra_error_t playback_state_transition(BOSPECTRA_StateMachine* sm, bospectra_playback_state_t new_state) {
    if (!sm) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    if (!playback_state_is_valid_transition(sm->current_state, new_state)) {
        return BOSPECTRA_ERR_STATE_INVALID;
    }

    sm->previous_state = sm->current_state;
    sm->current_state = new_state;
    return BOSPECTRA_SUCCESS;
}
