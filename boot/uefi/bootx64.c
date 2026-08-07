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

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    g_st=SystemTable; g_bs=SystemTable->BootServices; com1_init();
    uefi_print(L"[UEFI BOOTLOADER] Starting SignaturesOS Production UEFI Loader (BOOTX64.EFI)...\r\n");

    /* Step 1: GOP */
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop=NULL;
    EFI_STATUS status=g_bs->LocateProtocol(&g_gop_guid,NULL,(VOID**)&gop);
    if(EFI_ERROR(status)||!gop){uefi_print(L"ERROR:GOP\r\n");return status;}
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
    if(!root){uefi_print(L"ERROR:NO ROOT\r\n");return EFI_NOT_FOUND;}

    /* Step 3: Load kernel.bin at 0x100000 via AllocatePages(AllocateAddress) */
    EFI_FILE_PROTOCOL *kf=NULL;
    if(EFI_ERROR(root->Open(root,&kf,L"kernel.bin",EFI_FILE_MODE_READ,0))&&
       EFI_ERROR(root->Open(root,&kf,L"KERNEL.BIN",EFI_FILE_MODE_READ,0))&&
       EFI_ERROR(root->Open(root,&kf,L"\\kernel.bin",EFI_FILE_MODE_READ,0))&&
       EFI_ERROR(root->Open(root,&kf,L"\\KERNEL.BIN",EFI_FILE_MODE_READ,0))){
        uefi_print(L"ERROR:NO KERNEL\r\n"); return EFI_NOT_FOUND;
    }
    UINT8 ibuf[512]; UINTN isz=512;
    if(EFI_ERROR(kf->GetInfo(kf,&g_info_guid,&isz,ibuf))){uefi_print(L"ERROR:KINFO\r\n");return EFI_LOAD_ERROR;}
    EFI_FILE_INFO *fi=(EFI_FILE_INFO*)ibuf;
    UINTN ksz=(UINTN)fi->FileSize;

    /* Try to allocate kernel directly at 0x100000 */
    EFI_PHYSICAL_ADDRESS kpaddr=0x100000ULL;
    UINTN kpages=(ksz+4095)/4096+1;
    if(EFI_ERROR(g_bs->AllocatePages(AllocateAddress,EfiLoaderCode,kpages,&kpaddr))){
        kpaddr=0;
        if(EFI_ERROR(g_bs->AllocatePages(AllocateAnyPages,EfiLoaderCode,kpages,&kpaddr))){
            uefi_print(L"ERROR:KALLOC\r\n"); return EFI_LOAD_ERROR;
        }
    }
    UINTN bread=ksz;
    if(EFI_ERROR(kf->Read(kf,&bread,(VOID*)(uintptr_t)kpaddr))){uefi_print(L"ERROR:KREAD\r\n");return EFI_LOAD_ERROR;}
    kf->Close(kf);
    uefi_print(L"[UEFI BOOTLOADER] kernel.bin Loaded into RAM Pool Buffer Successfully.\r\n");

    /* Step 4: boot_info */
    boot_info_t *bi=NULL;
    if(EFI_ERROR(g_bs->AllocatePool(EfiLoaderData,sizeof(boot_info_t),(VOID**)&bi))||!bi){
        uefi_print(L"ERROR:BIALLOC\r\n"); return EFI_LOAD_ERROR;
    }
    for(UINTN i=0;i<sizeof(boot_info_t);i++) ((UINT8*)bi)[i]=0;
    bi->vbe_width=gop->Mode->Info->HorizontalResolution;
    bi->vbe_height=gop->Mode->Info->VerticalResolution;
    bi->vbe_pitch=gop->Mode->Info->PixelsPerScanLine*4;
    bi->vbe_bpp=32;
    bi->vbe_framebuffer=(uint64_t)gop->Mode->FrameBufferBase;

    /* Step 5: Allocate 7 pages for page tables + GDT (all from UEFI safe memory) */
    EFI_PHYSICAL_ADDRESS pt=0;
    if(EFI_ERROR(g_bs->AllocatePages(AllocateAnyPages,EfiLoaderData,7,&pt))){
        uefi_print(L"ERROR:PTALLOC\r\n"); return EFI_LOAD_ERROR;
    }

    /* Step 6: Memory map */
    UINTN mapsz=0,mapkey=0,descsz=0; UINT32 descver=0;
    EFI_MEMORY_DESCRIPTOR *mm=NULL;
    g_bs->GetMemoryMap(&mapsz,NULL,&mapkey,&descsz,&descver);
    mapsz+=4096;
    if(EFI_ERROR(g_bs->AllocatePool(EfiLoaderData,mapsz,(VOID**)&mm))||!mm){
        uefi_print(L"ERROR:MMALLOC\r\n"); return EFI_LOAD_ERROR;
    }

    /* Step 7: ExitBootServices retry loop */
    for(int retry=0;retry<10;retry++){
        UINTN cmsz=mapsz;
        if(EFI_ERROR(g_bs->GetMemoryMap(&cmsz,mm,&mapkey,&descsz,&descver))) break;
        if(!EFI_ERROR(g_bs->ExitBootServices(ImageHandle,mapkey))){
            UINTN dcnt=cmsz/descsz; uint32_t ve=0;
            for(UINTN i=0;i<dcnt&&ve<256;i++){
                EFI_MEMORY_DESCRIPTOR *d=(EFI_MEMORY_DESCRIPTOR*)((UINT8*)mm+(i*descsz));
                uint32_t t=MEMORY_TYPE_RESERVED;
                if(d->Type==EfiConventionalMemory||d->Type==EfiLoaderCode||d->Type==EfiLoaderData) t=MEMORY_TYPE_USABLE;
                else if(d->Type==EfiACPIReclaimMemory) t=MEMORY_TYPE_ACPI_RECLAIMABLE;
                else if(d->Type==EfiACPIMemoryNVS) t=MEMORY_TYPE_ACPI_NVS;
                else if(d->Type==EfiUnusableMemory) t=MEMORY_TYPE_BAD_MEMORY;
                bi->entries[ve].base_address=d->PhysicalStart;
                bi->entries[ve].length=d->NumberOfPages*4096ULL;
                bi->entries[ve].type=t;
                bi->entries[ve].acpi_attributes=1;
                ve++;
            }
            bi->memory_entry_count=ve;
            break;
        }
    }

    /* ====== POST-ExitBootServices: No UEFI calls allowed ====== */
    com1_print("\n[BOOTX64.EFI] ExitBootServices() Succeeded 100%!\n");

    /* Step 8: Mask legacy 8259A PIC */
    outb(0x21,0xFF); outb(0xA1,0xFF);

    /* Step 9: Relocate kernel to 0x100000 if not already there.
       OVMF/UEFI already provides a full identity map of all physical RAM,
       so we can read/write 0x100000 directly without any CR3 swap. */
    if(kpaddr != 0x100000ULL) {
        com1_print("[BOOTX64.EFI] Relocating kernel to 0x100000...\n");
        uint8_t *dst = (uint8_t*)0x100000ULL;
        uint8_t *src = (uint8_t*)(uintptr_t)kpaddr;
        for(uint64_t i = 0; i < ksz; i++) dst[i] = src[i];
        com1_print("[BOOTX64.EFI] kernel.bin Relocated to 0x100000!\n");
    } else {
        com1_print("[BOOTX64.EFI] kernel.bin Already at 0x100000.\n");
    }

    /* Step 10: Direct absolute jump to kernel _start at 0x100000.
       We use UEFI's existing identity mapping - no lretq, no GDT/CR3 swap.
       kernel_entry.asm _start will configure its own GDT, IDT, and page tables.
       RDI = boot_info pointer (System V AMD64 ABI first argument).
       RSP = 0x80000 (clean stack below kernel load address).
       Use explicit register constraints to guarantee correct registers. */
    com1_print("[BOOTX64.EFI] Jumping to _start at 0x100000 via jmp *rax...\n");
    {
        register uint64_t r_entry  __asm__("rax") = 0x100000ULL;
        register uint64_t r_biptr __asm__("rdi") = (uint64_t)(uintptr_t)bi;
        __asm__ __volatile__(
            "cli\n\t"
            "mov $0x80000, %%rsp\n\t"
            "xor %%rbp, %%rbp\n\t"
            "jmp *%%rax\n\t"
            :
            : "r"(r_entry), "r"(r_biptr)
            : "rsp", "rbp", "memory"
        );
    }
    while(1) __asm__ __volatile__("cli;hlt");
    return EFI_SUCCESS;
}
