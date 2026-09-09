/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Common Types and Error Definitions
 */

#ifndef ATOMS_APAL_TYPES_H
#define ATOMS_APAL_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* APAL Status Codes */
typedef enum {
    APAL_OK                 = 0,
    APAL_ERR_INVALID_PARAM  = -1,
    APAL_ERR_NO_MEMORY      = -2,
    APAL_ERR_NOT_FOUND      = -3,
    APAL_ERR_ACCESS_DENIED  = -4,
    APAL_ERR_BUSY           = -5,
    APAL_ERR_TIMEOUT        = -6,
    APAL_ERR_IO             = -7,
    APAL_ERR_UNSUPPORTED    = -8,
    APAL_ERR_INTERNAL       = -9
} apal_status_t;

/* Generic Handle */
typedef uint64_t apal_handle_t;
#define APAL_INVALID_HANDLE 0ULL

/* Memory Protection Flags */
typedef enum {
    APAL_PROT_NONE          = 0x0,
    APAL_PROT_READ          = 0x1,
    APAL_PROT_WRITE         = 0x2,
    APAL_PROT_EXEC          = 0x4,
    APAL_PROT_READ_WRITE    = (APAL_PROT_READ | APAL_PROT_WRITE),
    APAL_PROT_READ_EXEC     = (APAL_PROT_READ | APAL_PROT_EXEC),
    APAL_PROT_READ_WRITE_EXEC = (APAL_PROT_READ | APAL_PROT_WRITE | APAL_PROT_EXEC)
} apal_prot_t;

/* Memory Allocation Flags */
typedef enum {
    APAL_MEM_ANONYMOUS      = 0x01,
    APAL_MEM_COMMIT         = 0x02,
    APAL_MEM_RESERVE        = 0x04,
    APAL_MEM_GUARD          = 0x08
} apal_mem_flags_t;

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_APAL_TYPES_H */
