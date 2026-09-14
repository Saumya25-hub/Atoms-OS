.section .rodata, "a", @progbits
.p2align 4
.global g_embedded_media_player_elf
.global g_embedded_media_player_elf_len

g_embedded_media_player_elf:
    .incbin "build/media_player.elf"
g_embedded_media_player_elf_end:

.p2align 4
g_embedded_media_player_elf_len:
    .quad g_embedded_media_player_elf_end - g_embedded_media_player_elf

.section .text, "ax", @progbits
.p2align 4
.global get_embedded_media_player_elf_len
get_embedded_media_player_elf_len:
    movq g_embedded_media_player_elf_len(%rip), %rax
    testq %rax, %rax
    jnz .Ldone
    movq $g_embedded_media_player_elf_end - g_embedded_media_player_elf, %rax
.Ldone:
    ret
