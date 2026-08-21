#ifndef KERNEL_STACK_H
#define KERNEL_STACK_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define STACK_CANARY_BOTTOM_MAGIC 0x5AFE57AC5AFE57ACULL
#define STACK_CANARY_TOP_MAGIC    0x5AFE57AC5AFE57ADULL

struct Task;

void *kernel_stack_alloc(size_t size);
void kernel_stack_free(void *stack, size_t size);
bool kernel_stack_validate(const struct Task *task);

#endif // KERNEL_STACK_H
