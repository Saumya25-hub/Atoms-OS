#ifndef CURSOR_CERTIFICATION_H
#define CURSOR_CERTIFICATION_H

#include "kernel/core/core_legacy/boot/include/boot_info.h"

void atoms_cursor_certification_init(boot_info_t *boot_info);
void atoms_cursor_certification_task(void);

#endif /* CURSOR_CERTIFICATION_H */
