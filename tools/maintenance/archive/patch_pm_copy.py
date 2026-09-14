
with open('boot/stage2.asm', 'r') as f:
    code = f.read()

# Replace the unreal mode setup and load_kernel_loop
# Find the start of the A20 setup we added earlier
start_idx = code.find('    ; Enable A20 Line')
end_idx = code.find('disk_read_success:')

if start_idx != -1 and end_idx != -1:
    old_section = code[start_idx:end_idx]
    
    new_section = '''    ; Enable A20 Line
    in al, 0x92
    or al, 2
    out 0x92, al

load_kernel:
    mov dword [kernel_dest_addr], KERNEL_EXEC
    mov cx, KERNEL_SECTORS
    mov ebx, KERNEL_LBA

load_kernel_loop:
    cmp cx, 0
    je disk_read_success

    mov dx, cx
    cmp dx, 64
    jbe .do_read
    mov dx, 64

.do_read:
    mov word [dap_kernel_sectors], dx
    mov word [dap_kernel_segment], (KERNEL_BUFFER >> 4)
    mov dword [dap_kernel_lba], ebx

    pusha
    mov ah, 0x42
    mov dl, [BOOT_ADDR + BOOT_SECTOR_SIZE - 4]
    mov si, dap_kernel
    int 0x13
    jc kernel_error
    popa

    ; Copy chunk to high memory by temporarily entering Protected Mode
    cli
    push ds
    push es
    
    lgdt [gdt_descriptor]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp $+2
    
    mov bp, DATA_SEG
    mov ds, bp
    mov es, bp
    
    mov esi, KERNEL_BUFFER
    mov edi, dword [kernel_dest_addr]
    movzx ecx, dx
    shl ecx, 7   ; dx * 128
    rep movsd
    
    movzx edx, dx
    shl edx, 9   ; dx * 512
    add dword [kernel_dest_addr], edx
    
    ; Switch back to Real Mode
    mov eax, cr0
    and al, 0xFE
    mov cr0, eax
    jmp $+2
    
    xor bp, bp
    mov ds, bp
    mov es, bp
    
    pop es
    pop ds
    sti

    sub cx, dx
    movzx edx, dx
    add ebx, edx
    
    jmp load_kernel_loop

'''
    code = code.replace(old_section, new_section)
    
    with open('boot/stage2.asm', 'w') as f:
        f.write(code)
    print('Patched successfully!')
else:
    print('Could not find section.')

