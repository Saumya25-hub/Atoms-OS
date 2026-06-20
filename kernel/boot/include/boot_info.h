#ifndef BOOT_INFO_H
#define BOOT_INFO_H

#include <stdint.h>

#define BOOT_INFO_MAGIC 0x1337B007

typedef struct {
    uint64_t base_address;
    uint64_t length;
    uint32_t type;
    uint32_t acpi_attributes;
} __attribute__((packed)) memory_map_entry_t;

typedef struct {
    uint32_t memory_entry_count;
    uint32_t padding; // For 8-byte alignment of the entries array
    memory_map_entry_t entries[];
} __attribute__((packed)) boot_info_t;

// E820 Memory Types
#define MEMORY_TYPE_USABLE               1
#define MEMORY_TYPE_RESERVED             2
#define MEMORY_TYPE_ACPI_RECLAIMABLE     3
#define MEMORY_TYPE_ACPI_NVS             4
#define MEMORY_TYPE_BAD_MEMORY           5

#endif // BOOT_INFO_H
