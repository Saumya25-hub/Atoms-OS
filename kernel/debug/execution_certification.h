#ifndef ATOMS_EXECUTION_CERTIFICATION_H
#define ATOMS_EXECUTION_CERTIFICATION_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint32_t passed;
  uint32_t failed;
  uint32_t processes_created;
  uint32_t threads_created;
  uint64_t leaked_resources;
  bool process_lifecycle_passed;
  bool thread_lifecycle_passed;
  bool transition_validation_passed;
  bool resource_cleanup_passed;
  bool scheduler_initialized_passed;
  bool scheduler_policy_passed;
  bool scheduler_queue_passed;
  bool scheduler_consistency_passed;
  bool idle_thread_passed;
  bool priority_aging_contract_passed;
  bool diagnostics_passed;
} ATOMS_ExecutionCertificationReport;

bool ATOMS_Execution_RunCertification(
    ATOMS_ExecutionCertificationReport *out_report,
    uint32_t process_stress_count, uint32_t thread_stress_count);

#endif
