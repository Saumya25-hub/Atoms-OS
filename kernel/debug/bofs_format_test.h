#ifndef BOFS_FORMAT_TEST_H
#define BOFS_FORMAT_TEST_H

#include "kernel/core/core_legacy/boot/include/boot_info.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void bofs_phase3_format_test_run(boot_info_t* boot_info);

#ifdef __cplusplus
}
#endif

#endif /* BOFS_FORMAT_TEST_H */
