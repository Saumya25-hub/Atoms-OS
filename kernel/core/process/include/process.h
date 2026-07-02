#ifndef PROCESS_H
#define PROCESS_H

#include "kernel/core/process/include/process_image.h"
#include "kernel/core/scheduler/include/task.h"

Task* process_spawn(ProcessImage* image, const char* name);

#endif
