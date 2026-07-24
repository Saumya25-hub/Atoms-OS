#ifndef ATOMS_APP_PERMISSIONS_H
#define ATOMS_APP_PERMISSIONS_H

#include <stdint.h>
#include <stdbool.h>

// Security Capabilities & Permission Bitmaps (Phase 9 Foundation)
#define ATOMS_CAPABILITY_NONE           0x00000000
#define ATOMS_CAPABILITY_STORAGE_READ   0x00000001
#define ATOMS_CAPABILITY_STORAGE_WRITE  0x00000002
#define ATOMS_CAPABILITY_NETWORK        0x00000004
#define ATOMS_CAPABILITY_AUDIO_PLAYBACK 0x00000008
#define ATOMS_CAPABILITY_AUDIO_RECORD   0x00000010
#define ATOMS_CAPABILITY_UI_SURFACE     0x00000020
#define ATOMS_CAPABILITY_SYSTEM_CONTROL 0x00000040
#define ATOMS_CAPABILITY_FULL_ACCESS    0xFFFFFFFF

typedef struct {
    uint32_t app_id;
    uint32_t granted_capabilities;
    uint32_t revoked_capabilities;
    char     digital_signature[64];
    bool     is_trusted;
} ATOMS_AppSecurityProfile;

void ATOMS_Permissions_Init(void);
bool ATOMS_CheckCapability(uint32_t app_id, uint32_t required_capability);
void ATOMS_GrantCapability(uint32_t app_id, uint32_t capability);
void ATOMS_RevokeCapability(uint32_t app_id, uint32_t capability);

#endif // ATOMS_APP_PERMISSIONS_H
