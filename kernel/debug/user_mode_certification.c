#include "user_mode_certification.h"
#include "../../arch/x86_64/gdt/gdt.h"
#include "../core/memory/vmm/include/vmm.h"
#include "../core/process/process_manager.h"
#include "../core/scheduler/include/context.h"
#include "../core/scheduler/include/scheduler.h"
#include "../core/scheduler/include/task.h"
#include "../core/usermode/user_mode.h"
#include <stddef.h>

static void count(ATOMS_UserModeCertificationReport *report, bool passed) {
  if (passed)
    ++report->passed;
  else
    ++report->failed;
}

bool ATOMS_UserMode_RunCertification(
    ATOMS_UserModeCertificationReport *report) {
  if (!report)
    return false;
  for (uint32_t i = 0; i < sizeof(*report); ++i)
    ((uint8_t *)report)[i] = 0;

  report->canonical_validation =
      ATOMS_UserMode_IsCanonical(0x1000000) &&
      ATOMS_UserMode_IsCanonical(0xFFFF800000000000ULL) &&
      !ATOMS_UserMode_IsCanonical(0x0000800000000000ULL);
  count(report, report->canonical_validation);

  report->null_page_protection = !ATOMS_UserMode_IsUserRange(0, 1) &&
                                 !ATOMS_UserMode_IsUserRange(0xFFF, 0x2000);
  count(report, report->null_page_protection);

  report->kernel_range_rejection =
      !ATOMS_UserMode_IsUserRange(0x80000000ULL, 0x1000);
  count(report, report->kernel_range_rejection);

  report->overflow_rejection = !ATOMS_UserMode_IsUserRange(UINT64_MAX - 7, 16);
  count(report, report->overflow_rejection);

  void *first = vmm_create_address_space();
  void *second = vmm_create_address_space();
  report->address_space_creation = first != NULL && second != NULL;
  count(report, report->address_space_creation);

  if (first && second) {
    bool first_map =
        vmm_map_user_page(first, 0x2000000, VMM_ACCESS_READ | VMM_ACCESS_WRITE);
    VMMPageInfo first_info;
    VMMPageInfo second_info;
    bool first_visible = vmm_query_page(first, 0x2000000, &first_info);
    bool second_hidden = !vmm_query_page(second, 0x2000000, &second_info);
    report->address_space_isolation =
        first_map && first_visible && second_hidden;
    count(report, report->address_space_isolation);

    report->user_page_permissions =
        first_info.user && first_info.writable && !first_info.executable;
    count(report, report->user_page_permissions);

    report->readonly_enforcement =
        vmm_map_user_page(first, 0x2001000, VMM_ACCESS_READ) &&
        !vmm_validate_user_range(first, 0x2001000, 1, VMM_ACCESS_WRITE);
    count(report, report->readonly_enforcement);

    report->execute_enforcement =
        vmm_map_user_page(first, 0x2002000,
                          VMM_ACCESS_READ | VMM_ACCESS_EXECUTE) &&
        vmm_validate_user_range(first, 0x2002000, 1, VMM_ACCESS_EXECUTE);
    count(report, report->execute_enforcement);

    report->guard_page_protection =
        vmm_map_guard_page(first, 0x1FFFF000) &&
        !vmm_query_page(first, 0x1FFFF000, &first_info);
    count(report, report->guard_page_protection);
  } else {
    report->address_space_isolation = false;
    report->user_page_permissions = false;
    report->readonly_enforcement = false;
    report->execute_enforcement = false;
    report->guard_page_protection = false;
    report->failed += 5;
  }

  report->ring_selectors =
      USER_CODE_SEGMENT == 0x23 && USER_DATA_SEGMENT == 0x1B;
  count(report, report->ring_selectors);
  report->idle_kernel_privilege =
      ATOMS_UserMode_CurrentPrivilege() == ATOMS_PRIVILEGE_RING0;
  count(report, report->idle_kernel_privilege);

  ATOMS_UserModeDiagnostics diagnostics;
  ATOMS_UserMode_GetDiagnostics(&diagnostics);
  report->diagnostics_coherence =
      diagnostics.current_ring <= 3 &&
      diagnostics.current_address_space ==
          (scheduler_current_task() ? (uint64_t)scheduler_current_task()->pml4
                                    : 0);
  count(report, report->diagnostics_coherence);

  if (first && first != vmm_get_active_pml4())
    (void)vmm_destroy_address_space(first);
  if (second && second != vmm_get_active_pml4())
    (void)vmm_destroy_address_space(second);
  return report->failed == 0;
}
