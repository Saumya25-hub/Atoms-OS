#ifndef ATOMS_EXECUTION_CONTRACT_H
#define ATOMS_EXECUTION_CONTRACT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ATOMS_EXECUTION_TRACE_CAPACITY 128U
#define ATOMS_EXECUTION_NAME_LENGTH 32U

typedef enum {
  ATOMS_EXEC_OK = 0,
  ATOMS_EXEC_ERR_INVALID_ARGUMENT = 1,
  ATOMS_EXEC_ERR_NOT_INITIALIZED = 2,
  ATOMS_EXEC_ERR_ALREADY_EXISTS = 3,
  ATOMS_EXEC_ERR_NOT_FOUND = 4,
  ATOMS_EXEC_ERR_TABLE_FULL = 5,
  ATOMS_EXEC_ERR_ID_EXHAUSTED = 6,
  ATOMS_EXEC_ERR_INVALID_STATE = 7,
  ATOMS_EXEC_ERR_OWNERSHIP = 8,
  ATOMS_EXEC_ERR_BUSY = 9,
  ATOMS_EXEC_ERR_PERMISSION = 10,
  ATOMS_EXEC_ERR_RESOURCE = 11,
  ATOMS_EXEC_ERR_CORRUPT = 12,
  ATOMS_EXEC_ERR_UNSUPPORTED = 13
} ATOMS_ExecutionErrorCode;

typedef enum {
  ATOMS_EXEC_SEVERITY_INFO = 0,
  ATOMS_EXEC_SEVERITY_WARNING = 1,
  ATOMS_EXEC_SEVERITY_ERROR = 2,
  ATOMS_EXEC_SEVERITY_FATAL = 3
} ATOMS_ExecutionSeverity;

typedef struct {
  uint16_t module;
  uint16_t code;
  uint8_t severity;
  uint8_t reserved;
  uint32_t object_id;
  uint32_t owner_id;
  uint32_t current_state;
  uint32_t requested_state;
  uint64_t timestamp;
  const char *cause;
  const char *recovery;
} ATOMS_ExecutionError;

typedef enum {
  ATOMS_TRACE_PROCESS_CREATE = 1,
  ATOMS_TRACE_PROCESS_STATE = 2,
  ATOMS_TRACE_PROCESS_EXIT = 3,
  ATOMS_TRACE_THREAD_CREATE = 4,
  ATOMS_TRACE_THREAD_STATE = 5,
  ATOMS_TRACE_THREAD_EXIT = 6,
  ATOMS_TRACE_SCHEDULER_DECISION = 7,
  ATOMS_TRACE_CONTEXT_SWITCH = 8,
  ATOMS_TRACE_RESOURCE = 9,
  ATOMS_TRACE_WARNING = 10
} ATOMS_ExecutionTraceEvent;

typedef struct {
  uint64_t sequence;
  uint64_t timestamp;
  uint16_t event;
  uint16_t module;
  uint32_t object_id;
  uint32_t related_id;
  uint32_t state;
  uint32_t value;
} ATOMS_ExecutionTraceRecord;

typedef struct {
  uint32_t process_capacity;
  uint32_t process_active;
  uint32_t thread_capacity;
  uint32_t thread_active;
  uint64_t context_switches;
  uint64_t scheduler_ticks;
  uint64_t trace_sequence;
  uint32_t error_count;
  uint32_t warning_count;
} ATOMS_ExecutionDiagnostics;

void ATOMS_Execution_Init(void);
void ATOMS_Execution_Trace(uint16_t module, ATOMS_ExecutionTraceEvent event,
                           uint32_t object_id, uint32_t related_id,
                           uint32_t state, uint32_t value);
uint32_t ATOMS_Execution_TraceCopy(ATOMS_ExecutionTraceRecord *out,
                                   uint32_t capacity);
const ATOMS_ExecutionTraceRecord *ATOMS_Execution_TraceAt(uint32_t index);
void ATOMS_Execution_RecordError(const ATOMS_ExecutionError *error);
void ATOMS_Execution_GetDiagnostics(ATOMS_ExecutionDiagnostics *out);
const char *ATOMS_Execution_ErrorName(ATOMS_ExecutionErrorCode code);
bool ATOMS_ProcessStateTransitionValid(uint32_t current, uint32_t requested);
bool ATOMS_ThreadStateTransitionValid(uint32_t current, uint32_t requested);

#ifdef __cplusplus
}
#endif

#endif
