[BITS 64]
[GLOBAL _start]
[EXTERN kernel_main]

_start:
    ; Align the stack to 16 bytes per System V ABI requirements
    and rsp, 0xFFFFFFFFFFFFFFF0
    
    ; Execute the C Kernel
    call kernel_main
    
    ; The PM explicitly ordered: After kernel_main returns, THEN execute cli/hlt.
.end:
    cli
    hlt
    jmp .end
