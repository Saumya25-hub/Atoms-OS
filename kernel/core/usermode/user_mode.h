#ifndef ATOMS_USER_MODE_H
#define ATOMS_USER_MODE_H

#include "../interrupt/include/isr.h"
#include "../scheduler/include/task.h"
#include <stdbool.h>
#include <stdint.h>

#define ATOMS_USER_PAGE_SIZE 4096ULL
#define ATOMS_USER_MIN_ADDRESS 0x0000000001000000ULL
#define ATOMS_USER_MAX_ADDRESS 0x00007FFFFFFFFFFFULL
#define ATOMS_USER_STACK_TOP 0x00007FFFFFFFE000ULL
#define ATOMS_USER_STACK_GUARD_PAGES 1U
#define ATOMS_USER_DEFAULT_STACK_PAGES 16U

typedef enum {
  ATOMS_PRIVILEGE_RING0 = 0,
  ATOMS_PRIVILEGE_RING3 = 3,
  ATOMS_PRIVILEGE_INVALID = 0xFF
} ATOMS_PrivilegeLevel;

typedef enum {
  ATOMS_USER_ACCESS_READ = 1U << 0,
  ATOMS_USER_ACCESS_WRITE = 1U << 1,
  ATOMS_USER_ACCESS_EXECUTE = 1U << 2
} ATOMS_UserAccess;

typedef enum {
  ATOMS_USER_EXCEPTION_DIVIDE = 0,
  ATOMS_USER_EXCEPTION_INVALID_OPCODE = 6,
  ATOMS_USER_EXCEPTION_STACK = 12,
  ATOMS_USER_EXCEPTION_GPF = 13,
  ATOMS_USER_EXCEPTION_PAGE_FAULT = 14
} ATOMS_UserExceptionVector;

typedef struct {
  uint64_t rip;
  uint64_t rsp;
  uint64_t rflags;
  uint64_t fault_address;
  uint64_t error_code;
  uint64_t address_space;
  uint32_t vector;
  uint32_t pid;
  uint32_t tid;
  uint8_t privilege;
  uint8_t access;
  uint16_t reserved;
} ATOMS_UserExceptionContext;

typedef struct {
  uint64_t ring_entries;
  uint64_t ring_returns;
  uint64_t user_exceptions;
  uint64_t kernel_exceptions;
  uint64_t page_faults;
  uint64_t protection_faults;
  uint64_t privilege_violations;
  uint64_t invalid_pointer_rejections;
  uint64_t address_spaces_created;
  uint64_t address_spaces_destroyed;
  uint64_t user_pages_mapped;
  uint64_t guard_pages_created;
  uint64_t terminated_user_processes;
  uint64_t last_fault_address;
  uint64_t current_address_space;
  uint32_t current_pid;
  uint32_t current_tid;
  uint8_t current_ring;
  uint8_t current_privilege;
  uint16_t reserved;
  ATOMS_UserExceptionContext last_exception;
} ATOMS_UserModeDiagnostics;

void ATOMS_UserMode_Init(void);
ATOMS_PrivilegeLevel ATOMS_UserMode_CurrentPrivilege(void);
bool ATOMS_UserMode_IsCanonical(uint64_t address);
bool ATOMS_UserMode_IsUserRange(uint64_t address, uint64_t size);
bool ATOMS_UserMode_ValidateAddress(void *pml4, uint64_t address, uint64_t size,
                                    uint32_t access);
bool ATOMS_UserMode_CopyFromUser(void *destination, const void *user_source,
                                 uint64_t size);
bool ATOMS_UserMode_CopyToUser(void *user_destination, const void *source,
                               uint64_t size);
bool ATOMS_UserMode_ValidateEntry(void *pml4, uint64_t entry_point,
                                  uint64_t stack_pointer);
void ATOMS_UserMode_RecordTransition(bool entering_user);
uint64_t ATOMS_UserMode_HandleException(registers_t *registers,
                                        uint64_t fault_address);
void ATOMS_UserMode_GetDiagnostics(ATOMS_UserModeDiagnostics *out);
void ATOMS_UserMode_DumpPrivilege(void);
void ATOMS_UserMode_DumpException(void);
void ATOMS_UserMode_DumpMappings(void *pml4, uint64_t start, uint64_t end);

#endif
