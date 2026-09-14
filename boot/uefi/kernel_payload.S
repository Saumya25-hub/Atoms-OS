.section .rdata, "dr"
.p2align 4
.global g_kernel_data
.global g_kernel_size_val

g_kernel_data:
    .incbin "build/kernel.bin"
g_kernel_data_end:

.p2align 4
g_kernel_size_val:
    .quad g_kernel_data_end - g_kernel_data
