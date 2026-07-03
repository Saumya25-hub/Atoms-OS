#ifndef AC97_BDL_H
#define AC97_BDL_H

#include <stdint.h>

#define AC97_BDL_ENTRIES 32

// Descriptor Flags
#define AC97_BDL_FLAG_IOC (1 << 15) // Interrupt On Completion
#define AC97_BDL_FLAG_BUP (1 << 14) // Buffer Underrun Policy

#pragma pack(push, 1)
typedef struct {
    uint32_t buffer_phys_addr;
    uint16_t length; // Length in samples (not bytes)
    uint16_t flags;
} Ac97BdlEntry;
#pragma pack(pop)

#endif // AC97_BDL_H
