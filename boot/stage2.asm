; ==============================================================================
; SignaturesOS - Stage 2 Bootloader
; ==============================================================================
%include "boot/memory.inc"
%include "boot/boot.inc"

[ORG STAGE2_ADDR]
[BITS 16]

stage2_start:
    ; 1. Print "Stage2 OK"
    mov si, stage2_msg
print_loop:
    lodsb
    or al, al
    jz load_kernel
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x0A
    int 0x10
    jmp print_loop

load_kernel:
    ; Load Kernel via Extended LBA (AH=42h)
    mov ah, 0x42
    mov dl, [BOOT_ADDR + BOOT_SECTOR_SIZE - 4]        
    mov si, dap_kernel
    int 0x13
    jc kernel_error

    ; Print "Disk Read OK"
    mov si, disk_ok_msg
print_disk_ok:
    lodsb
    or al, al
    jz enable_a20
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x0A
    int 0x10
    jmp print_disk_ok

kernel_error:
    mov si, kernel_err_msg
print_kerr:
    lodsb
    or al, al
    jz halt_err
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x0C
    int 0x10
    jmp print_kerr
halt_err:
    cli
    hlt
    jmp halt_err

enable_a20:
    cli

    ; Detect Physical Memory (E820)
    mov di, BOOT_INFO_ADDR + 8  ; First entry at BOOT_INFO_ADDR + 8
    xor ebx, ebx
    xor bp, bp                  ; Entry count
.e820_loop:
    mov eax, 0xE820
    mov ecx, 24
    mov edx, 0x534D4150         ; 'SMAP'
    mov dword [di + 20], 1      ; Default ACPI valid bit
    int 0x15
    jc .e820_done
    cmp eax, 0x534D4150
    jne .e820_done
    jcxz .e820_skip
    add di, 24
    inc bp
.e820_skip:
    test ebx, ebx
    jz .e820_done
    jmp .e820_loop
.e820_done:
    ; Store entry count (32-bit) at BOOT_INFO_ADDR
    mov dword [BOOT_INFO_ADDR], ebp
    mov dword [BOOT_INFO_ADDR + 4], 0 ; Padding

    ; 3. Enable A20 Line
    in al, 0x92
    or al, 2
    out 0x92, al

    ; 4. Load GDT & Enter Protected Mode
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    jmp CODE_SEG:protected_mode_start

; Disk Address Packet (DAP) for Kernel Read
align 4
dap_kernel:
    db 0x10             
    db 0                
    dw KERNEL_SECTORS               
    dw 0x0000           
    dw (KERNEL_BUFFER >> 4)           
    dq KERNEL_LBA                

; ==============================================================================
; 32-Bit Global Descriptor Table (GDT)
; ==============================================================================
align 8                     
gdt_start:
    dq 0x0                  

gdt_code:                   
    dw 0xFFFF               
    dw 0x0                  
    db 0x0                  
    db 10011010b            
    db 11001111b            
    db 0x0                  

gdt_data:                   
    dw 0xFFFF               
    dw 0x0                  
    db 0x0                  
    db 10010010b            
    db 11001111b            
    db 0x0                  
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1  
    dd gdt_start                

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

; ==============================================================================
; 32-Bit Protected Mode
; ==============================================================================
[BITS 32]
protected_mode_start:
    ; Reload segment registers
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Initialize 32-bit stack
    mov ebp, STACK_TOP
    mov esp, ebp

    ; Copy Kernel from KERNEL_BUFFER to KERNEL_EXEC
    mov esi, KERNEL_BUFFER        
    mov edi, KERNEL_EXEC       
    mov ecx, (KERNEL_SECTORS * BOOT_SECTOR_SIZE) / 4           
    rep movsd               

    ; Print "Protected Mode OK"
    mov ebx, pm_message
    mov edx, VGA_MEMORY

print_pm_loop:
    mov al, [ebx]
    cmp al, 0
    je setup_paging

    mov ah, 0x0E
    mov [edx], ax

    add ebx, 1
    add edx, 2
    jmp print_pm_loop

setup_paging:
    ; Zero out Page Tables
    mov edi, PAGE_TABLE_BASE        
    xor eax, eax            
    mov ecx, 4096           
    rep stosd               

    ; Link Tables
    mov edi, PAGE_TABLE_BASE
    mov dword [edi], PAGE_TABLE_BASE + 0x1003

    mov edi, PAGE_TABLE_BASE + 0x1000
    mov dword [edi], PAGE_TABLE_BASE + 0x2003

    mov edi, PAGE_TABLE_BASE + 0x2000
    mov dword [edi], PAGE_TABLE_BASE + 0x3003

    ; Identity Map the first 2MB
    mov edi, PAGE_TABLE_BASE + 0x3000        
    mov ebx, 0x00000003     
    mov ecx, 512            

build_pt_loop:
    mov dword [edi], ebx    
    add ebx, 0x1000         
    add edi, 8              
    loop build_pt_loop

    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5          
    mov cr4, eax

    ; Load CR3
    mov eax, PAGE_TABLE_BASE
    mov cr3, eax

    ; Print "Paging OK"
    mov ebx, paging_message
    mov edx, VGA_MEMORY + 160

print_paging_loop:
    mov al, [ebx]
    cmp al, 0
    je setup_long_mode

    mov ah, 0x0B
    mov [edx], ax

    add ebx, 1
    add edx, 2
    jmp print_paging_loop

setup_long_mode:
    ; Enable Long Mode (LME)
    mov ecx, 0xC0000080     
    rdmsr                   
    or eax, 1 << 8          
    wrmsr                   

    ; Enable Paging (PG)
    mov eax, cr0
    or eax, 1 << 31         
    mov cr0, eax

    ; Load 64-bit GDT
    lgdt [gdt64_descriptor]

    ; Far Jump to 64-bit Long Mode
    jmp CODE64_SEG:long_mode_start

; ==============================================================================
; 64-Bit Global Descriptor Table (GDT)
; ==============================================================================
align 8
gdt64_start:
    dq 0x0000000000000000   
gdt64_code:
    dq 0x0020980000000000   
gdt64_data:
    dq 0x0000920000000000   
gdt64_end:

gdt64_descriptor:
    dw gdt64_end - gdt64_start - 1
    dd gdt64_start

CODE64_SEG equ gdt64_code - gdt64_start
DATA64_SEG equ gdt64_data - gdt64_start

; ==============================================================================
; 64-Bit Long Mode
; ==============================================================================
[BITS 64]
long_mode_start:
    ; Reload segment registers
    mov ax, DATA64_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Print "Long Mode OK"
    mov rbx, lm_message
    mov rdx, VGA_MEMORY + 320

print_lm_loop:
    mov al, [rbx]
    cmp al, 0
    je jump_kernel

    mov ah, 0x0A
    mov [rdx], ax

    add rbx, 1
    add rdx, 2
    jmp print_lm_loop

jump_kernel:
    mov rdi, BOOT_INFO_ADDR     ; Pass boot_info_t pointer to kernel via RDI
    mov rax, KERNEL_EXEC
    jmp rax

; Strings
stage2_msg db "Stage2 OK", 13, 10, 0
disk_ok_msg db "Disk Read OK", 13, 10, 0
kernel_err_msg db "Error: Kernel Read FAILED! Halting.", 0
pm_message db "Protected Mode OK", 0
paging_message db "Paging OK", 0
lm_message db "Long Mode OK", 0

    times (STAGE2_SECTORS * BOOT_SECTOR_SIZE) - ($ - $$) db 0
