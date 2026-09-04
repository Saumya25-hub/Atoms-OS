#ifndef BOSX_PHASE10_TEST_H
#define BOSX_PHASE10_TEST_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/core/core_legacy/boot/include/boot_info.h"

#ifdef __cplusplus
extern "C" {
#endif

void bosx_phase10_test_run(boot_info_t* boot_info);

#ifdef __cplusplus
}
#endif

#endif // BOSX_PHASE10_TEST_H
