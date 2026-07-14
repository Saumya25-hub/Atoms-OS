; ==============================================================================
; SignaturesOS - Boot Configuration Constants
; ==============================================================================

; Disk Layout Constants
BOOT_SECTOR_SIZE    equ 512
BOOT_SIGNATURE      equ 0xAA55

; Stage 2 Bootloader parameters
STAGE2_LBA          equ 1           ; Stage 2 starts immediately after Boot Sector (LBA 1)
STAGE2_SECTORS      equ 4           ; Number of sectors Stage 2 occupies

; Kernel parameters
KERNEL_LBA          equ 5           ; Kernel starts immediately after Stage 2 (LBA 1 + 4 = 5)
KERNEL_SECTORS      equ 950         ; Number of sectors allocated for Kernel disk read
