[bits 64]
global g_embedded_chromium_elf
global g_embedded_chromium_elf_len
global get_embedded_chromium_elf_len

section .rodata
align 16
g_embedded_chromium_elf:
    incbin "build/chromium_browser.elf"
g_embedded_chromium_elf_end:

align 16
g_embedded_chromium_elf_len:
    dq g_embedded_chromium_elf_end - g_embedded_chromium_elf

section .text
align 16
get_embedded_chromium_elf_len:
    mov rax, [rel g_embedded_chromium_elf_len]
    test rax, rax
    jnz .done
    mov rax, g_embedded_chromium_elf_end - g_embedded_chromium_elf
.done:
    ret
