#ifndef BOSPECTRA_MEDIA_H
#define BOSPECTRA_MEDIA_H

#include "bospectra_types.h"

// High-Level Session Configuration Struct
typedef struct {
    uint32_t flags;
    uint32_t target_width;
    uint32_t target_height;
    uint32_t sample_rate;
    uint8_t  channels;
    bool     auto_play;
    bool     looping;
} BOSPECTRA_SessionConfig;

// Public High-Level Media Session Interface
bospectra_error_t BOSPECTRA_Session_Create(const char* file_path, const BOSPECTRA_SessionConfig* config, bospectra_handle_t* out_session);
bospectra_error_t BOSPECTRA_Session_Destroy(bospectra_handle_t session);
bospectra_error_t BOSPECTRA_Session_Start(bospectra_handle_t session);
bospectra_error_t BOSPECTRA_Session_Pause(bospectra_handle_t session);
bospectra_error_t BOSPECTRA_Session_Stop(bospectra_handle_t session);
bospectra_error_t BOSPECTRA_Session_GetState(bospectra_handle_t session, bospectra_state_t* out_state);

#endif // BOSPECTRA_MEDIA_H
