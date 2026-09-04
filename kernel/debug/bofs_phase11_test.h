#ifndef BOFS_PHASE11_TEST_H
#define BOFS_PHASE11_TEST_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/core/core_legacy/boot/include/boot_info.h"

#ifdef __cplusplus
extern "C" {
#endif

void bofs_phase11_test_run(boot_info_t* boot_info);

#ifdef __cplusplus
}
#endif

#endif // BOFS_PHASE11_TEST_H
