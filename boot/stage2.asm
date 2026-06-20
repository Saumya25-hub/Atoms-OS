; ==============================================================================
; SignaturesOS - Stage 2 Bootloader
; ==============================================================================
[ORG 0x8000]
[BITS 16]

stage2_start:
    ; 1. Print "Stage2 OK"
    mov si, stage2_msg
print_loop:
    lodsb
    or al, al
    jz enable_a20
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x0A
    int 0x10
    jmp print_loop

enable_a20:
    cli

    ; 2. Enable A20 Line (Fast A20 Method)
    in al, 0x92
    or al, 2
    out 0x92, al

    ; 3. Load the Global Descriptor Table (GDT)
    lgdt [gdt_descriptor]

    ; 4. Set the Protection Enable (PE) bit in CR0
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    ; 5. Far Jump into 32-bit Protected Mode
    jmp CODE_SEG:protected_mode_start

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
; 32-Bit Protected Mode & Phase 4A (Paging)
; ==============================================================================
[BITS 32]
protected_mode_start:
    ; 6. Reload segment registers
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 7. Initialize 32-bit stack
    mov ebp, 0x90000
    mov esp, ebp

    ; 8. Print "Protected Mode OK" to VGA buffer at 0xB8000
    mov ebx, pm_message
    mov edx, 0xB8000        ; VGA Video Memory Address (Row 0)

print_pm_loop:
    mov al, [ebx]
    cmp al, 0
    je setup_paging         

    mov ah, 0x0E            ; Yellow on Black
    mov [edx], ax

    add ebx, 1
    add edx, 2
    jmp print_pm_loop

setup_paging:
    ; ==========================================================================
    ; Phase 4A: Page Tables Initialization (Long Mode Prep)
    ; ==========================================================================
    
    ; 1. Zero out the 16KB space for Page Tables (0x10000 to 0x13FFF)
    mov edi, 0x10000        
    mov cr3, edi            
    xor eax, eax            
    mov ecx, 4096           
    rep stosd               

    ; 2. Link Page Map Level 4 (PML4) to Page Directory Pointer Table (PDPT)
    mov edi, 0x10000
    mov dword [edi], 0x11003

    ; 3. Link PDPT to Page Directory (PD)
    mov edi, 0x11000
    mov dword [edi], 0x12003

    ; 4. Link PD to Page Table (PT)
    mov edi, 0x12000
    mov dword [edi], 0x13003

    ; 5. Identity Map the first 2MB of memory in the PT
    mov edi, 0x13000        
    mov ebx, 0x00000003     
    mov ecx, 512            

build_pt_loop:
    mov dword [edi], ebx    
    add ebx, 0x1000         
    add edi, 8              
    loop build_pt_loop

    ; 6. Enable PAE (Physical Address Extension) in CR4
    mov eax, cr4
    or eax, 1 << 5          
    mov cr4, eax

    ; 7. Load CR3 with PML4 Base Address
    mov eax, 0x10000
    mov cr3, eax

    ; 8. Print "Paging OK" to VGA memory
    ; Row 1 starts at 0xB8000 + 160 = 0xB80A0
    mov ebx, paging_message
    mov edx, 0xB80A0

print_paging_loop:
    mov al, [ebx]
    cmp al, 0
    je setup_long_mode

    mov ah, 0x0B            ; Light Cyan text on Black
    mov [edx], ax

    add ebx, 1
    add edx, 2
    jmp print_paging_loop

setup_long_mode:
    ; ==========================================================================
    ; Phase 4B: Transition to 64-bit Long Mode
    ; ==========================================================================
    
    ; 1. Enable Long Mode in the EFER MSR
    mov ecx, 0xC0000080     ; MSR number for EFER (Extended Feature Enable Register)
    rdmsr                   ; Read MSR into EAX/EDX
    or eax, 1 << 8          ; Set LME (Long Mode Enable) bit (Bit 8)
    wrmsr                   ; Write EAX/EDX back to MSR

    ; 2. Enable Paging (Set PG bit in CR0)
    ; Since LME and PAE are set, this instantly puts the CPU into Compatibility Mode.
    mov eax, cr0
    or eax, 1 << 31         ; Set PG (Paging) bit (Bit 31)
    mov cr0, eax

    ; 3. Load the 64-bit Global Descriptor Table
    lgdt [gdt64_descriptor]

    ; 4. Far Jump to 64-bit Long Mode Code Segment
    ; This loads CS with the 64-bit selector and enters pure Long Mode
    jmp CODE64_SEG:long_mode_start

; ==============================================================================
; 64-Bit Global Descriptor Table (GDT)
; ==============================================================================
align 8
gdt64_start:
    dq 0x0000000000000000   ; Null Descriptor
gdt64_code:
    dq 0x0020980000000000   ; 64-bit Code Segment
gdt64_data:
    dq 0x0000920000000000   ; 64-bit Data Segment
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
    ; 5. Reload segment registers with 64-bit Data Segment
    ; Note: In 64-bit mode, DS/ES/SS are largely ignored, but it's safe to load them.
    mov ax, DATA64_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; 6. Print "Long Mode OK" to VGA memory
    ; Row 2 starts at 0xB8000 + 320 = 0xB8140
    mov rbx, lm_message     ; Use 64-bit registers
    mov rdx, 0xB8140

print_lm_loop:
    mov al, [rbx]
    cmp al, 0
    je halt_lm

    mov ah, 0x0A            ; Light Green on Black
    mov [rdx], ax

    add rbx, 1
    add rdx, 2
    jmp print_lm_loop

halt_lm:
    ; Phase 4B Complete - The CPU is now in 64-bit Long Mode!
    cli
    hlt
    jmp halt_lm

; Strings
stage2_msg db "Stage2 OK", 13, 10, 0
pm_message db "Protected Mode OK", 0
paging_message db "Paging OK", 0
lm_message db "Long Mode OK", 0

    ; Pad Stage 2 to match exactly 4 sectors (2048 bytes)
    times (4 * 512) - ($ - $$) db 0
