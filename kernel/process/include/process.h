#ifndef PROCESS_H
#define PROCESS_H

#include "kernel/process/include/process_image.h"
#include "kernel/scheduler/include/task.h"

Task* process_spawn(ProcessImage* image, const char* name);

#endif
