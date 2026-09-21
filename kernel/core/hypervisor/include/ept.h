/*
 * ATOMS OS — Intel Extended Page Tables (EPT) Architecture Definitions
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 2: Second-Level Address Translation & Guest Physical Memory Virtualization
 */

#ifndef ATOMS_EPT_H
#define ATOMS_EPT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Intel EPT Capability MSR */
#define IA32_VMX_EPT_VPID_CAP_MSR           0x0000048C

/* EPT Pointer (EPTP) Configuration Bits */
#define EPTP_MEM_TYPE_UC                    0x00ULL          /* Uncacheable */
#define EPTP_MEM_TYPE_WB                    0x06ULL          /* Write Back */
#define EPTP_PAGE_WALK_LENGTH_4             (3ULL << 3)      /* 4-level paging (PML4) */
#define EPTP_ENABLE_ACCESS_DIRTY            (1ULL << 6)      /* Accessed / Dirty flags enable */
#define EPTP_ADDR_MASK                      0x000FFFFFFFFFF000ULL

/* EPT Entry Flag Bits (PML4E, PDPTE, PDE, PTE) */
#define EPT_FLAG_READ                       (1ULL << 0)
#define EPT_FLAG_WRITE                      (1ULL << 1)
#define EPT_FLAG_EXEC                       (1ULL << 2)
#define EPT_FLAG_MEM_TYPE_UC                (0ULL << 3)
#define EPT_FLAG_MEM_TYPE_WB                (6ULL << 3)
#define EPT_FLAG_IGNORE_PAT                 (1ULL << 6)
#define EPT_FLAG_PAGE_SIZE_2MB              (1ULL << 7)
#define EPT_FLAG_ACCESSED                   (1ULL << 8)
#define EPT_FLAG_DIRTY                      (1ULL << 9)
#define EPT_FLAG_USER_EXEC                  (1ULL << 10)
#define EPT_ADDR_MASK                       0x000FFFFFFFFFF000ULL

/* Secondary Processor-Based Execution Controls for EPT */
#define VMX_SECONDARY_EXEC_ENABLE_EPT       (1ULL << 1)
#define VMX_SECONDARY_EXEC_ENABLE_VPID      (1ULL << 5)
#define VMX_SECONDARY_EXEC_UNRESTRICTED_GUEST (1ULL << 7)

/* VMCS EPT Pointer Field Encoding */
#define VMCS_EPT_POINTER                    0x0000201A
#define VMCS_EPT_POINTER_HIGH               0x0000201B
#define VMCS_GUEST_PHYSICAL_ADDRESS         0x00002400
#define VMCS_GUEST_PHYSICAL_ADDRESS_HIGH    0x00002401
#define VMCS_SECONDARY_VM_EXEC_CONTROL      0x0000401E

/* VM-Exit Reasons for EPT */
#define VMX_EXIT_REASON_EPT_VIOLATION       48
#define VMX_EXIT_REASON_EPT_MISCONFIG       49
#define VMX_EXIT_REASON_INVEPT              50

/* EPT Violation Qualification Bitmasks */
#define EPT_VIOLATION_DATA_READ             (1ULL << 0)
#define EPT_VIOLATION_DATA_WRITE            (1ULL << 1)
#define EPT_VIOLATION_INSTR_FETCH           (1ULL << 2)
#define EPT_VIOLATION_ENTRY_READABLE        (1ULL << 3)
#define EPT_VIOLATION_ENTRY_WRITABLE        (1ULL << 4)
#define EPT_VIOLATION_ENTRY_EXEC            (1ULL << 5)
#define EPT_VIOLATION_GLA_VALID             (1ULL << 7)
#define EPT_VIOLATION_GPA_TRANSLATION       (1ULL << 8)
#define EPT_VIOLATION_USER_MODE             (1ULL << 9)
#define EPT_VIOLATION_READ_WRITE_PAGE       (1ULL << 10)
#define EPT_VIOLATION_EXEC_PAGE             (1ULL << 11)
#define EPT_VIOLATION_NMI_UNBLOCK           (1ULL << 12)

/* INVEPT Instruction Types */
#define INVEPT_TYPE_SINGLE_CONTEXT          1ULL
#define INVEPT_TYPE_ALL_CONTEXT             2ULL

/* INVEPT Descriptor (128-bit packed) */
typedef struct __attribute__((packed)) {
    uint64_t eptp;
    uint64_t reserved;
} invept_desc_t;

/* Low-level INVEPT instruction execution */
static inline uint8_t invept_execute(uint64_t type, const invept_desc_t *desc) {
    uint8_t error_code = 0;
    __asm__ volatile (
        "invept (%[desc]), %[type];"
        "setc %[err];"
        : [err] "=r"(error_code)
        : [type] "r"(type), [desc] "r"(desc)
        : "memory", "cc"
    );
    return error_code;
}

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_EPT_H */
