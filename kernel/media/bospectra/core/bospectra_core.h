#ifndef BOSPECTRA_CORE_H
#define BOSPECTRA_CORE_H

#include "../include/bospectra_types.h"

// Private Core Initialization & Subsystem Bootstrap APIs
bospectra_error_t bospectra_core_bootstrap_subsystems(void);
bospectra_error_t bospectra_core_teardown_subsystems(void);
bool              bospectra_core_is_ready(void);

#endif // BOSPECTRA_CORE_H
