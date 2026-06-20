; ==============================================================================
; SignaturesOS - Stage 1 Bootloader
; ==============================================================================
%include "boot/memory.inc"
%include "boot/boot.inc"

[ORG BOOT_ADDR]        
[BITS 16]           

start:
    ; 1. Normalize Segment Registers and Setup Stack
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, BOOT_ADDR

    ; Save the boot drive number provided by BIOS in DL
    mov [BOOT_ADDR + BOOT_SECTOR_SIZE - 4], dl

    ; 2. Print "Stage1 OK" message
    mov si, stage1_msg
print_s1_loop:
    lodsb
    or al, al
    jz disk_read_start
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x07
    int 0x10
    jmp print_s1_loop

disk_read_start:
    ; 3. Disk Read: Load Stage 2 via Extended LBA (AH=42h)
    mov ah, 0x42
    mov dl, [BOOT_ADDR + BOOT_SECTOR_SIZE - 4]
    mov si, dap_stage2
    int 0x13
    jc disk_error
    
    ; 4. Transfer Execution to Stage 2
    jmp 0x0000:STAGE2_ADDR

disk_error:
    mov si, disk_error_msg
error_print_loop:
    lodsb
    or al, al
    jz halt_loop
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x0C
    int 0x10
    jmp error_print_loop

halt_loop:
    cli
    hlt
    jmp halt_loop

; Variables & Data Structures
stage1_msg db "Stage1 OK", 13, 10, 0
disk_error_msg db "Error: Stage1 Disk Read FAILED! Halting.", 0

align 4
dap_stage2:
    db 0x10                 ; Size of DAP
    db 0                    ; Unused
    dw STAGE2_SECTORS       ; Number of sectors
    dw STAGE2_ADDR          ; Offset
    dw 0x0000               ; Segment
    dq STAGE2_LBA           ; LBA

    ; Padding and Magic Signature
    times BOOT_SECTOR_SIZE - 4 - ($ - $$) db 0
    BOOT_DRIVE_STORAGE db 0, 0
    dw BOOT_SIGNATURE
