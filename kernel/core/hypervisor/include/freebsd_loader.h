/*
 * ATOMS OS — FreeBSD amd64 Direct Kernel & Image Loader
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 4: Full FreeBSD amd64 Guest Boot Foundation
 */

#ifndef ATOMS_FREEBSD_LOADER_H
#define ATOMS_FREEBSD_LOADER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct atoms_vm VirtualMachine;
typedef struct vcpu vCPU;

/* FreeBSD ELF Constants */
#define FREEBSD_ELF_MAGIC                   0x464C457FU /* "\x7FELF" */
#define FREEBSD_ELFCLASS64                  2
#define FREEBSD_ELFDATA2LSB                 1
#define FREEBSD_EM_X86_64                   62
#define FREEBSD_ELFOSABI_FREEBSD            9
#define FREEBSD_ELFOSABI_NONE               0

#define FREEBSD_PT_LOAD                     1
#define FREEBSD_PF_X                        1
#define FREEBSD_PF_W                        2
#define FREEBSD_PF_R                        4

/* Guest Physical Address Map for FreeBSD Bootloader Handover */
#define FREEBSD_BOOTINFO_GPA                0x00010000ULL
#define FREEBSD_ENVP_GPA                    0x00011000ULL
#define FREEBSD_MODULEP_GPA                 0x00012000ULL
#define FREEBSD_GUEST_PML4_GPA              0x00020000ULL
#define FREEBSD_GUEST_PDPT_LOW_GPA          0x00021000ULL
#define FREEBSD_GUEST_PDPT_HIGH_GPA         0x00022000ULL
#define FREEBSD_GUEST_PD_LOW_GPA            0x00023000ULL
#define FREEBSD_GUEST_PD_HIGH_GPA           0x00024000ULL
#define FREEBSD_GUEST_STACK_TOP_GPA         0x0007FF00ULL
#define FREEBSD_KERNEL_DEFAULT_ENTRY_GPA    0x00200000ULL

/* FreeBSD Preload Metadata Tag Types (sys/sys/linker.h) */
#define FREEBSD_MODINFO_END                 0x0000
#define FREEBSD_MODINFO_NAME                0x0001
#define FREEBSD_MODINFO_TYPE                0x0002
#define FREEBSD_MODINFO_ADDR                0x0003
#define FREEBSD_MODINFO_SIZE                0x0004
#define FREEBSD_MODINFO_EMPTY               0x0005
#define FREEBSD_MODINFO_ARGS                0x0006
#define FREEBSD_MODINFO_METADATA            0x8000

/* FreeBSD Metadata Sub-types (sys/sys/linker.h & sys/x86/include/metadata.h) */
#define FREEBSD_MODINFOMD_AOUTEXEC          0x0001
#define FREEBSD_MODINFOMD_ELFHDR            0x0002
#define FREEBSD_MODINFOMD_SSYM              0x0003
#define FREEBSD_MODINFOMD_ESYM              0x0004
#define FREEBSD_MODINFOMD_DYNAMIC           0x0005
#define FREEBSD_MODINFOMD_ENVP              0x0006
#define FREEBSD_MODINFOMD_HOWTO             0x0007
#define FREEBSD_MODINFOMD_KERNEND           0x0008
#define FREEBSD_MODINFOMD_SHDR              0x0009
#define FREEBSD_MODINFOMD_NOCOPY            0x000a

#define FREEBSD_MODINFOMD_SMAP              0x1001
#define FREEBSD_MODINFOMD_SMAP_XATTR        0x1002
#define FREEBSD_MODINFOMD_DTBP              0x1003
#define FREEBSD_MODINFOMD_EFI_MAP           0x1004
#define FREEBSD_MODINFOMD_EFI_FB            0x1005
#define FREEBSD_MODINFOMD_MODULEP           0x1006

/* Boot Flags in HOWTO (sys/sys/reboot.h) */
#define FREEBSD_RB_ASKNAME                  0x00000001
#define FREEBSD_RB_SINGLE                   0x00000002
#define FREEBSD_RB_NOSYNC                   0x00000004
#define FREEBSD_RB_HALT                     0x00000008
#define FREEBSD_RB_INITNAME                 0x00000010
#define FREEBSD_RB_DFLTROOT                 0x00000020
#define FREEBSD_RB_KDB                      0x00000040
#define FREEBSD_RB_GDB                      0x00000080
#define FREEBSD_RB_MUTE                     0x00000100
#define FREEBSD_RB_SELFTEST                 0x00000200
#define FREEBSD_RB_VERBOSE                  0x00000800
#define FREEBSD_RB_SERIAL                   0x00001000
#define FREEBSD_RB_CDROM                    0x00002000
#define FREEBSD_RB_POWEROFF                 0x00004000
#define FREEBSD_RB_GDBLOOP                  0x00008000
#define FREEBSD_RB_PAUSE                    0x00040000
#define FREEBSD_RB_MULTIPLE                 0x20000000

/* FreeBSD SMAP (E820 Memory Entry) */
typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
} __attribute__((packed)) FreeBSD_SmapEntry;

/* FreeBSD Standard BootInfo Structure */
typedef struct {
    uint32_t bi_version;
    uint32_t bi_kernelname;     /* GPA to string */
    uint32_t bi_nfs_diskless;
    uint32_t bi_n_bios_used;
    uint32_t bi_bios_geom[8];
    uint32_t bi_size;
    uint8_t  bi_mementry_cnt;
    uint8_t  bi_mementry_pad[3];
    uint32_t bi_memsize;        /* Total RAM in KB */
    uint32_t bi_basemem;        /* Base RAM (640 KB) */
    uint32_t bi_extmem;         /* Extended RAM in KB */
    uint32_t bi_symtab;
    uint32_t bi_esymtab;
    uint32_t bi_kernend;        /* End GPA of loaded kernel */
    uint32_t bi_envp;           /* GPA of loader environment */
    uint32_t bi_modulep;        /* GPA of module metadata */
} __attribute__((packed)) FreeBSD_BootInfo;

/* FreeBSD 64-bit ELF Header */
typedef struct {
    uint8_t  e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed)) FreeBSD_Elf64_Ehdr;

/* FreeBSD 64-bit Program Header */
typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} __attribute__((packed)) FreeBSD_Elf64_Phdr;

/* FreeBSD Loader Core APIs */
bool freebsd_loader_validate_image(const void *image, size_t size);
bool freebsd_loader_load_kernel(VirtualMachine *vm, const void *elf_image, size_t size, uint64_t *out_entry_point);
bool freebsd_loader_setup_guest_paging(VirtualMachine *vm);
bool freebsd_loader_setup_bootinfo(VirtualMachine *vm, uint64_t kernend_gpa);
bool freebsd_loader_setup_vcpu_environment(vCPU *vcpu, uint64_t entry_gpa);

/* Isolated FreeBSD Root Disk Image Initializer */
bool freebsd_loader_init_root_disk(VirtualMachine *vm);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_FREEBSD_LOADER_H */
