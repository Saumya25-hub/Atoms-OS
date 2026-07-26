#ifndef ATOMS_USER_MODE_CERTIFICATION_H
#define ATOMS_USER_MODE_CERTIFICATION_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint32_t passed;
  uint32_t failed;
  bool canonical_validation;
  bool null_page_protection;
  bool kernel_range_rejection;
  bool overflow_rejection;
  bool address_space_creation;
  bool address_space_isolation;
  bool user_page_permissions;
  bool readonly_enforcement;
  bool execute_enforcement;
  bool guard_page_protection;
  bool ring_selectors;
  bool idle_kernel_privilege;
  bool diagnostics_coherence;
} ATOMS_UserModeCertificationReport;

bool ATOMS_UserMode_RunCertification(ATOMS_UserModeCertificationReport *report);

#endif
