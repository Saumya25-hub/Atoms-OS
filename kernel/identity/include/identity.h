#ifndef ATOMS_KERNEL_IDENTITY_H
#define ATOMS_KERNEL_IDENTITY_H

#include <stdbool.h>
#include <stdint.h>

/*
 * ============================================================================
 * ATOMS Identity Engine (AIE) — Core Authentication Subsystem
 * ============================================================================
 * Phase 1: Temporary hardcoded credential validation (admin / admin123).
 * Phase 2: Settings integration for user-customizable credentials.
 * Phase 3: Persistent configuration / SQLite database backend.
 * Phase 4: Multi-user profiles, password hashing, and ACL permissions.
 * ============================================================================
 */

/* Initialize the Identity Engine subsystem */
void Identity_Init(void);

/*
 * Authenticate credentials against the current user database.
 * Returns true if username and password match a valid system account.
 */
bool Identity_Authenticate(const char* username, const char* password);

/* Get the default system username for UI display */
const char* Identity_GetDefaultUsername(void);

#endif /* ATOMS_KERNEL_IDENTITY_H */
