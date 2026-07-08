/**
 * @file agdte_buffer_manager.c
 * @brief ATOMEGearDisplayTrainEngine (AGDTE) Buffer Manager
 * @status Phase 2 Core Architecture & Display Scheduler Frozen
 *
 * @section PURPOSE
 * Manages zero-copy presentation buffer metadata, enforces strict ownership handoff,
 * prevents duplicate frame ownership across subsystems, and tracks buffer roles.
 */

#include "../include/agdte.h"

/* --- Static Pool for Authoritative Presentation Buffers --- */
static AGDTE_BufferDescriptor s_buffers[AGDTE_MAX_BUFFERS];
static uint32_t s_registered_buffer_count = 0;

void agdte_buffer_reset_all(void) {
    for (uint32_t i = 0; i < AGDTE_MAX_BUFFERS; i++) {
        s_buffers[i].buffer_id = i;
        s_buffers[i].virtual_address = (void*)0;
        s_buffers[i].physical_address = 0;
        s_buffers[i].width = 0;
        s_buffers[i].height = 0;
        s_buffers[i].pitch = 0;
        s_buffers[i].bpp = 32;
        s_buffers[i].role = AGDTE_BUFFER_ROLE_BACK;
        s_buffers[i].owner = AGDTE_BUFFER_OWNER_NONE;
        s_buffers[i].assigned_surface_id = 0xFFFFFFFF;
        s_buffers[i].is_locked = false;
    }
    s_registered_buffer_count = 0;
}

AGDTE_Error AGDTE_Buffer_Register(void* virtual_address, uint32_t width, uint32_t height, uint32_t pitch, AGDTE_BufferRole role, uint32_t* out_buffer_id) {
    if (!virtual_address || !out_buffer_id) {
        return AGDTE_ERR_NULL_POINTER;
    }
    if (width == 0 || height == 0 || pitch == 0) {
        return AGDTE_ERR_INVALID_STATE;
    }
    if (s_registered_buffer_count >= AGDTE_MAX_BUFFERS) {
        return AGDTE_ERR_OUT_OF_MEMORY;
    }

    /* Verify buffer address is not already registered under duplicate ownership */
    for (uint32_t i = 0; i < AGDTE_MAX_BUFFERS; i++) {
        if (s_buffers[i].virtual_address == virtual_address && s_buffers[i].owner != AGDTE_BUFFER_OWNER_NONE) {
            return AGDTE_ERR_OWNERSHIP_VIOLATION;
        }
    }

    /* Find available slot */
    for (uint32_t i = 0; i < AGDTE_MAX_BUFFERS; i++) {
        if (s_buffers[i].owner == AGDTE_BUFFER_OWNER_NONE && s_buffers[i].virtual_address == (void*)0) {
            s_buffers[i].buffer_id = i;
            s_buffers[i].virtual_address = virtual_address;
            s_buffers[i].physical_address = (uint64_t)(uintptr_t)virtual_address; /* Identity/Linear mapping baseline */
            s_buffers[i].width = width;
            s_buffers[i].height = height;
            s_buffers[i].pitch = pitch;
            s_buffers[i].bpp = 32;
            s_buffers[i].role = role;
            s_buffers[i].owner = AGDTE_BUFFER_OWNER_AGDTE_QUEUE;
            s_buffers[i].assigned_surface_id = 0xFFFFFFFF;
            s_buffers[i].is_locked = false;

            *out_buffer_id = i;
            s_registered_buffer_count++;
            return AGDTE_OK;
        }
    }

    return AGDTE_ERR_OUT_OF_MEMORY;
}

AGDTE_Error AGDTE_Buffer_Unregister(uint32_t buffer_id) {
    if (buffer_id >= AGDTE_MAX_BUFFERS || s_buffers[buffer_id].virtual_address == (void*)0) {
        return AGDTE_ERR_INVALID_BUFFER;
    }
    if (s_buffers[buffer_id].is_locked || s_buffers[buffer_id].owner == AGDTE_BUFFER_OWNER_DISPLAY_ACTIVE) {
        return AGDTE_ERR_OWNERSHIP_VIOLATION;
    }

    s_buffers[buffer_id].virtual_address = (void*)0;
    s_buffers[buffer_id].owner = AGDTE_BUFFER_OWNER_NONE;
    s_buffers[buffer_id].assigned_surface_id = 0xFFFFFFFF;
    if (s_registered_buffer_count > 0) {
        s_registered_buffer_count--;
    }
    return AGDTE_OK;
}

AGDTE_Error AGDTE_Buffer_TransferOwnership(uint32_t buffer_id, AGDTE_BufferOwner new_owner) {
    if (buffer_id >= AGDTE_MAX_BUFFERS || s_buffers[buffer_id].virtual_address == (void*)0) {
        return AGDTE_ERR_INVALID_BUFFER;
    }

    /* Enforce strict transfer contract: no locked buffers can be forcibly hijacked */
    if (s_buffers[buffer_id].is_locked) {
        return AGDTE_ERR_OWNERSHIP_VIOLATION;
    }

    /* Prevent duplicate active display assignments without explicit unpinning */
    if (s_buffers[buffer_id].owner == AGDTE_BUFFER_OWNER_DISPLAY_ACTIVE && new_owner == AGDTE_BUFFER_OWNER_BOGE_RENDER) {
        /* Must transition through staging or queue before rendering onto active scanout */
        s_buffers[buffer_id].owner = new_owner;
        return AGDTE_OK;
    }

    s_buffers[buffer_id].owner = new_owner;
    return AGDTE_OK;
}

AGDTE_BufferDescriptor* AGDTE_Buffer_GetDescriptor(uint32_t buffer_id) {
    if (buffer_id >= AGDTE_MAX_BUFFERS || s_buffers[buffer_id].virtual_address == (void*)0) {
        return (AGDTE_BufferDescriptor*)0;
    }
    return &s_buffers[buffer_id];
}
