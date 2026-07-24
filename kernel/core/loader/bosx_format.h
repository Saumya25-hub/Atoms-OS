#ifndef BOSX_FORMAT_H
#define BOSX_FORMAT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// BOSX Executable Binary Format Specification (Phase 10)
// Magic: 'B' 'O' 'S' 'X' -> 0x58534F42
// ============================================================
#define BOSX_MAGIC              0x58534F42
#define BOSX_VERSION_MAJOR      1
#define BOSX_VERSION_MINOR      0
#define BOSX_ABI_VERSION        1

#define BOSX_ARCH_X86_64        0x003E
#define BOSX_ARCH_X86_32        0x0003

#define BOSX_MAX_SECTIONS       16
#define BOSX_MAX_IMPORTS        64
#define BOSX_MAX_EXPORTS        64

// Section Flags
#define BOSX_SEC_READ           (1 << 0)
#define BOSX_SEC_WRITE          (1 << 1)
#define BOSX_SEC_EXEC           (1 << 2)
#define BOSX_SEC_BSS            (1 << 3)
#define BOSX_SEC_RELOC          (1 << 4)

typedef struct {
    uint32_t magic;             // 'BOSX'
    uint16_t version_major;     // 1
    uint16_t version_minor;     // 0
    uint16_t abi_version;       // 1
    uint16_t architecture;      // BOSX_ARCH_X86_64
    uint64_t entry_point;       // Virtual Entry Point Offset
    uint64_t image_base;        // Preferred Load Base Address
    uint32_t image_size;        // Total Memory Size
    uint32_t section_count;     // Count of Section Headers
    uint32_t relocation_count;  // Count of Relocation Entries
    uint32_t import_count;      // Count of Import Entries
    uint32_t export_count;      // Count of Export Entries
    uint32_t checksum;          // Adler-32 / CRC32 Checksum
    uint32_t build_id;          // Unique Build Identifier
    uint8_t  signature[64];     // Digital Signature Placeholder
    uint32_t reserved[4];       // Reserved for Future Expansion
} __attribute__((packed)) BOSX_Header;

typedef struct {
    char     name[16];          // Section Name (".text", ".rodata", etc.)
    uint32_t virtual_addr;      // Relative Virtual Address (RVA)
    uint32_t virtual_size;      // Size in Memory
    uint32_t raw_data_offset;   // Offset in BOSX File
    uint32_t raw_data_size;     // Size in BOSX File
    uint32_t flags;             // BOSX_SEC_* flags
} __attribute__((packed)) BOSX_SectionHeader;

typedef struct {
    uint32_t virtual_offset;    // Location needing relocation
    uint32_t type;              // Relocation Type (0 = Absolute 64-bit, 1 = Relative 32-bit)
    int64_t  addend;            // Addend value
} __attribute__((packed)) BOSX_RelocationEntry;

typedef struct {
    char     library_name[64];  // Target SLL ("window.sll")
    char     symbol_name[64];   // Imported Symbol ("BOS_CreateWindow")
    uint32_t import_rva;        // Address slot to patch
} __attribute__((packed)) BOSX_ImportEntry;

typedef struct {
    char     symbol_name[64];   // Exported Symbol ("MyLibrary_Func")
    uint32_t export_rva;        // Relative Virtual Address of Function
} __attribute__((packed)) BOSX_ExportEntry;

#ifdef __cplusplus
}
#endif

#endif // BOSX_FORMAT_H
