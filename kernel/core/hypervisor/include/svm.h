/*
 * ATOMS OS — AMD SVM (AMD-V) Architecture Definitions
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 */

#ifndef ATOMS_SVM_H
#define ATOMS_SVM_H

#include <stdint.h>
#include <stdbool.h>

#define AMD_EFER_MSR                        0xC0000080
#define AMD_EFER_SVME_BIT                   (1ULL << 12)

#define AMD_VM_CR_MSR                       0xC0010114
#define AMD_VM_HSAVE_PA_MSR                 0xC0010117

/* AMD SVM Exit Codes */
#define SVM_EXIT_CR0_READ                   0x0000
#define SVM_EXIT_CR0_WRITE                  0x0010
#define SVM_EXIT_CR3_READ                   0x0003
#define SVM_EXIT_CR3_WRITE                  0x0013
#define SVM_EXIT_CR4_READ                   0x0004
#define SVM_EXIT_CR4_WRITE                  0x0014
#define SVM_EXIT_INTR                       0x0060
#define SVM_EXIT_NMI                        0x0061
#define SVM_EXIT_INIT                       0x0062
#define SVM_EXIT_VINTR                      0x0063
#define SVM_EXIT_CPUID                      0x0072
#define SVM_EXIT_HLT                        0x0078
#define SVM_EXIT_VMMCALL                    0x0081
#define SVM_EXIT_IOIO                       0x007B
#define SVM_EXIT_MSR                        0x007C
#define SVM_EXIT_NPF                        0x0400
#define SVM_EXIT_SHUTDOWN                   0x7FFF

/* AMD VMCB (Virtual Machine Control Block) Structure */
typedef struct __attribute__((packed)) {
    uint32_t cr_intercepts;
    uint32_t dr_intercepts;
    uint32_t exception_intercepts;
    uint32_t general1_intercepts;
    uint32_t general2_intercepts;
    uint32_t res1[10];
    uint64_t iopm_base_pa;
    uint64_t msrpm_base_pa;
    uint64_t tsc_offset;
    uint32_t asid;
    uint8_t  tlb_control;
    uint8_t  res2[3];
    uint64_t vintr;
    uint64_t interrupt_shadow;
    uint64_t exit_code;
    uint64_t exit_info1;
    uint64_t exit_info2;
    uint64_t exit_int_info;
    uint64_t np_enable;
    uint8_t  res3[16];
    uint64_t event_inj;
    uint64_t n_cr3;
    uint64_t lbr_virt_enable;
    uint8_t  res4[800];
    
    /* VMCB State Save Area */
    uint8_t  state_save[1024];
} AMD_VMCB;

#endif /* ATOMS_SVM_H */
