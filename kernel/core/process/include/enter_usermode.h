#ifndef ENTER_USERMODE_H
#define ENTER_USERMODE_H

#include <stdint.h>

// Launches Ring 3 userspace execution
void enter_usermode(uint64_t rip, uint64_t rsp);

#endif
