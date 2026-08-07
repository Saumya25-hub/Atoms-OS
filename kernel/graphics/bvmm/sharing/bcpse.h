#ifndef _BOS_BVMM_BCPSE_H_
#define _BOS_BVMM_BCPSE_H_

#include "../hal/bghal.h"
#include "kernel/core/sync/spinlock.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file bcpse.h
 * @brief Production Cross-Process GPU Resource Sharing Engine (BCPSE V1.0) Master Header
 * 
 * BCPSE is the permanent GPU resource sharing authority for BOS. Enables zero-copy sharing
 * of Surfaces, Textures, Buffers, Render Targets, Video Frames, Cursors, Overlays, and Scanout
 * Objects across process boundaries with capability token security, generation protection,
 * and automatic process exit cleanup.
 */

#define BCPSE_CANARY_MAGIC          0x42435053  /* "BCPS" */
#define BCPSE_MAX_SHARED_OBJECTS    1024
#define BCPSE_INVALID_HANDLE        0ULL

typedef uint64_t bcpse_handle_t;

/* Shared Object Types (Phase 11A Spec) */
typedef enum {
    BCPSE_OBJ_SURFACE       = 0,
    BCPSE_OBJ_TEXTURE       = 1,
    BCPSE_OBJ_BUFFER        = 2,
    BCPSE_OBJ_RENDER_TARGET = 3,
    BCPSE_OBJ_SCANOUT       = 4,
    BCPSE_OBJ_CURSOR        = 5,
    BCPSE_OBJ_OVERLAY       = 6,
    BCPSE_OBJ_VIDEO_FRAME   = 7
} bcpse_obj_type_t;

/* Permission Bitmask Flags (Phase 11F Spec) */
#define BCPSE_PERM_NONE         0x0000
#define BCPSE_PERM_READ         (1 << 0)
#define BCPSE_PERM_WRITE        (1 << 1)
#define BCPSE_PERM_READ_WRITE   (BCPSE_PERM_READ | BCPSE_PERM_WRITE)
#define BCPSE_PERM_SCANOUT      (1 << 2)
#define BCPSE_PERM_PRESENTATION (1 << 3)
#define BCPSE_PERM_VIDEO_DECODE (1 << 4)
#define BCPSE_PERM_PROTECTED    (1 << 5)
#define BCPSE_PERM_CURSOR       (1 << 6)
#define BCPSE_PERM_OVERLAY      (1 << 7)
#define BCPSE_PERM_ADMIN        (1 << 8)
#define BCPSE_PERM_KERNEL       (1 << 9)

typedef uint32_t bcpse_perm_flags_t;

/* Capability Security Token (Phase 11B Spec) */
typedef struct {
    uint64_t nonce;
    uint32_t owner_pid;
    uint32_t granted_pid;
    bcpse_perm_flags_t permissions;
    uint64_t expiration_time;
    uint32_t token_checksum;
} bcpse_security_token_t;

/* Primary Shared Object Descriptor Structure */
typedef struct bcpse_shared_obj {
    uint32_t               canary_magic;   /* 0x42435053 */
    bcpse_handle_t         shared_handle;
    uint16_t               generation_id;
    uint32_t               registry_index;
    bcpse_obj_type_t       type;
    uint32_t               owner_pid;
    brrle_lifetime_id_t    lifetime_id;
    btfe_texture_id_t      texture_id;
    bvmm_surface_id_t      surface_id;
    uint64_t               phys_vram_addr;
    uint64_t               size_bytes;
    uint32_t               export_count;
    uint32_t               import_count;
    uint32_t               ref_count;
    bcpse_security_token_t token;
    bool                   is_revoked;
} bcpse_shared_obj_t;

/* BCPSE Diagnostics Descriptor */
typedef struct {
    uint32_t total_objects_exported;
    uint32_t total_objects_imported;
    uint32_t active_shared_objects;
    uint64_t total_shared_vram_bytes;
    uint64_t zero_copy_transfers;
    uint32_t security_violations;
    uint32_t revoked_handles;
    uint32_t process_cleanups_executed;
} bcpse_diagnostics_t;

/**
 * @brief Initialize the Production Cross-Process GPU Resource Sharing Engine (BCPSE V1.0).
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bcpse_init(void);

/**
 * @brief Shutdown BCPSE and release shared object registries.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bcpse_shutdown(void);

/**
 * @brief Export a GPU object for cross-process sharing.
 * @param type Shared object type (Surface, Texture, Buffer, etc.).
 * @param lifetime_id Target BRRLE lifetime ID.
 * @param owner_pid Owner process ID.
 * @param target_pid Target process ID permitted to import (or 0 for public).
 * @param perms Granted permission bitmask.
 * @param out_handle Pointer to receive 64-bit Shared Handle.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bcpse_export_object(bcpse_obj_type_t type, brrle_lifetime_id_t lifetime_id, uint32_t owner_pid, uint32_t target_pid, bcpse_perm_flags_t perms, bcpse_handle_t* out_handle);

/**
 * @brief Import a shared GPU object in a consumer process (Zero-Copy).
 * @param handle Shared handle to import.
 * @param caller_pid Calling process ID.
 * @param required_perms Requested permission flags.
 * @param out_obj Pointer to receive imported shared object descriptor.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bcpse_import_object(bcpse_handle_t handle, uint32_t caller_pid, bcpse_perm_flags_t required_perms, bcpse_shared_obj_t** out_obj);

/**
 * @brief Release an imported reference to a shared GPU object.
 * @param handle Shared handle to release.
 * @param caller_pid Calling process ID.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bcpse_release_import(bcpse_handle_t handle, uint32_t caller_pid);

/**
 * @brief Revoke a shared GPU handle immediately.
 * @param handle Shared handle to revoke.
 * @param owner_pid Process ID of object owner requesting revocation.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bcpse_revoke_handle(bcpse_handle_t handle, uint32_t owner_pid);

/**
 * @brief Cleanup all shared GPU resources and imports owned by an exiting process.
 * @param exiting_pid Process ID of exiting/terminated process.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bcpse_cleanup_process_resources(uint32_t exiting_pid);

/**
 * @brief Dump developer inspection log for BCPSE operations.
 */
void bcpse_dump(void);

/**
 * @brief Retrieve current BCPSE diagnostics summary.
 * @param out_diag Pointer to receive diagnostics descriptor.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bcpse_get_diagnostics(bcpse_diagnostics_t* out_diag);

/**
 * @brief Validate internal integrity of BCPSE state.
 * @return true if valid, false if corruption detected.
 */
bool bcpse_validate_all(void);

#ifdef __cplusplus
}
#endif

#endif /* _BOS_BVMM_BCPSE_H_ */
