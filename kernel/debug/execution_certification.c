#include "execution_certification.h"
#include "../core/execution/include/execution_contract.h"
#include "../core/process/process_manager.h"
#include "../core/scheduler/include/scheduler.h"
#include "../core/thread/thread_manager.h"

static void count_result(ATOMS_ExecutionCertificationReport *report,
                         bool passed) {
  if (passed)
    ++report->passed;
  else
    ++report->failed;
}

bool ATOMS_Execution_RunCertification(
    ATOMS_ExecutionCertificationReport *out_report,
    uint32_t process_stress_count, uint32_t thread_stress_count) {
  if (!out_report)
    return false;
  for (uint32_t i = 0; i < sizeof(*out_report); ++i)
    ((uint8_t *)out_report)[i] = 0;
  if (process_stress_count == 0)
    process_stress_count = 1;
  if (thread_stress_count == 0)
    thread_stress_count = 1;
  if (process_stress_count > ATOMS_MAX_PROCESSES)
    process_stress_count = ATOMS_MAX_PROCESSES;
  if (thread_stress_count > ATOMS_MAX_THREADS)
    thread_stress_count = ATOMS_MAX_THREADS;

  ATOMS_PCB *process =
      ATOMS_Process_Create("certification", "/system/certification", 0, 0);
  out_report->process_lifecycle_passed =
      process != 0 && process->state == ATOMS_PROC_STATE_READY;
  count_result(out_report, out_report->process_lifecycle_passed);

  if (process) {
    for (uint32_t i = 0; i < process_stress_count && i < 8U; ++i) {
      ATOMS_PCB *child = ATOMS_Process_Create(
          "cert-child", "/system/cert-child", process->pid, 0);
      if (child) {
        ++out_report->processes_created;
        ATOMS_Process_Terminate(child->pid, 0);
      }
    }
    ATOMS_TCB *thread = ATOMS_Thread_Create(process->pid, "cert-thread", 0, 16);
    out_report->thread_lifecycle_passed =
        thread != 0 && thread->state == ATOMS_THREAD_STATE_READY;
    count_result(out_report, out_report->thread_lifecycle_passed);
    if (thread) {
      ++out_report->threads_created;
      ATOMS_Thread_Terminate(thread->tid);
    }
    out_report->transition_validation_passed =
        ATOMS_ProcessStateTransitionValid(ATOMS_PROC_STATE_READY,
                                          ATOMS_PROC_STATE_RUNNING) &&
        !ATOMS_ProcessStateTransitionValid(ATOMS_PROC_STATE_CLOSED,
                                           ATOMS_PROC_STATE_RUNNING) &&
        ATOMS_ThreadStateTransitionValid(ATOMS_THREAD_STATE_READY,
                                         ATOMS_THREAD_STATE_RUNNING) &&
        !ATOMS_ThreadStateTransitionValid(ATOMS_THREAD_STATE_FREE,
                                          ATOMS_THREAD_STATE_RUNNING);
    count_result(out_report, out_report->transition_validation_passed);
    ATOMS_Process_Terminate(process->pid, 0);
  }

  out_report->resource_cleanup_passed =
      ATOMS_Process_AuditLeaks(&out_report->leaked_resources);
  count_result(out_report, out_report->resource_cleanup_passed);

  SchedulerDiagnostics scheduler_diagnostics;
  scheduler_get_diagnostics(&scheduler_diagnostics);
  out_report->scheduler_initialized_passed =
      scheduler_get_idle_task() != 0 &&
      scheduler_get_idle_task()->state != TASK_TERMINATED;
  count_result(out_report, out_report->scheduler_initialized_passed);

  SchedulerPolicy original_policy = scheduler_get_policy();
  out_report->scheduler_policy_passed =
      scheduler_set_policy(SCHEDULER_POLICY_ROUND_ROBIN) &&
      scheduler_get_policy() == SCHEDULER_POLICY_ROUND_ROBIN &&
      scheduler_set_policy(SCHEDULER_POLICY_PRIORITY_AGING) &&
      scheduler_get_policy() == SCHEDULER_POLICY_PRIORITY_AGING;
  count_result(out_report, out_report->scheduler_policy_passed);
  (void)scheduler_set_policy(original_policy);

  out_report->scheduler_queue_passed =
      scheduler_diagnostics.ready_count <= scheduler_get_task_count() &&
      scheduler_diagnostics.current_task_id ==
          (scheduler_current_task() ? scheduler_current_task()->id : 0);
  count_result(out_report, out_report->scheduler_queue_passed);

  out_report->scheduler_consistency_passed = scheduler_validate_consistency();
  count_result(out_report, out_report->scheduler_consistency_passed);

  out_report->idle_thread_passed =
      scheduler_get_idle_task() != 0 &&
      scheduler_get_idle_task()->base_priority == SCHEDULER_PRIORITY_MIN &&
      scheduler_get_idle_task()->stack != 0;
  count_result(out_report, out_report->idle_thread_passed);

  out_report->priority_aging_contract_passed =
      SCHEDULER_PRIORITY_LEVELS == ATOMS_THREAD_PRIORITY_LEVELS &&
      SCHEDULER_AGING_INTERVAL_TICKS > 0;
  count_result(out_report, out_report->priority_aging_contract_passed);

  scheduler_get_diagnostics(&scheduler_diagnostics);
  out_report->diagnostics_passed =
      scheduler_diagnostics.context_switches ==
          scheduler_get_context_switch_count() &&
      scheduler_diagnostics.tick_count == scheduler_get_tick_count();
  count_result(out_report, out_report->diagnostics_passed);
  return out_report->failed == 0;
}
