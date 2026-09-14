/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Master Header
 */

#ifndef ATOMS_APAL_H
#define ATOMS_APAL_H

#include "apal_types.h"
#include "../memory/apal_memory.h"
#include "../threads/apal_thread.h"
#include "../process/apal_process.h"
#include "../ipc/apal_ipc.h"
#include "../shared_memory/apal_shm.h"
#include "../filesystem/apal_fs.h"
#include "../sockets/apal_socket.h"
#include "../time/apal_time.h"
#include "../randomness/apal_rand.h"
#include "../graphics/apal_surface.h"
#include "../input/apal_input.h"
#include "../audio/apal_audio.h"
#include "../loader/apal_loader.h"
#include "../exceptions/apal_exception.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Global APAL System Lifecycle */
apal_status_t apal_init(void);
apal_status_t apal_shutdown(void);
const char *apal_get_version_string(void);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_APAL_H */
