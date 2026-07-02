[BITS 64]

global sys_yield
global sys_sleep
global sys_uptime
global sys_getpid

; -----------------------------------
; void sys_yield(void)
; -----------------------------------
sys_yield:
    mov rax, 0      ; SYS_YIELD
    int 0x80
    ret

; -----------------------------------
; void sys_sleep(uint64_t ticks)
; -----------------------------------
; Arg 1 (ticks) is passed in RDI (System V ABI)
sys_sleep:
    mov rax, 1      ; SYS_SLEEP
    ; RDI already contains the ticks argument, no need to move it
    int 0x80
    ret

; -----------------------------------
; uint64_t sys_uptime(void)
; -----------------------------------
sys_uptime:
    mov rax, 2      ; SYS_UPTIME
    int 0x80
    ; RAX contains the return value from the syscall
    ret

; -----------------------------------
; uint64_t sys_getpid(void)
; -----------------------------------
sys_getpid:
    mov rax, 3      ; SYS_GETPID
    int 0x80
    ; RAX contains the return value
    ret
