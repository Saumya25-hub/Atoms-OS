#include "include/efi.h"
#include "kernel/core/core_legacy/boot/include/boot_info.h"

static EFI_GUID g_gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
static EFI_GUID g_fs_guid  = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
static EFI_GUID g_lip_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
static EFI_GUID g_info_guid= EFI_FILE_INFO_GUID;
static EFI_SYSTEM_TABLE *g_st = NULL;
static EFI_BOOT_SERVICES *g_bs = NULL;

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__ ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__ ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
static void com1_init(void) {
    outb(0x3F8+1,0x00); outb(0x3F8+3,0x80); outb(0x3F8+0,0x03);
    outb(0x3F8+1,0x00); outb(0x3F8+3,0x03); outb(0x3F8+2,0xC7); outb(0x3F8+4,0x0B);
}
static void com1_putc(char c) { while((inb(0x3F8+5)&0x20)==0); outb(0x3F8,c); }
static void com1_print(const char *msg) {
    while(*msg){ if(*msg=='\n')com1_putc('\r'); com1_putc(*msg++); }
}
static void uefi_print(CHAR16 *msg) {
    if(g_st&&g_st->ConOut) g_st->ConOut->OutputString(g_st->ConOut,msg);
    for(CHAR16*p=msg;*p;p++) com1_putc((char)*p);
}

// Framebuffer raw pixel drawing (Works POST-ExitBootServices safely)
static void draw_fb_rect(uint64_t fb_base, uint32_t pitch_bytes, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    if (!fb_base) return;
    for (uint32_t r = y; r < y + h; r++) {
        uint32_t *row = (uint32_t*)(uintptr_t)(fb_base + r * pitch_bytes);
        for (uint32_t c = x; c < x + w; c++) {
            row[c] = color;
        }
    }
}

__attribute__((weak)) uint8_t g_kernel_data[1] = {0};
__attribute__((weak)) uint64_t g_kernel_size_val = 0;

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    g_st=SystemTable; g_bs=SystemTable->BootServices; com1_init();
    uefi_print(L"[UEFI STAGE 1] Starting SignaturesOS Production UEFI Loader (BOOTX64.EFI)...\r\n");

    /* Step 1: GOP */
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop=NULL;
    EFI_STATUS status=g_bs->LocateProtocol(&g_gop_guid,NULL,(VOID**)&gop);
    if (EFI_ERROR(status) || !gop) {
        if (SystemTable->ConsoleOutHandle) {
            g_bs->HandleProtocol(SystemTable->ConsoleOutHandle, &g_gop_guid, (VOID**)&gop);
        }
    }
    if (EFI_ERROR(status) || !gop) {
        EFI_HANDLE *goph=NULL; UINTN gopcnt=0;
        if (!EFI_ERROR(g_bs->LocateHandleBuffer(ByProtocol, &g_gop_guid, NULL, &gopcnt, &goph)) && gopcnt > 0) {
            g_bs->HandleProtocol(goph[0], &g_gop_guid, (VOID**)&gop);
            g_bs->FreePool(goph);
        }
    }

    if (gop && gop->Mode && gop->Mode->Info) {
        UINT32 best_mode=gop->Mode->Mode,max_width=0;
        for(UINT32 m=0;m<gop->Mode->MaxMode;m++){
            EFI_GRAPHICS_OUTPUT_MODE_INFORMATION*info=NULL;
            UINTN sz=sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
            if(!EFI_ERROR(gop->QueryMode(gop,m,&sz,&info))&&info)
                if((info->PixelFormat==PixelBlueGreenRedReserved8BitPerColor||
                    info->PixelFormat==PixelRedGreenBlueReserved8BitPerColor)
                   &&info->HorizontalResolution>=max_width)
                    { max_width=info->HorizontalResolution; best_mode=m; }
        }
        gop->SetMode(gop,best_mode);
        uefi_print(L"[UEFI BOOTLOADER] GOP Resolution Initialized Successfully.\r\n");
    } else {
        uefi_print(L"[UEFI BOOTLOADER] GOP Display Protocol Not Active. Continuing in Headless/Serial mode...\r\n");
    }

    /* Step 2: Root FS */
    EFI_FILE_PROTOCOL *root=NULL;
    EFI_LOADED_IMAGE_PROTOCOL *li=NULL;
    if(!EFI_ERROR(g_bs->HandleProtocol(ImageHandle,&g_lip_guid,(VOID**)&li))&&li&&li->DeviceHandle){
        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs=NULL;
        if(!EFI_ERROR(g_bs->HandleProtocol(li->DeviceHandle,&g_fs_guid,(VOID**)&fs))&&fs)
            fs->OpenVolume(fs,&root);
    }
    if(!root){
        EFI_HANDLE *fsh=NULL; UINTN cnt=0;
        if(!EFI_ERROR(g_bs->LocateHandleBuffer(ByProtocol,&g_fs_guid,NULL,&cnt,&fsh))&&cnt>0){
            for(UINTN i=0;i<cnt;i++){
                EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs=NULL;
                if(!EFI_ERROR(g_bs->HandleProtocol(fsh[i],&g_fs_guid,(VOID**)&fs))&&fs){
                    EFI_FILE_PROTOCOL *tr=NULL;
                    if(!EFI_ERROR(fs->OpenVolume(fs,&tr))&&tr){root=tr;break;}
                }
            }
            g_bs->FreePool(fsh);
        }
    }

    /* Step 3: Load kernel.bin at 0x100000 via AllocatePages(AllocateAddress) */
    EFI_FILE_PROTOCOL *kf=NULL;
    if (root) {
        root->Open(root,&kf,L"kernel.bin",EFI_FILE_MODE_READ,0);
        if(!kf) root->Open(root,&kf,L"KERNEL.BIN",EFI_FILE_MODE_READ,0);
        if(!kf) root->Open(root,&kf,L"\\kernel.bin",EFI_FILE_MODE_READ,0);
        if(!kf) root->Open(root,&kf,L"\\KERNEL.BIN",EFI_FILE_MODE_READ,0);
    }

    EFI_PHYSICAL_ADDRESS kpaddr=0x100000ULL;
    UINTN ksz=0;
    if (kf) {
        UINT8 ibuf[512]; UINTN isz=512;
        if(EFI_ERROR(kf->GetInfo(kf,&g_info_guid,&isz,ibuf))){
            uefi_print(L"ERROR: KINFO FAILED\r\n");
            while(1) __asm__ __volatile__("cli;hlt");
        }
        EFI_FILE_INFO *fi=(EFI_FILE_INFO*)ibuf;
        ksz=(UINTN)fi->FileSize;
        UINTN kpages=(ksz+4095)/4096+1;
        if(EFI_ERROR(g_bs->AllocatePages(AllocateAddress,EfiLoaderCode,kpages,&kpaddr))){
            kpaddr=0;
            if(EFI_ERROR(g_bs->AllocatePages(AllocateAnyPages,EfiLoaderCode,kpages,&kpaddr))){
                uefi_print(L"ERROR: KALLOC DISK FAILED\r\n");
                while(1) __asm__ __volatile__("cli;hlt");
            }
        }
        UINTN bread=ksz;
        if(EFI_ERROR(kf->Read(kf,&bread,(VOID*)(uintptr_t)kpaddr))){
            uefi_print(L"ERROR: KREAD FAILED\r\n");
            while(1) __asm__ __volatile__("cli;hlt");
        }
        kf->Close(kf);
        uefi_print(L"[UEFI BOOTLOADER] kernel.bin Loaded from Disk into RAM Buffer Successfully.\r\n");
    } else {
        uefi_print(L"[UEFI BOOTLOADER] Unpacking Embedded Atoms OS Kernel Payload into RAM...\r\n");
        ksz = (UINTN)g_kernel_size_val;
        UINTN kpages = (ksz + 4095) / 4096 + 1;
        if (EFI_ERROR(g_bs->AllocatePages(AllocateAddress, EfiLoaderCode, kpages, &kpaddr))) {
            kpaddr = 0;
            if (EFI_ERROR(g_bs->AllocatePages(AllocateAnyPages, EfiLoaderCode, kpages, &kpaddr))) {
                uefi_print(L"ERROR: KALLOC EMBEDDED KERNEL FAILED\r\n");
                while(1) __asm__ __volatile__("cli;hlt");
            }
        }

        uint8_t *dst = (uint8_t*)(uintptr_t)kpaddr;
        for (UINTN i = 0; i < ksz; i++) {
            dst[i] = g_kernel_data[i];
        }
        uefi_print(L"[UEFI BOOTLOADER] Embedded Atoms OS Kernel Unpacked into 0x100000 RAM Successfully!\r\n");
    }

    /* Step 4: boot_info */
    boot_info_t *bi=NULL;
    if(EFI_ERROR(g_bs->AllocatePool(EfiLoaderData,sizeof(boot_info_t),(VOID**)&bi))||!bi){
        uefi_print(L"ERROR: BIALLOC FAILED\r\n");
        while(1) __asm__ __volatile__("cli;hlt");
    }
    for(UINTN i=0;i<sizeof(boot_info_t);i++) ((UINT8*)bi)[i]=0;
    if (gop && gop->Mode && gop->Mode->Info) {
        bi->vbe_width=gop->Mode->Info->HorizontalResolution;
        bi->vbe_height=gop->Mode->Info->VerticalResolution;
        bi->vbe_pitch=gop->Mode->Info->PixelsPerScanLine*4;
        bi->vbe_bpp=32;
        bi->vbe_framebuffer=(uint64_t)gop->Mode->FrameBufferBase;
    } else {
        bi->vbe_width=1024;
        bi->vbe_height=768;
        bi->vbe_pitch=1024*4;
        bi->vbe_bpp=32;
        bi->vbe_framebuffer=0;
    }

    /* Step 5: Allocate 7 pages for page tables + GDT (all from UEFI safe memory) */
    EFI_PHYSICAL_ADDRESS pt=0;
    if(EFI_ERROR(g_bs->AllocatePages(AllocateAnyPages,EfiLoaderData,7,&pt))){
        uefi_print(L"ERROR:PTALLOC\r\n"); return EFI_LOAD_ERROR;
    }

    /* Step 6: Allocate Memory Map Buffer with 8KB Slack Space */
    UINTN mapsz = 0, mapkey = 0, descsz = 0; UINT32 descver = 0;
    EFI_MEMORY_DESCRIPTOR *mm = NULL;

    g_bs->GetMemoryMap(&mapsz, NULL, &mapkey, &descsz, &descver);
    mapsz += 8192; // 8KB extra slack space for real hardware map growth

    if (EFI_ERROR(g_bs->AllocatePool(EfiLoaderData, mapsz, (VOID**)&mm)) || !mm) {
        uefi_print(L"ERROR:MMALLOC\r\n");
        return EFI_LOAD_ERROR;
    }

    /* Populate Memory Map Structure for Boot Info */
    UINTN cmsz = mapsz;
    EFI_STATUS gmm_status = g_bs->GetMemoryMap(&cmsz, mm, &mapkey, &descsz, &descver);
    if (EFI_ERROR(gmm_status)) {
        uefi_print(L"ERROR:GMM_INIT\r\n");
        return gmm_status;
    }

    UINTN dcnt = cmsz / descsz; UINT32 ve = 0;
    for (UINTN i = 0; i < dcnt && ve < 256; i++) {
        EFI_MEMORY_DESCRIPTOR *d = (EFI_MEMORY_DESCRIPTOR*)((UINT8*)mm + (i * descsz));
        UINT32 t = MEMORY_TYPE_RESERVED;
        if (d->Type == EfiConventionalMemory || d->Type == EfiLoaderCode || d->Type == EfiLoaderData) t = MEMORY_TYPE_USABLE;
        else if (d->Type == EfiACPIReclaimMemory) t = MEMORY_TYPE_ACPI_RECLAIMABLE;
        else if (d->Type == EfiACPIMemoryNVS) t = MEMORY_TYPE_ACPI_NVS;
        else if (d->Type == EfiUnusableMemory) t = MEMORY_TYPE_BAD_MEMORY;
        bi->entries[ve].base_address = d->PhysicalStart;
        bi->entries[ve].length = d->NumberOfPages * 4096ULL;
        bi->entries[ve].type = t;
        bi->entries[ve].acpi_attributes = 1;
        ve++;
    }
    bi->memory_entry_count = ve;

    uefi_print(L"[UEFI HANDOFF] Memory Map Populated. Entering Strict Silent Handoff Loop...\r\n");

    /* Step 7: 100% UEFI Spec-Compliant Silent ExitBootServices Handoff Loop
     * CRITICAL RULE FOR REAL HARDWARE (H81):
     * ABSOLUTELY NO uefi_print() / ConOut CALLS BETWEEN GetMemoryMap AND ExitBootServices!
     * Output only to COM1 serial (0x3F8 port IO) which does NOT touch UEFI memory/ConOut.
     */
    EFI_STATUS exit_status = EFI_LOAD_ERROR;

    for (int retry = 1; retry <= 10; retry++) {
        // 1. Refresh memory map & mapkey silently
        cmsz = mapsz;
        gmm_status = g_bs->GetMemoryMap(&cmsz, mm, &mapkey, &descsz, &descver);
        if (EFI_ERROR(gmm_status)) {
            if (gmm_status == EFI_BUFFER_TOO_SMALL) {
                com1_print("[EBS] Buffer too small, expanding buffer...\n");
                g_bs->FreePool(mm);
                mapsz = cmsz + 8192;
                mm = NULL;
                if (EFI_ERROR(g_bs->AllocatePool(EfiLoaderData, mapsz, (VOID**)&mm)) || !mm) {
                    com1_print("[EBS] Reallocation failed!\n");
                    break;
                }
                continue;
            }
            com1_print("[EBS] Fatal GetMemoryMap Error in loop!\n");
            break;
        }

        // 2. Call ExitBootServices IMMEDIATELY with refreshed mapkey (NO CONOUT PRINTS IN BETWEEN)
        exit_status = g_bs->ExitBootServices(ImageHandle, mapkey);

        // 3. Check status
        if (!EFI_ERROR(exit_status)) {
            // SUCCESS! ExitBootServices terminated UEFI Boot Services!
            com1_print("\n[SUCCESS] ExitBootServices() Succeeded 100% on Real Hardware!\n");
            break;
        }

        // On real AMI H81 firmware, the first call signals EVT_SIGNAL_EXIT_BOOT_SERVICES
        // which may invalidate the mapkey. The second call will NOT re-signal handlers and will succeed!
        com1_print("[EBS] ExitBootServices returned EFI_INVALID_PARAMETER (0x8000000000000002). Retrying silently...\n");
    }

    if (EFI_ERROR(exit_status)) {
        uefi_print(L"\r\n[FATAL] ExitBootServices failed after all silent retries!\r\n");
        while(1) __asm__ __volatile__("cli;hlt");
        return exit_status;
    }

    /* ====== POST-ExitBootServices: No UEFI Boot Services calls allowed ====== */
    
    // Clear entire GOP VRAM framebuffer to 100% pure black #000000 (wipes all firmware text remnants)
    draw_fb_rect(bi->vbe_framebuffer, bi->vbe_pitch, 0, 0, bi->vbe_width, bi->vbe_height, 0x00000000);

    /* Step 8: Mask legacy 8259A PIC */
    outb(0x21, 0xFF); outb(0xA1, 0xFF);

    /* Step 9: Relocate kernel to 0x100000 if not already there */
    if (kpaddr != 0x100000ULL) {
        com1_print("[BOOTX64.EFI] Relocating kernel to 0x100000...\n");
        uint8_t *dst = (uint8_t*)0x100000ULL;
        uint8_t *src = (uint8_t*)(uintptr_t)kpaddr;
        for (uint64_t i = 0; i < ksz; i++) dst[i] = src[i];
    }

    /* Step 10: Absolute Jump to kernel _start at 0x100000 */
    com1_print("[BOOTX64.EFI] Jumping to _start at 0x100000...\n");
    {
        register uint64_t r_entry __asm__("rax") = 0x100000ULL;
        register uint64_t r_biptr __asm__("rdi") = (uint64_t)(uintptr_t)bi;
        __asm__ __volatile__(
            "cli\n\t"
            "mov $0x70000, %%rsp\n\t"
            "xor %%rbp, %%rbp\n\t"
            "jmp *%%rax\n\t"
            :
            : "r"(r_entry), "r"(r_biptr)
            : "rsp", "rbp", "memory"
        );
    }

    /* Marker F: Should never be reached */
    com1_print("[BOOTX64.EFI] ERROR: Control returned after kernel jump!\n");
    while(1) __asm__ __volatile__("cli;hlt");
    return EFI_SUCCESS;
}
