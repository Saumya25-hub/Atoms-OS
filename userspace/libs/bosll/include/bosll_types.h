#ifndef BOS_BOSLL_TYPES_H
#define BOS_BOSLL_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef uint64_t BOS_HANDLE;
typedef uint64_t BOS_STATUS;

#define BOS_SUCCESS             0x00000000
#define BOS_STATUS_UNSUCCESSFUL 0xC0000001
#define BOS_STATUS_INVALID_HANDLE 0xC0000008
#define BOS_STATUS_NO_MEMORY    0xC0000017

typedef struct {
    uint64_t ProcessId;
    uint64_t ParentProcessId;
    uint64_t Flags;
} BOS_PROCESS_INFO;

typedef struct {
    uint64_t ThreadId;
    uint64_t ProcessId;
    uint64_t StackBase;
    uint64_t StackLimit;
} BOS_THREAD_INFO;

typedef struct {
    uint64_t Rip;
    uint64_t Rsp;
    uint64_t Rflags;
    uint64_t Rax;
    uint64_t Rbx;
    uint64_t Rcx;
    uint64_t Rdx;
    uint64_t Rsi;
    uint64_t Rdi;
    uint64_t Rbp;
    uint64_t R8;
    uint64_t R9;
    uint64_t R10;
    uint64_t R11;
    uint64_t R12;
    uint64_t R13;
    uint64_t R14;
    uint64_t R15;
} BOS_CPU_CONTEXT;

#endif // BOS_BOSLL_TYPES_H
