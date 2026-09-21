/*
 * ATOMS OS — AMD Nested Page Tables (NPT) Architecture Definitions
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 2: Second-Level Address Translation for AMD SVM
 */

#ifndef ATOMS_NPT_H
#define ATOMS_NPT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* AMD NPT Entry Flag Bits */
#define NPT_FLAG_PRESENT                    (1ULL << 0)
#define NPT_FLAG_WRITE                      (1ULL << 1)
#define NPT_FLAG_USER                       (1ULL << 2)
#define NPT_FLAG_PWT                        (1ULL << 3)
#define NPT_FLAG_PCD                        (1ULL << 4)
#define NPT_FLAG_ACCESSED                   (1ULL << 5)
#define NPT_FLAG_DIRTY                      (1ULL << 6)
#define NPT_FLAG_PAGE_SIZE_2MB              (1ULL << 7)
#define NPT_FLAG_NX                         (1ULL << 63)
#define NPT_ADDR_MASK                       0x000FFFFFFFFFF000ULL

/* AMD Nested Page Fault (NPF) Error Code Bitmasks (exit_info1) */
#define NPF_FLAG_PRESENT                    (1ULL << 0)
#define NPF_FLAG_WRITE                      (1ULL << 1)
#define NPF_FLAG_USER                       (1ULL << 2)
#define NPF_FLAG_RSVD                       (1ULL << 3)
#define NPF_FLAG_INSTR_FETCH                (1ULL << 4)

/* AMD VMCB TLB Control Flags */
#define VMCB_TLB_CONTROL_DO_NOTHING         0
#define VMCB_TLB_CONTROL_FLUSH_ALL_ASID     1
#define VMCB_TLB_CONTROL_FLUSH_GUEST_ASID   3
#define VMCB_TLB_CONTROL_FLUSH_NON_GLOBAL   7

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_NPT_H */
