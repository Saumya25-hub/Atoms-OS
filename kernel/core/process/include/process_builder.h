#ifndef PROCESS_BUILDER_H
#define PROCESS_BUILDER_H

#include "kernel/core/process/include/process_image.h"
#include <stdbool.h>

// User stack resides at 0x00007FFFFFFFE000 (Top of the stack)
#define USER_STACK_TOP   0x00007FFFFFFFE000ULL
#define USER_STACK_PAGES 4

bool process_build_user_stack(ProcessImage* image, void* pml4);

#endif
