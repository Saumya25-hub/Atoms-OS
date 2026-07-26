#include "user_mode.h"
#include "../../drivers/display/display.h"
#include "../memory/vmm/include/paging.h"
#include "../memory/vmm/include/vmm.h"
#include "../process/process_manager.h"
#include "../scheduler/include/scheduler.h"
#include "../thread/thread_manager.h"
#include <stddef.h>

static ATOMS_UserModeDiagnostics g_user_diag;
static bool g_user_initialized;

static void zero_bytes(void *memory, uint64_t size) {
  uint8_t *bytes = (uint8_t *)memory;
  for (uint64_t i = 0; i < size; ++i)
    bytes[i] = 0;
}

static bool add_overflow(uint64_t a, uint64_t b, uint64_t *out) {
  if (b > UINT64_MAX - a)
    return true;
  *out = a + b;
  return false;
}

void ATOMS_UserMode_Init(void) {
  zero_bytes(&g_user_diag, sizeof(g_user_diag));
  g_user_initialized = true;
}

ATOMS_PrivilegeLevel ATOMS_UserMode_CurrentPrivilege(void) {
  Task *task = scheduler_current_task();
  if (task && task->is_user_task)
    return ATOMS_PRIVILEGE_RING3;
  return ATOMS_PRIVILEGE_RING0;
}

bool ATOMS_UserMode_IsCanonical(uint64_t address) {
  uint64_t upper = address >> 48;
  return upper == 0 || upper == 0xFFFF;
}

bool ATOMS_UserMode_IsUserRange(uint64_t address, uint64_t size) {
  uint64_t end;
  if (size == 0 || !ATOMS_UserMode_IsCanonical(address) ||
      add_overflow(address, size - 1, &end))
    return false;
  return address >= ATOMS_USER_MIN_ADDRESS && end <= ATOMS_USER_MAX_ADDRESS;
}

bool ATOMS_UserMode_ValidateAddress(void *pml4, uint64_t address, uint64_t size,
                                    uint32_t access) {
  if (!ATOMS_UserMode_IsUserRange(address, size)) {
    ++g_user_diag.invalid_pointer_rejections;
    return false;
  }
  return vmm_validate_user_range(pml4, address, size, access);
}

bool ATOMS_UserMode_CopyFromUser(void *destination, const void *user_source,
                                 uint64_t size) {
  if (!destination || !user_source)
    return false;
  Task *task = scheduler_current_task();
  if (!task || !task->is_user_task ||
      !ATOMS_UserMode_ValidateAddress(task->pml4, (uint64_t)user_source, size,
                                      VMM_ACCESS_READ))
    return false;
  uint8_t *dst = (uint8_t *)destination;
  const uint8_t *src = (const uint8_t *)user_source;
  for (uint64_t i = 0; i < size; ++i)
    dst[i] = src[i];
  return true;
}

bool ATOMS_UserMode_CopyToUser(void *user_destination, const void *source,
                               uint64_t size) {
  if (!source || !user_destination)
    return false;
  Task *task = scheduler_current_task();
  if (!task || !task->is_user_task ||
      !ATOMS_UserMode_ValidateAddress(task->pml4, (uint64_t)user_destination,
                                      size, VMM_ACCESS_WRITE))
    return false;
  uint8_t *dst = (uint8_t *)user_destination;
  const uint8_t *src = (const uint8_t *)source;
  for (uint64_t i = 0; i < size; ++i)
    dst[i] = src[i];
  return true;
}

bool ATOMS_UserMode_ValidateEntry(void *pml4, uint64_t entry_point,
                                  uint64_t stack_pointer) {
  return ATOMS_UserMode_ValidateAddress(pml4, entry_point, 1,
                                        VMM_ACCESS_READ | VMM_ACCESS_EXECUTE) &&
         ATOMS_UserMode_ValidateAddress(pml4, stack_pointer - 1, 1,
                                        VMM_ACCESS_READ | VMM_ACCESS_WRITE);
}

void ATOMS_UserMode_RecordTransition(bool entering_user) {
  if (entering_user)
    ++g_user_diag.ring_entries;
  else
    ++g_user_diag.ring_returns;
}

uint64_t ATOMS_UserMode_HandleException(registers_t *registers,
                                        uint64_t fault_address) {
  if (!registers)
    return 0;
  Task *task = scheduler_current_task();
  bool from_user = (registers->cs & 3U) == 3U;
  if (from_user) {
    ++g_user_diag.user_exceptions;
    if (registers->int_no == 14U)
      ++g_user_diag.page_faults;
    if (registers->int_no == 13U)
      ++g_user_diag.protection_faults;
    if (registers->int_no == 13U || registers->int_no == 14U)
      ++g_user_diag.privilege_violations;
    g_user_diag.last_fault_address = fault_address;
    g_user_diag.last_exception.rip = registers->rip;
    g_user_diag.last_exception.rsp = registers->rsp;
    g_user_diag.last_exception.rflags = registers->rflags;
    g_user_diag.last_exception.fault_address = fault_address;
    g_user_diag.last_exception.error_code = registers->err_code;
    g_user_diag.last_exception.address_space = task ? (uint64_t)task->pml4 : 0;
    g_user_diag.last_exception.vector = (uint32_t)registers->int_no;
    g_user_diag.last_exception.pid = task ? task->owner_pid : 0;
    g_user_diag.last_exception.tid = task ? (uint32_t)task->id : 0;
    g_user_diag.last_exception.privilege = ATOMS_PRIVILEGE_RING3;
    if (task && task != scheduler_get_idle_task()) {
      uint32_t pid = task->owner_pid ? task->owner_pid : (uint32_t)task->id;
      ATOMS_Process_RecordUserException(pid, (uint32_t)registers->int_no,
                                        registers->rip, fault_address,
                                        registers->err_code);
      scheduler_terminate_task(task);
      if (pid)
        ATOMS_Process_Terminate(pid, -(int32_t)registers->int_no);
      ++g_user_diag.terminated_user_processes;
      return 0;
    }
  } else {
    ++g_user_diag.kernel_exceptions;
  }
  return 0;
}

void ATOMS_UserMode_GetDiagnostics(ATOMS_UserModeDiagnostics *out) {
  if (!out)
    return;
  *out = g_user_diag;
  Task *task = scheduler_current_task();
  out->current_address_space = task ? (uint64_t)task->pml4 : 0;
  out->current_pid = task ? task->owner_pid : 0;
  out->current_tid = task ? (uint32_t)task->id : 0;
  out->current_ring = (uint8_t)ATOMS_UserMode_CurrentPrivilege();
  out->current_privilege = out->current_ring;
}

void ATOMS_UserMode_DumpPrivilege(void) {
  ATOMS_UserModeDiagnostics diagnostics;
  ATOMS_UserMode_GetDiagnostics(&diagnostics);
  display_print("\n--- Privilege Diagnostics ---\nRing: ");
  display_print_dec(diagnostics.current_ring);
  display_print(" PID/TID: ");
  display_print_dec(diagnostics.current_pid);
  display_print("/");
  display_print_dec(diagnostics.current_tid);
  display_print(" Address Space: ");
  display_print_hex(diagnostics.current_address_space);
  display_print("\nTransitions in/out: ");
  display_print_dec(diagnostics.ring_entries);
  display_print("/");
  display_print_dec(diagnostics.ring_returns);
  display_print("\n-----------------------------\n");
}

void ATOMS_UserMode_DumpException(void) {
  ATOMS_UserModeDiagnostics diagnostics;
  ATOMS_UserMode_GetDiagnostics(&diagnostics);
  display_print("\n--- User Exception Diagnostics ---\nVector: ");
  display_print_dec(diagnostics.last_exception.vector);
  display_print(" RIP: ");
  display_print_hex(diagnostics.last_exception.rip);
  display_print(" Fault: ");
  display_print_hex(diagnostics.last_exception.fault_address);
  display_print("\nUser exceptions: ");
  display_print_dec(diagnostics.user_exceptions);
  display_print(" Page faults: ");
  display_print_dec(diagnostics.page_faults);
  display_print(" Protection faults: ");
  display_print_dec(diagnostics.protection_faults);
  display_print("\n----------------------------------\n");
}

void ATOMS_UserMode_DumpMappings(void *pml4, uint64_t start, uint64_t end) {
  vmm_dump_address_space(pml4, start, end);
}
