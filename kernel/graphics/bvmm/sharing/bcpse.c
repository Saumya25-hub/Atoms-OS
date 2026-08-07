#include "bcpse.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/sync/spinlock.h"
#include "kernel/core/lib/include/string.h"

extern bvmm_result_t bcpse_registry_init(void);
extern bvmm_result_t bcpse_registry_shutdown(void);
extern bvmm_result_t bcpse_registry_register_object(bcpse_shared_obj_t* obj, bcpse_handle_t* out_handle);
extern bvmm_result_t bcpse_registry_lookup_object(bcpse_handle_t handle, bcpse_shared_obj_t** out_obj);
extern bvmm_result_t bcpse_registry_unregister_object(bcpse_handle_t handle);

static atoms_spinlock_t   g_sharing_engine_lock;
static bool               g_sharing_active = false;
static bcpse_diagnostics_t g_sharing_diag = {0};

bvmm_result_t bcpse_init(void) {
    if (g_sharing_active) return BVMM_ERR_ALREADY_INITIALIZED;

    atoms_spinlock_init(&g_sharing_engine_lock, 0);
    memset(&g_sharing_diag, 0, sizeof(bcpse_diagnostics_t));

    bvmm_result_t res = bcpse_registry_init();
    if (res != BVMM_SUCCESS) return res;

    g_sharing_active = true;
    return BVMM_SUCCESS;
}

bvmm_result_t bcpse_shutdown(void) {
    if (!g_sharing_active) return BVMM_ERR_NOT_INITIALIZED;

    bcpse_registry_shutdown();
    g_sharing_active = false;
    return BVMM_SUCCESS;
}

bvmm_result_t bcpse_export_object(bcpse_obj_type_t type, brrle_lifetime_id_t lifetime_id, uint32_t owner_pid, uint32_t target_pid, bcpse_perm_flags_t perms, bcpse_handle_t* out_handle) {
    if (owner_pid == 0 || !out_handle) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_sharing_active) return BVMM_ERR_NOT_INITIALIZED;

    /* Validate BRRLE Lifetime */
    brrle_lifetime_desc_t* desc = NULL;
    if (lifetime_id != BRRLE_INVALID_LIFETIME_ID) {
        if (brrle_lifetime_lookup(lifetime_id, &desc) != BVMM_SUCCESS || !desc) {
            return BVMM_ERR_INVALID_HANDLE;
        }
    }

    bcpse_shared_obj_t* obj = (bcpse_shared_obj_t*)kmalloc(sizeof(bcpse_shared_obj_t));
    if (!obj) return BVMM_ERR_OUT_OF_MEMORY;

    memset(obj, 0, sizeof(bcpse_shared_obj_t));
    obj->canary_magic  = BCPSE_CANARY_MAGIC;
    obj->type          = type;
    obj->owner_pid     = owner_pid;
    obj->lifetime_id   = lifetime_id;
    obj->phys_vram_addr= 0x10000000ULL;
    obj->size_bytes    = 4 * 1024 * 1024ULL;
    obj->export_count  = 1;
    obj->import_count  = 0;
    obj->ref_count     = 1;

    /* Populate Security Token */
    obj->token.nonce          = 0x123456789ABCDEF0ULL + (uint64_t)lifetime_id;
    obj->token.owner_pid      = owner_pid;
    obj->token.granted_pid    = target_pid;
    obj->token.permissions    = perms;
    obj->token.expiration_time= 0ULL;
    obj->token.token_checksum = (uint32_t)(perms ^ owner_pid);

    bcpse_handle_t handle = BCPSE_INVALID_HANDLE;
    bvmm_result_t res = bcpse_registry_register_object(obj, &handle);
    if (res != BVMM_SUCCESS) {
        kfree(obj);
        return res;
    }

    atoms_spin_lock(&g_sharing_engine_lock);
    g_sharing_diag.total_objects_exported++;
    g_sharing_diag.active_shared_objects++;
    g_sharing_diag.total_shared_vram_bytes += obj->size_bytes;
    atoms_spin_unlock(&g_sharing_engine_lock);

    *out_handle = handle;
    return BVMM_SUCCESS;
}

bvmm_result_t bcpse_import_object(bcpse_handle_t handle, uint32_t caller_pid, bcpse_perm_flags_t required_perms, bcpse_shared_obj_t** out_obj) {
    if (handle == BCPSE_INVALID_HANDLE || caller_pid == 0 || !out_obj) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_sharing_active) return BVMM_ERR_NOT_INITIALIZED;

    bcpse_shared_obj_t* obj = NULL;
    bvmm_result_t res = bcpse_registry_lookup_object(handle, &obj);
    if (res != BVMM_SUCCESS || !obj) return res;

    if (obj->is_revoked) {
        atoms_spin_lock(&g_sharing_engine_lock);
        g_sharing_diag.security_violations++;
        atoms_spin_unlock(&g_sharing_engine_lock);
        return BVMM_ERR_PERMISSION_DENIED;
    }

    /* Validate Granted PID if non-zero */
    if (obj->token.granted_pid != 0 && obj->token.granted_pid != caller_pid) {
        atoms_spin_lock(&g_sharing_engine_lock);
        g_sharing_diag.security_violations++;
        atoms_spin_unlock(&g_sharing_engine_lock);
        return BVMM_ERR_PERMISSION_DENIED;
    }

    /* Validate Requested Permission Bitmask */
    if ((obj->token.permissions & required_perms) != required_perms) {
        atoms_spin_lock(&g_sharing_engine_lock);
        g_sharing_diag.security_violations++;
        atoms_spin_unlock(&g_sharing_engine_lock);
        return BVMM_ERR_PERMISSION_DENIED;
    }

    /* Zero Copy Import: Increment counters and return same object pointer */
    obj->import_count++;
    obj->ref_count++;

    if (obj->lifetime_id != BRRLE_INVALID_LIFETIME_ID) {
        brrle_cpu_acquire(obj->lifetime_id);
    }

    atoms_spin_lock(&g_sharing_engine_lock);
    g_sharing_diag.total_objects_imported++;
    g_sharing_diag.zero_copy_transfers++;
    atoms_spin_unlock(&g_sharing_engine_lock);

    *out_obj = obj;
    return BVMM_SUCCESS;
}

bvmm_result_t bcpse_release_import(bcpse_handle_t handle, uint32_t caller_pid) {
    (void)caller_pid;
    bcpse_shared_obj_t* obj = NULL;
    bvmm_result_t res = bcpse_registry_lookup_object(handle, &obj);
    if (res != BVMM_SUCCESS || !obj) return res;

    if (obj->import_count > 0) obj->import_count--;
    if (obj->ref_count > 0) obj->ref_count--;

    if (obj->lifetime_id != BRRLE_INVALID_LIFETIME_ID) {
        brrle_cpu_release(obj->lifetime_id);
    }

    /* Destroy object only when CPU == 0, GPU == 0, Imports == 0, Exports == 0 */
    if (obj->ref_count == 0 && obj->import_count == 0) {
        atoms_spin_lock(&g_sharing_engine_lock);
        if (g_sharing_diag.active_shared_objects > 0) g_sharing_diag.active_shared_objects--;
        if (g_sharing_diag.total_shared_vram_bytes >= obj->size_bytes) {
            g_sharing_diag.total_shared_vram_bytes -= obj->size_bytes;
        }
        atoms_spin_unlock(&g_sharing_engine_lock);

        bcpse_registry_unregister_object(handle);
        kfree(obj);
    }

    return BVMM_SUCCESS;
}

bvmm_result_t bcpse_revoke_handle(bcpse_handle_t handle, uint32_t owner_pid) {
    bcpse_shared_obj_t* obj = NULL;
    bvmm_result_t res = bcpse_registry_lookup_object(handle, &obj);
    if (res != BVMM_SUCCESS || !obj) return res;

    if (obj->owner_pid != owner_pid) return BVMM_ERR_PERMISSION_DENIED;

    obj->is_revoked = true;

    atoms_spin_lock(&g_sharing_engine_lock);
    g_sharing_diag.revoked_handles++;
    atoms_spin_unlock(&g_sharing_engine_lock);

    return BVMM_SUCCESS;
}

bvmm_result_t bcpse_cleanup_process_resources(uint32_t exiting_pid) {
    if (exiting_pid == 0) return BVMM_ERR_INVALID_ARGUMENT;

    for (uint32_t i = 1; i <= BCPSE_MAX_SHARED_OBJECTS; i++) {
        bcpse_shared_obj_t* obj = NULL;
        if (bcpse_registry_lookup_object((bcpse_handle_t)i, &obj) == BVMM_SUCCESS && obj) {
            if (obj->owner_pid == exiting_pid || obj->token.granted_pid == exiting_pid) {
                bcpse_release_import((bcpse_handle_t)i, exiting_pid);
            }
        }
    }

    atoms_spin_lock(&g_sharing_engine_lock);
    g_sharing_diag.process_cleanups_executed++;
    atoms_spin_unlock(&g_sharing_engine_lock);

    return BVMM_SUCCESS;
}

bvmm_result_t bcpse_get_diagnostics(bcpse_diagnostics_t* out_diag) {
    if (!out_diag) return BVMM_ERR_INVALID_ARGUMENT;
    atoms_spin_lock(&g_sharing_engine_lock);
    *out_diag = g_sharing_diag;
    atoms_spin_unlock(&g_sharing_engine_lock);
    return BVMM_SUCCESS;
}
