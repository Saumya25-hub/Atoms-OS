#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#define KERNEL_TASK_STACK_SIZE (4 * 1024)

typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_SLEEPING,
    TASK_TERMINATED
} TaskState;

typedef struct Task {
    uint64_t id;
    const char* name;
    TaskState state;
    void* stack;
    uint64_t rsp;
    uint64_t rip;
    struct Task* next;
} Task;

typedef struct Context {
    // General purpose registers
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rdi, rsi, rbp, rbx, rdx, rcx, rax;
    
    // Interrupt frame (iretq expectations)
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} Context;

int main() {
    printf("sizeof(Task) = %zu\n", sizeof(Task));
    printf("offsetof(Task, id) = %zu\n", offsetof(Task, id));
    printf("offsetof(Task, name) = %zu\n", offsetof(Task, name));
    printf("offsetof(Task, state) = %zu\n", offsetof(Task, state));
    printf("offsetof(Task, stack) = %zu\n", offsetof(Task, stack));
    printf("offsetof(Task, rsp) = %zu\n", offsetof(Task, rsp));
    printf("offsetof(Task, rip) = %zu\n", offsetof(Task, rip));
    printf("offsetof(Task, next) = %zu\n", offsetof(Task, next));
    
    printf("\nsizeof(Context) = %zu\n", sizeof(Context));
    printf("offsetof(Context, r15) = %zu\n", offsetof(Context, r15));
    printf("offsetof(Context, rax) = %zu\n", offsetof(Context, rax));
    printf("offsetof(Context, rip) = %zu\n", offsetof(Context, rip));
    printf("offsetof(Context, cs) = %zu\n", offsetof(Context, cs));
    printf("offsetof(Context, rflags) = %zu\n", offsetof(Context, rflags));
    printf("offsetof(Context, rsp) = %zu\n", offsetof(Context, rsp));
    printf("offsetof(Context, ss) = %zu\n", offsetof(Context, ss));
    return 0;
}
