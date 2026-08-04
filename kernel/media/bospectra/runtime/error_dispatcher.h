/*
 * BOSPECTRA V3 — Error Dispatcher Subsystem
 * kernel/media/bospectra/runtime/error_dispatcher.h
 */

#ifndef BOSPECTRA_V3_ERROR_DISPATCHER_H
#define BOSPECTRA_V3_ERROR_DISPATCHER_H

#include "error_manager.h"

void              bospectra_error_dispatcher_init(void);
void              bospectra_error_dispatcher_shutdown(void);

bospectra_error_t bospectra_error_dispatch(const BOSPECTRA_Error* err);
uint32_t          bospectra_error_get_total_count(void);

#endif /* BOSPECTRA_V3_ERROR_DISPATCHER_H */
