/*
 * ATOMS OS — Intel VT-x (VMX) Architecture Definitions
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 */

#ifndef ATOMS_VMX_H
#define ATOMS_VMX_H

#include <stdint.h>
#include <stdbool.h>

/* Intel IA32 MSR Constants */
#define IA32_FEATURE_CONTROL_MSR            0x0000003A
#define IA32_FEATURE_CONTROL_LOCK_BIT       (1ULL << 0)
#define IA32_FEATURE_CONTROL_VMXON_OUTSIDE  (1ULL << 2)
#define IA32_FEATURE_CONTROL_VMXON_OUTSIDE_SMX IA32_FEATURE_CONTROL_VMXON_OUTSIDE

#define CR4_VMXE_BIT                        (1ULL << 13)

#define IA32_VMX_BASIC_MSR                  0x00000480
#define IA32_VMX_PINBASED_CTLS_MSR          0x00000481
#define IA32_VMX_PROCBASED_CTLS_MSR         0x00000482
#define IA32_VMX_EXIT_CTLS_MSR              0x00000483
#define IA32_VMX_ENTRY_CTLS_MSR             0x00000484
#define IA32_VMX_CR0_FIXED0_MSR             0x00000486
#define IA32_VMX_CR0_FIXED1_MSR             0x00000487
#define IA32_VMX_CR4_FIXED0_MSR             0x00000488
#define IA32_VMX_CR4_FIXED1_MSR             0x00000489
#define IA32_VMX_PROCBASED_CTLS2_MSR        0x0000048B
#define IA32_VMX_EPT_VPID_CAP_MSR           0x0000048C
#define IA32_VMX_EPT_VPID_CAP_AD_BITS       (1ULL << 21)

#define IA32_VMX_TRUE_PINBASED_CTLS_MSR     0x0000048D
#define IA32_VMX_TRUE_PROCBASED_CTLS_MSR    0x0000048E
#define IA32_VMX_TRUE_EXIT_CTLS_MSR         0x0000048F
#define IA32_VMX_TRUE_ENTRY_CTLS_MSR        0x00000490

/* 16-Bit Guest-State Fields */
#define VMCS_GUEST_ES_SELECTOR              0x00000800
#define VMCS_GUEST_CS_SELECTOR              0x00000802
#define VMCS_GUEST_SS_SELECTOR              0x00000804
#define VMCS_GUEST_DS_SELECTOR              0x00000806
#define VMCS_GUEST_FS_SELECTOR              0x00000808
#define VMCS_GUEST_GS_SELECTOR              0x0000080A
#define VMCS_GUEST_LDTR_SELECTOR            0x0000080C
#define VMCS_GUEST_TR_SELECTOR              0x0000080E

/* 16-Bit Host-State Fields */
#define VMCS_HOST_ES_SELECTOR               0x00000C00
#define VMCS_HOST_CS_SELECTOR               0x00000C02
#define VMCS_HOST_SS_SELECTOR               0x00000C04
#define VMCS_HOST_DS_SELECTOR               0x00000C06
#define VMCS_HOST_FS_SELECTOR               0x00000C08
#define VMCS_HOST_GS_SELECTOR               0x00000C0A
#define VMCS_HOST_TR_SELECTOR               0x00000C0C

/* 64-Bit Control Fields */
#define VMCS_EPT_POINTER                    0x0000201A

/* 64-Bit Read-Only Data Fields */
#define VMCS_GUEST_PHYSICAL_ADDRESS         0x00002400

/* 64-Bit Guest-State Fields */
#define VMCS_LINK_POINTER                   0x00002800
#define VMCS_GUEST_IA32_DEBUGCTL            0x00002802
#define VMCS_GUEST_IA32_PAT                 0x00002804
#define VMCS_GUEST_IA32_EFER                0x00002806
#define VMCS_GUEST_PDPTE0                   0x0000280A
#define VMCS_GUEST_PDPTE1                   0x0000280C
#define VMCS_GUEST_PDPTE2                   0x0000280E
#define VMCS_GUEST_PDPTE3                   0x00002810

/* 64-Bit Host-State Fields */
#define VMCS_HOST_IA32_PAT                  0x00002C00
#define VMCS_HOST_IA32_EFER                 0x00002C02

/* 32-Bit Control Fields */
#define VMCS_PIN_BASED_VM_EXEC_CONTROL      0x00004000
#define VMCS_CPU_BASED_VM_EXEC_CONTROL      0x00004002
#define VMCS_EXCEPTION_BITMAP               0x00004004
#define VMCS_PAGE_FAULT_ERROR_CODE_MASK     0x00004006
#define VMCS_PAGE_FAULT_ERROR_CODE_MATCH    0x00004008
#define VMCS_CR3_TARGET_COUNT               0x0000400A
#define VMCS_VM_EXIT_CONTROLS               0x0000400C
#define VMCS_VM_EXIT_MSR_STORE_COUNT        0x0000400E
#define VMCS_VM_EXIT_MSR_LOAD_COUNT         0x00004010
#define VMCS_VM_ENTRY_CONTROLS              0x00004012
#define VMCS_VM_ENTRY_MSR_LOAD_COUNT        0x00004014
#define VMCS_VM_ENTRY_INTR_INFO_FIELD       0x00004016
#define VMCS_VM_ENTRY_EXCEPTION_ERROR_CODE  0x00004018
#define VMCS_VM_ENTRY_INSTRUCTION_LEN       0x0000401A
#define VMCS_TPR_THRESHOLD                  0x0000401C
#define VMCS_SECONDARY_VM_EXEC_CONTROL      0x0000401E

/* Natural 64-Bit Control Fields */
#define VMCS_CR0_GUEST_HOST_MASK            0x00006000
#define VMCS_CR4_GUEST_HOST_MASK            0x00006002
#define VMCS_CR0_READ_SHADOW                0x00006004
#define VMCS_CR4_READ_SHADOW                0x00006006

/* 32-Bit Read-Only Data Fields */
#define VMCS_VM_INSTRUCTION_ERROR           0x00004400
#define VMCS_VM_EXIT_REASON                 0x00004402
#define VMCS_VM_EXIT_INTR_INFO              0x00004404
#define VMCS_VM_EXIT_INSTRUCTION_LEN        0x0000440C

/* 32-Bit Guest-State Fields */
#define VMCS_GUEST_ES_LIMIT                 0x00004800
#define VMCS_GUEST_CS_LIMIT                 0x00004802
#define VMCS_GUEST_SS_LIMIT                 0x00004804
#define VMCS_GUEST_DS_LIMIT                 0x00004806
#define VMCS_GUEST_FS_LIMIT                 0x00004808
#define VMCS_GUEST_GS_LIMIT                 0x0000480A
#define VMCS_GUEST_LDTR_LIMIT               0x0000480C
#define VMCS_GUEST_TR_LIMIT                 0x0000480E
#define VMCS_GUEST_GDTR_LIMIT               0x00004810
#define VMCS_GUEST_IDTR_LIMIT               0x00004812
#define VMCS_GUEST_ES_AR_BYTES              0x00004814
#define VMCS_GUEST_CS_AR_BYTES              0x00004816
#define VMCS_GUEST_SS_AR_BYTES              0x00004818
#define VMCS_GUEST_DS_AR_BYTES              0x0000481A
#define VMCS_GUEST_FS_AR_BYTES              0x0000481C
#define VMCS_GUEST_GS_AR_BYTES              0x0000481E
#define VMCS_GUEST_LDTR_AR_BYTES            0x00004820
#define VMCS_GUEST_TR_AR_BYTES              0x00004822
#define VMCS_GUEST_INTERRUPTIBILITY_INFO    0x00004824
#define VMCS_GUEST_ACTIVITY_STATE           0x00004826
#define VMCS_GUEST_SYSENTER_CS              0x0000482A

/* 32-Bit Host-State Fields */
#define VMCS_HOST_IA32_SYSENTER_CS          0x00004C00

/* Natural 64-Bit Read-Only Fields */
#define VMCS_EXIT_QUALIFICATION             0x00006400

/* Natural 64-Bit Guest-State Fields */
#define VMCS_GUEST_CR0                      0x00006800
#define VMCS_GUEST_CR3                      0x00006802
#define VMCS_GUEST_CR4                      0x00006804
#define VMCS_GUEST_ES_BASE                  0x00006806
#define VMCS_GUEST_CS_BASE                  0x00006808
#define VMCS_GUEST_SS_BASE                  0x0000680A
#define VMCS_GUEST_DS_BASE                  0x0000680C
#define VMCS_GUEST_FS_BASE                  0x0000680E
#define VMCS_GUEST_GS_BASE                  0x00006810
#define VMCS_GUEST_LDTR_BASE                0x00006812
#define VMCS_GUEST_TR_BASE                  0x00006814
#define VMCS_GUEST_GDTR_BASE                0x00006816
#define VMCS_GUEST_IDTR_BASE                0x00006818
#define VMCS_GUEST_DR7                      0x0000681A
#define VMCS_GUEST_RSP                      0x0000681C
#define VMCS_GUEST_RIP                      0x0000681E
#define VMCS_GUEST_RFLAGS                   0x00006820
#define VMCS_GUEST_PENDING_DBG_EXCEPTIONS   0x00006822
#define VMCS_GUEST_SYSENTER_ESP             0x00006824
#define VMCS_GUEST_SYSENTER_EIP             0x00006826

/* Natural 64-Bit Host-State Fields */
#define VMCS_HOST_CR0                       0x00006C00
#define VMCS_HOST_CR3                       0x00006C02
#define VMCS_HOST_CR4                       0x00006C04
#define VMCS_HOST_FS_BASE                   0x00006C06
#define VMCS_HOST_GS_BASE                   0x00006C08
#define VMCS_HOST_TR_BASE                   0x00006C0A
#define VMCS_HOST_GDTR_BASE                 0x00006C0C
#define VMCS_HOST_IDTR_BASE                 0x00006C0E
#define VMCS_HOST_IA32_SYSENTER_ESP         0x00006C10
#define VMCS_HOST_IA32_SYSENTER_EIP         0x00006C12
#define VMCS_HOST_RSP                       0x00006C14
#define VMCS_HOST_RIP                       0x00006C16

/* Standard Basic VM Exit Reasons */
#define VMX_EXIT_REASON_EXCEPTION_NMI       0
#define VMX_EXIT_REASON_EXTERNAL_INTR       1
#define VMX_EXIT_REASON_TRIPLE_FAULT        2
#define VMX_EXIT_REASON_CPUID               10
#define VMX_EXIT_REASON_HLT                 12
#define VMX_EXIT_REASON_INVD                13
#define VMX_EXIT_REASON_VMCALL              18
#define VMX_EXIT_REASON_CR_ACCESS           28
#define VMX_EXIT_REASON_IO_INSTRUCTION      30
#define VMX_EXIT_REASON_RDMSR               31
#define VMX_EXIT_REASON_WRMSR               32
#define VMX_EXIT_REASON_INVALID_GUEST_STATE 33
#define VMX_EXIT_REASON_EPT_VIOLATION       48

#endif /* ATOMS_VMX_H */
