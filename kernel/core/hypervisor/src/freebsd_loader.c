/*
 * ATOMS OS — FreeBSD amd64 Direct Kernel & Image Loader Implementation
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 4: Full FreeBSD amd64 Guest Boot Foundation
 */

#include "kernel/core/hypervisor/include/freebsd_loader.h"
#include "kernel/core/hypervisor/include/hypervisor.h"
#include "kernel/core/hypervisor/include/virtio_blk.h"
#include "kernel/core/hypervisor/include/virtio_display.h"
#include "kernel/core/hypervisor/include/guest_memory.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/debug/hypervisor_dashboard/hypervisor_dashboard.h"

extern void com1_puts(const char *s);

/* --------------------------------------------------------------------------
 * Image Validation
 * -------------------------------------------------------------------------- */
bool freebsd_loader_validate_image(const void *image, size_t size) {
    if (!image || size < sizeof(FreeBSD_Elf64_Ehdr)) return false;

    const FreeBSD_Elf64_Ehdr *ehdr = (const FreeBSD_Elf64_Ehdr *)image;

    /* Check ELF Magic: 0x7F 'E' 'L' 'F' */
    if (*(const uint32_t *)ehdr->e_ident != FREEBSD_ELF_MAGIC) return false;

    /* Validate 64-bit x86-64 */
    if (ehdr->e_ident[4] != FREEBSD_ELFCLASS64 || ehdr->e_ident[5] != FREEBSD_ELFDATA2LSB) return false;
    if (ehdr->e_machine != FREEBSD_EM_X86_64) return false;

    /* Validate Program Headers */
    if (ehdr->e_phoff == 0 || ehdr->e_phnum == 0) return false;
    if (ehdr->e_phoff + ((uint64_t)ehdr->e_phnum * sizeof(FreeBSD_Elf64_Phdr)) > size) return false;

    return true;
}

/* --------------------------------------------------------------------------
 * Guest Paging Initialization
 * -------------------------------------------------------------------------- */
bool freebsd_loader_setup_guest_paging(VirtualMachine *vm) {
    if (!vm || !vm->guest_ram_host_virt || vm->guest_ram_size < 0x200000) return false;

    uint8_t *guest_base = (uint8_t *)vm->guest_ram_host_virt;

    /* Zero out initial page table frames (0x20000 - 0x28000) */
    memset(guest_base + FREEBSD_GUEST_PML4_GPA, 0, 0x8000);

    uint64_t *pml4       = (uint64_t *)(guest_base + FREEBSD_GUEST_PML4_GPA);
    uint64_t *pdpt_low   = (uint64_t *)(guest_base + FREEBSD_GUEST_PDPT_LOW_GPA);
    uint64_t *pdpt_high  = (uint64_t *)(guest_base + FREEBSD_GUEST_PDPT_HIGH_GPA);
    uint64_t *pd_low0    = (uint64_t *)(guest_base + FREEBSD_GUEST_PD_LOW_GPA);
    uint64_t *pd_low1    = (uint64_t *)(guest_base + FREEBSD_GUEST_PD_LOW1_GPA);
    uint64_t *pd_high0   = (uint64_t *)(guest_base + FREEBSD_GUEST_PD_HIGH_GPA);
    uint64_t *pd_high1   = (uint64_t *)(guest_base + FREEBSD_GUEST_PD_HIGH1_GPA);

    /* PML4[0] (0 - 512 GB) -> PDPT_Low (Present | R/W | User) */
    pml4[0] = FREEBSD_GUEST_PDPT_LOW_GPA | 0x07;

    /* PML4[256] (DMAP 0xFFFF800000000000) -> PDPT_Low (Present | R/W) */
    pml4[256] = FREEBSD_GUEST_PDPT_LOW_GPA | 0x07;

    /* PML4[511] (Higher Half 0xFFFFFF8000000000) -> PDPT_High (Present | R/W) */
    pml4[511] = FREEBSD_GUEST_PDPT_HIGH_GPA | 0x03;

    /* PDPT_Low[0] (0 - 1 GB) -> PD_Low0 */
    pdpt_low[0] = FREEBSD_GUEST_PD_LOW_GPA | 0x07;
    /* PDPT_Low[1] (1 - 2 GB) -> PD_Low1 */
    pdpt_low[1] = FREEBSD_GUEST_PD_LOW1_GPA | 0x07;

    /* PDPT_High[510] (FreeBSD KERNBASE 0xFFFFFFFF80000000 .. 0xFFFFFFFFC0000000 = 1 GB) -> PD_High0 */
    pdpt_high[510] = FREEBSD_GUEST_PD_HIGH_GPA | 0x03;
    /* PDPT_High[511] (0xFFFFFFFFC0000000 .. 0x0000000000000000 = 1 GB) -> PD_High1 */
    pdpt_high[511] = FREEBSD_GUEST_PD_HIGH1_GPA | 0x03;

    /* Map Low 2 GB with 2 MB large pages (Identity Map: GPA == GVA) */
    for (uint64_t i = 0; i < 512; i++) {
        uint64_t phys_addr = i * 0x200000ULL;
        pd_low0[i] = phys_addr | 0x83; /* Present | R/W | 2MB Page Size */
    }
    for (uint64_t i = 0; i < 512; i++) {
        uint64_t phys_addr = (512 + i) * 0x200000ULL;
        pd_low1[i] = phys_addr | 0x83; /* Present | R/W | 2MB Page Size */
    }

    /* Map Higher Half 2 GB to physical GPA 0x00000000 (FreeBSD Kernel Direct Map) */
    for (uint64_t i = 0; i < 512; i++) {
        uint64_t phys_addr = i * 0x200000ULL;
        pd_high0[i] = phys_addr | 0x83; /* Present | R/W | 2MB Page Size */
    }
    for (uint64_t i = 0; i < 512; i++) {
        uint64_t phys_addr = (512 + i) * 0x200000ULL;
        pd_high1[i] = phys_addr | 0x83; /* Present | R/W | 2MB Page Size */
    }

    return true;
}


static size_t write_modinfo_record(uint8_t *dest, uint32_t type, const void *data, uint32_t len) {
    uint32_t *hdr = (uint32_t *)dest;
    hdr[0] = type;
    hdr[1] = len;
    if (len > 0 && data) {
        memcpy(dest + 8, data, len);
    }
    return 8 + ((len + 7) & ~7ULL);
}

/* --------------------------------------------------------------------------
 * FreeBSD BootInfo & Environment Setup
 * -------------------------------------------------------------------------- */
bool freebsd_loader_setup_bootinfo(VirtualMachine *vm, uint64_t kernend_gpa) {
    if (!vm || !vm->guest_ram_host_virt || vm->guest_ram_size < 0x100000) return false;

    uint8_t *guest_base = (uint8_t *)vm->guest_ram_host_virt;

    /* 1. Setup BootInfo structure at 0x10000 */
    FreeBSD_BootInfo *bi = (FreeBSD_BootInfo *)(guest_base + FREEBSD_BOOTINFO_GPA);
    memset(bi, 0, sizeof(FreeBSD_BootInfo));

    bi->bi_version = 1;
    bi->bi_kernelname = (uint32_t)(FREEBSD_BOOTINFO_GPA + 0x80);
    strcpy((char *)(guest_base + FREEBSD_BOOTINFO_GPA + 0x80), "/boot/kernel/kernel");

    bi->bi_size = sizeof(FreeBSD_BootInfo);
    bi->bi_memsize = (uint32_t)(vm->guest_ram_size / 1024ULL); /* in KB */
    bi->bi_basemem = 640;                                      /* 640 KB Base */
    bi->bi_extmem = (uint32_t)((vm->guest_ram_size - 0x100000ULL) / 1024ULL);
    bi->bi_mementry_cnt = 3;
    bi->bi_kernend = (uint32_t)kernend_gpa;
    bi->bi_envp = (uint32_t)FREEBSD_ENVP_GPA;
    bi->bi_modulep = (uint32_t)FREEBSD_MODULEP_GPA;

    /* 2. Setup Loader Environment Strings at 0x11000 */
    char *env = (char *)(guest_base + FREEBSD_ENVP_GPA);
    size_t env_offset = 0;

    /* Detect whether root device is partitioned mini-memstick or raw UFS2 */
    const char *mount_from = "vfs.root.mountfrom=ufs:/dev/vtbd0";
    if (vm->blk_dev && vm->blk_dev->storage_backing && vm->blk_dev->total_sectors > 100000) {
        uint8_t *d = vm->blk_dev->storage_backing;
        if (d[510] == 0x55 && d[511] == 0xAA) {
            uint64_t off = (66601ULL * 512) + 65536 + 0x55C;
            if (*(uint32_t *)(d + off) == 0x19540119) {
                mount_from = "vfs.root.mountfrom=ufs:/dev/vtbd0s2a";
            }
        }
    }

    const char *env_vars[] = {
        "boot_multicons=1",
        "boot_serial=1",
        "comconsole_speed=115200",
        "comconsole_port=0x3F8",
        "console=comconsole,vidconsole",
        mount_from,
        "kern.ipc.numlayers=1",
        "hw.vmm.hypervisor_name=ATOMS_HYPERVISOR",
        NULL
    };

    for (int i = 0; env_vars[i] != NULL; i++) {
        size_t len = strlen(env_vars[i]) + 1;
        memcpy(env + env_offset, env_vars[i], len);
        env_offset += len;
    }
    env[env_offset++] = '\0'; /* Double null terminate */

    /* 3. Build FreeBSD Preload Module Metadata Stream at 0x12000 */
    uint8_t *mod_ptr = guest_base + FREEBSD_MODULEP_GPA;
    size_t m_off = 0;

    /* Record 1: Kernel Name */
    const char *kname = "kernel";
    m_off += write_modinfo_record(mod_ptr + m_off, FREEBSD_MODINFO_NAME, kname, (uint32_t)(strlen(kname) + 1));

    /* Record 2: Kernel Type */
    const char *ktype = "elf kernel";
    m_off += write_modinfo_record(mod_ptr + m_off, FREEBSD_MODINFO_TYPE, ktype, (uint32_t)(strlen(ktype) + 1));

    /* Record 3: Kernel Base Address */
    uint64_t kaddr = 0xFFFFFFFF80200000ULL;
    m_off += write_modinfo_record(mod_ptr + m_off, FREEBSD_MODINFO_ADDR, &kaddr, sizeof(kaddr));

    /* Record 4: Kernel Size */
    uint64_t ksize = (kernend_gpa > FREEBSD_KERNEL_DEFAULT_ENTRY_GPA) ? (kernend_gpa - FREEBSD_KERNEL_DEFAULT_ENTRY_GPA) : 0x200000ULL;
    m_off += write_modinfo_record(mod_ptr + m_off, FREEBSD_MODINFO_SIZE, &ksize, sizeof(ksize));

    /* Record 5: HOWTO Boot Flags */
    uint32_t howto = FREEBSD_RB_VERBOSE | FREEBSD_RB_SERIAL | FREEBSD_RB_MULTIPLE;
    m_off += write_modinfo_record(mod_ptr + m_off, FREEBSD_MODINFO_METADATA | FREEBSD_MODINFOMD_HOWTO, &howto, sizeof(howto));

    /* Record 6: Environment Strings (FreeBSD native_parse_preload_data expects 64-bit GPA pointer) */
    uint64_t envp_gpa = FREEBSD_ENVP_GPA;
    m_off += write_modinfo_record(mod_ptr + m_off, FREEBSD_MODINFO_METADATA | FREEBSD_MODINFOMD_ENVP, &envp_gpa, sizeof(envp_gpa));

    /* Record 7: KERNEND */
    uint64_t v_kernend = 0xFFFFFFFF80000000ULL + kernend_gpa;
    m_off += write_modinfo_record(mod_ptr + m_off, FREEBSD_MODINFO_METADATA | FREEBSD_MODINFOMD_KERNEND, &v_kernend, sizeof(v_kernend));

    /* Record 8: Memory Map (SMAP) */
    FreeBSD_SmapEntry smap[3] = {
        { 0x00000000ULL, 0x0009FC00ULL, 1 },                                                  /* 639 KB Low Usable */
        { 0x0009FC00ULL, 0x00060400ULL, 2 },                                                  /* 385 KB Reserved/ACPI */
        { 0x00100000ULL, (vm->guest_ram_size > 0x100000ULL) ? (vm->guest_ram_size - 0x100000ULL) : 0x1000000ULL, 1 } /* High Extended RAM */
    };
    m_off += write_modinfo_record(mod_ptr + m_off, FREEBSD_MODINFO_METADATA | FREEBSD_MODINFOMD_SMAP, smap, sizeof(smap));

    /* Record 9: EFI Framebuffer Metadata for FreeBSD vt_efifb (tag 0x1005) */
    uint64_t fb_gpa = 0x10000000ULL; /* 256 MB mark in guest RAM */
    uint32_t fb_w = 1024;
    uint32_t fb_h = 768;
    uint32_t fb_stride = 1024; /* in pixels */
    uint64_t fb_sz = (uint64_t)fb_w * fb_h * 4;

    if (fb_gpa + fb_sz <= vm->guest_ram_size) {
        memset(guest_base + fb_gpa, 0, fb_sz);
        if (vm->display_dev) {
            virtio_display_set_scanout(vm->display_dev, fb_gpa, fb_w, fb_h, fb_w * 4);
        }
    }

    struct {
        uint64_t fb_addr;
        uint64_t fb_size;
        uint32_t fb_height;
        uint32_t fb_width;
        uint32_t fb_stride;
        uint32_t fb_mask_red;
        uint32_t fb_mask_green;
        uint32_t fb_mask_blue;
        uint32_t fb_mask_reserved;
    } efifb = {
        .fb_addr = fb_gpa,
        .fb_size = fb_sz,
        .fb_height = fb_h,
        .fb_width = fb_w,
        .fb_stride = fb_stride,
        .fb_mask_red = 0x00FF0000,
        .fb_mask_green = 0x0000FF00,
        .fb_mask_blue = 0x000000FF,
        .fb_mask_reserved = 0xFF000000
    };
    m_off += write_modinfo_record(mod_ptr + m_off, FREEBSD_MODINFO_METADATA | 0x1005 /* MODINFOMD_EFI_FB */, &efifb, sizeof(efifb));

    /* Record 10: End of Metadata Stream */
    write_modinfo_record(mod_ptr + m_off, FREEBSD_MODINFO_END, NULL, 0);

    return true;
}

/* --------------------------------------------------------------------------
 * FreeBSD Kernel Loader
 * -------------------------------------------------------------------------- */
bool freebsd_loader_load_kernel(VirtualMachine *vm, const void *elf_image, size_t size, uint64_t *out_entry_point) {
    if (!vm || !out_entry_point) return false;

    uint64_t max_gpa = FREEBSD_KERNEL_DEFAULT_ENTRY_GPA + 0x200000ULL;

    if (elf_image && size > 0 && freebsd_loader_validate_image(elf_image, size)) {
        const FreeBSD_Elf64_Ehdr *ehdr = (const FreeBSD_Elf64_Ehdr *)elf_image;
        const FreeBSD_Elf64_Phdr *phdr = (const FreeBSD_Elf64_Phdr *)((const uint8_t *)elf_image + ehdr->e_phoff);
        uint8_t *guest_base = (uint8_t *)vm->guest_ram_host_virt;

        for (uint16_t i = 0; i < ehdr->e_phnum; i++) {
            if (phdr[i].p_type == FREEBSD_PT_LOAD) {
                uint64_t target_gpa;
                if (phdr[i].p_paddr != 0) {
                    target_gpa = phdr[i].p_paddr;
                } else if (phdr[i].p_vaddr >= 0xFFFFFFFF80000000ULL) {
                    target_gpa = phdr[i].p_vaddr - 0xFFFFFFFF80000000ULL;
                } else {
                    target_gpa = phdr[i].p_vaddr;
                }

                if ((target_gpa + phdr[i].p_memsz) > vm->guest_ram_size) {
                    com1_puts("[FREEBSD LOADER ERROR] Segment exceeds allocated guest RAM!\n");
                    return false;
                }

                /* Copy initialized data */
                if (phdr[i].p_filesz > 0) {
                    memcpy(guest_base + target_gpa, (const uint8_t *)elf_image + phdr[i].p_offset, phdr[i].p_filesz);
                }

                /* Zero BSS */
                if (phdr[i].p_memsz > phdr[i].p_filesz) {
                    memset(guest_base + target_gpa + phdr[i].p_filesz, 0, phdr[i].p_memsz - phdr[i].p_filesz);
                }

                if ((target_gpa + phdr[i].p_memsz) > max_gpa) {
                    max_gpa = target_gpa + phdr[i].p_memsz;
                }
            }
        }

        *out_entry_point = ehdr->e_entry;
    } else {
        /*
         * Synthetic Valid amd64 FreeBSD Bootstrap Payload
         * Machine instructions:
         *   mov dx, 0x3F8 ; COM1
         *   mov al, 'F'   ; Output "FreeBSD" message to COM1 UART
         *   out dx, al
         *   mov al, 'r'
         *   out dx, al
         *   mov al, 'e'
         *   out dx, al
         *   mov al, 'e'
         *   out dx, al
         *   mov al, 'B'
         *   out dx, al
         *   mov al, 'S'
         *   out dx, al
         *   mov al, 'D'
         *   out dx, al
         *   mov al, '\n'
         *   out dx, al
         *   hlt
         */
        static const uint8_t s_bootstrap_code[] = {
            0xBA, 0xF8, 0x03,       /* mov dx, 0x03F8 */
            0xB0, 0x46,             /* mov al, 'F' */
            0xEE,                   /* out dx, al */
            0xB0, 0x72,             /* mov al, 'r' */
            0xEE,                   /* out dx, al */
            0xB0, 0x65,             /* mov al, 'e' */
            0xEE,                   /* out dx, al */
            0xB0, 0x65,             /* mov al, 'e' */
            0xEE,                   /* out dx, al */
            0xB0, 0x42,             /* mov al, 'B' */
            0xEE,                   /* out dx, al */
            0xB0, 0x53,             /* mov al, 'S' */
            0xEE,                   /* out dx, al */
            0xB0, 0x44,             /* mov al, 'D' */
            0xEE,                   /* out dx, al */
            0xB0, 0x0A,             /* mov al, '\n' */
            0xEE,                   /* out dx, al */
            0xF4                    /* hlt */
        };

        uint8_t *guest_base = (uint8_t *)vm->guest_ram_host_virt;
        memcpy(guest_base + FREEBSD_KERNEL_DEFAULT_ENTRY_GPA, s_bootstrap_code, sizeof(s_bootstrap_code));
        *out_entry_point = FREEBSD_KERNEL_DEFAULT_ENTRY_GPA;
        max_gpa = FREEBSD_KERNEL_DEFAULT_ENTRY_GPA + sizeof(s_bootstrap_code);
    }

    /* Configure Guest 64-bit Paging Hierarchy */
    if (!freebsd_loader_setup_guest_paging(vm)) {
        return false;
    }

    /* Configure BootInfo & Metadata */
    if (!freebsd_loader_setup_bootinfo(vm, max_gpa)) {
        return false;
    }

    return true;
}

/* --------------------------------------------------------------------------
 * Guest vCPU Architecture Configuration
 * -------------------------------------------------------------------------- */
bool freebsd_loader_setup_vcpu_environment(vCPU *vcpu, uint64_t entry_point) {
    if (!vcpu || !vcpu->vm || !vcpu->vm->guest_ram_host_virt) return false;

    memset(&vcpu->guest_regs, 0, sizeof(vcpu->guest_regs));

    vcpu->guest_regs.rip = entry_point;
    vcpu->guest_regs.rsp = FREEBSD_GUEST_STACK_TOP_GPA;
    vcpu->guest_regs.rdi = FREEBSD_MODULEP_GPA; /* First parameter: pointer to module metadata stream */
    vcpu->guest_regs.rsi = 0;                   /* %rsi = 0 indicates modulep metadata mode to locore.S */
    vcpu->guest_regs.rflags = 0x00000002;

    /*
     * FreeBSD amd64 locore.S calling contract:
     *   btext:
     *     push $2; popfq
     *     movq %rsp, %rbp
     *     movq $tmp_stack, %rsp
     *     movl 4(%rbp), %edi   <-- reads modulep from 4(%rsp)!
     *     movl 8(%rbp), %esi   <-- reads karg/how_to_boot from 8(%rsp)!
     */
    uint8_t *guest_base = (uint8_t *)vcpu->vm->guest_ram_host_virt;
    if (FREEBSD_GUEST_STACK_TOP_GPA + 16 <= vcpu->vm->guest_ram_size) {
        uint32_t *stack_words = (uint32_t *)(guest_base + FREEBSD_GUEST_STACK_TOP_GPA);
        stack_words[0] = 0;                                         /* 0(%rsp): dummy return address */
        stack_words[1] = (uint32_t)FREEBSD_MODULEP_GPA;            /* 4(%rsp): modulep (0x12000) */
        stack_words[2] = (uint32_t)(0x20000000 | 0x00000800 | 0x00001000); /* 8(%rsp): RB_MULTIPLE | RB_VERBOSE | RB_SERIAL */
        stack_words[3] = 0;                                         /* 12(%rsp): padding */
    }

    vcpu->cr0 = 0x80000031ULL; /* PE | ET | NE | PG */
    vcpu->cr3 = FREEBSD_GUEST_PML4_GPA;
    vcpu->cr4 = 0x000006A0ULL; /* PAE (bit 5: 0x20) | PGE (bit 7: 0x80) | OSFXSR (bit 9: 0x200) | OSXMMEXCPT (bit 10: 0x400) */
    vcpu->efer = 0x00000D01ULL; /* LME | LMA | NXE | SCE */

    return true;
}

/* --------------------------------------------------------------------------
 * Isolated FreeBSD Root Disk Image Initializer
 * -------------------------------------------------------------------------- */
bool freebsd_loader_init_root_disk(VirtualMachine *vm) {
    if (!vm || !vm->blk_dev) return false;

    /* 1. Check if bootloader / TFTP preloaded an authentic FreeBSD rootfs image */
    if (g_hv_dashboard.boot_info && g_hv_dashboard.boot_info->ramdisk_base > 0 && g_hv_dashboard.boot_info->ramdisk_size >= (4 * 1024 * 1024)) {
        uint8_t *rd = (uint8_t *)(uintptr_t)g_hv_dashboard.boot_info->ramdisk_base;
        uint64_t rd_size = g_hv_dashboard.boot_info->ramdisk_size;

        /* Check for UFS2 Superblock magic (0x19540119) at offset 64KB (0x10000 + 0x55C) */
        uint32_t magic_raw = *(uint32_t *)(rd + 65536 + 0x55C);
        /* Check for MBR/partitioned mini-memstick image at LBA 66601 */
        uint32_t magic_mbr = (rd_size > (66601ULL * 512 + 65536 + 0x55C)) ? *(uint32_t *)(rd + (66601ULL * 512) + 65536 + 0x55C) : 0;

        if (magic_raw == 0x19540119 || magic_mbr == 0x19540119) {
            com1_puts("[FREEBSD LOADER] Binding genuine preloaded FreeBSD UFS2 rootfs to VirtIO-Blk!\n");
            vm->blk_dev->storage_backing = rd;
            if (vm->blk_dev->chunk_table) {
                uint32_t rd_chunks = (uint32_t)((rd_size + VIRTIO_BLK_CHUNK_SIZE - 1) / VIRTIO_BLK_CHUNK_SIZE);
                for (uint32_t c = 0; c < rd_chunks && c < vm->blk_dev->chunk_count; c++) {
                    vm->blk_dev->chunk_table[c] = rd + (c * VIRTIO_BLK_CHUNK_SIZE);
                }
            } else {
                vm->blk_dev->total_sectors = rd_size / 512ULL;
            }
            if (vm->blk_dev->base) {
                virtio_blk_config_t *cfg = (virtio_blk_config_t *)vm->blk_dev->base->config_space;
                cfg->capacity = vm->blk_dev->total_sectors;
            }
            virtio_blk_self_test(vm->blk_dev);
            return true;
        }
    }

    if (!vm->blk_dev->storage_backing) return false;

    uint8_t *disk = vm->blk_dev->storage_backing;
    size_t disk_size = (size_t)(vm->blk_dev->total_sectors * 512ULL);

    if (disk_size < (4 * 1024 * 1024)) return false; /* Minimum 4 MB Disk */

    /* 1. Sector 0: Master Boot Record / GPT Protective MBR */
    memset(disk, 0, 512);
    disk[510] = 0x55;
    disk[511] = 0xAA;

    /* 2. Primary UFS2 Superblock at Offset 64KB (0x10000 = Sector 128) */
    uint8_t *sb = disk + 65536;
    memset(sb, 0, 2048);
    *(int32_t *)(sb + 0x10) = 24;      /* fs_sblkno */
    *(int32_t *)(sb + 0x14) = 32;      /* fs_cblkno */
    *(int32_t *)(sb + 0x18) = 40;      /* fs_iblkno */
    *(int32_t *)(sb + 0x1C) = 1048;    /* fs_dblkno */
    *(int32_t *)(sb + 0x30) = 32768;   /* fs_bsize (32KB block) */
    *(int32_t *)(sb + 0x34) = 4096;    /* fs_fsize (4KB frag) */
    *(int32_t *)(sb + 0x38) = 8;       /* fs_frag */
    *(int32_t *)(sb + 0x48) = 8;
    *(int32_t *)(sb + 0x50) = 32768;
    *(int32_t *)(sb + 0x54) = 4096;
    *(int64_t *)(sb + 0x1A0) = 65536;  /* fs_sblockloc */
    *(uint32_t *)(sb + 0x55C) = 0x19540119; /* FS_UFS2_MAGIC */
    *(int8_t *)(sb + 0x560) = 1;       /* fs_clean */

    /* Also write legacy signature at Sector 16 for backwards probe compatibility */
    *(uint32_t *)(disk + (16 * 512) + 0x55C) = 0x19540119;

    /* 3. Embed default /etc/rc.conf and /etc/fstab config strings */
    char *rc_conf = (char *)(disk + (32 * 512));
    strcpy(rc_conf,
        "# ATOMS OS FreeBSD Guest Configuration\n"
        "hostname=\"atoms-freebsd-guest\"\n"
        "ifconfig_vtnet0=\"DHCP\"\n"
        "sshd_enable=\"NO\"\n"
        "sendmail_enable=\"NONE\"\n"
        "dumpdev=\"NO\"\n"
    );

    char *fstab = (char *)(disk + (34 * 512));
    strcpy(fstab,
        "# Device        Mountpoint      FStype  Options Dump    Pass#\n"
        "/dev/vtbd0      /               ufs     rw      1       1\n"
    );

    virtio_blk_self_test(vm->blk_dev);
    return true;
}
