#ifndef ATOMS_X86_64_SMP_H
#define ATOMS_X86_64_SMP_H

#include "../gdt/gdt.h"
#include <stdbool.h>
#include <stdint.h>

#define ATOMS_MAX_CPUS 8U
#define ATOMS_MAX_IOAPICS 4U
#define ATOMS_MAX_ISO 16U
#define ATOMS_CPU_NONE UINT32_MAX

struct Task;

typedef enum {
  ATOMS_CPU_ABSENT = 0,
  ATOMS_CPU_DISCOVERED,
  ATOMS_CPU_CONFIGURED,
  ATOMS_CPU_STARTING,
  ATOMS_CPU_ONLINE,
  ATOMS_CPU_FAILED
} ATOMS_CPUState;

typedef enum {
  ATOMS_IPI_RESCHEDULE = 0,
  ATOMS_IPI_TLB_SHOOTDOWN,
  ATOMS_IPI_CALL_FUNCTION,
  ATOMS_IPI_STOP,
  ATOMS_IPI_DIAGNOSTIC,
  ATOMS_IPI_MESSAGE_COUNT
} ATOMS_IPIMessage;

typedef struct {
  uint8_t processor_id;
  uint8_t apic_id;
  bool enabled;
  bool online_capable;
} ATOMS_CPUDescriptor;

typedef struct {
  uint8_t id;
  uint32_t address;
  uint32_t gsi_base;
} ATOMS_IOAPICDescriptor;

typedef struct {
  uint8_t bus;
  uint8_t source;
  uint32_t gsi;
  uint16_t flags;
} ATOMS_InterruptOverride;

typedef struct {
  bool acpi_found;
  bool madt_found;
  bool checksum_valid;
  bool truncated;
  bool bsp_only_fallback;
  uint8_t acpi_revision;
  uint32_t discovered_count;
  uint32_t configured_count;
  uint32_t online_count;
  uint32_t scheduling_count;
  uint32_t ioapic_count;
  uint32_t override_count;
  uint32_t bsp_apic_id;
  uint64_t local_apic_address;
  ATOMS_CPUDescriptor cpus[ATOMS_MAX_CPUS];
  ATOMS_IOAPICDescriptor ioapics[ATOMS_MAX_IOAPICS];
  ATOMS_InterruptOverride overrides[ATOMS_MAX_ISO];
} ATOMS_CPUTopology;

typedef struct {
  uint32_t logical_id;
  uint32_t apic_id;
  volatile ATOMS_CPUState state;
  bool is_bsp;
  bool scheduler_enabled;
  uint16_t interrupt_depth;
  uint16_t preempt_depth;
  struct Task *current_task;
  struct Task *idle_task;
  void *scheduler_queue;
  tss_t *tss;
  uint64_t syscall_stack_top;
  void *syscall_active_frame;
  uint32_t syscall_nesting;
  uint64_t scheduler_ticks;
  uint64_t context_switches;
  uint64_t migrations_in;
  uint64_t migrations_out;
  uint64_t ipi_sent[ATOMS_IPI_MESSAGE_COUNT];
  uint64_t ipi_received[ATOMS_IPI_MESSAGE_COUNT];
} ATOMS_PerCPU;

typedef struct {
  bool supported;
  bool enabled;
  bool x2apic;
  uint64_t physical_base;
  uint32_t bsp_apic_id;
  uint64_t ipi_send_failures;
} ATOMS_LocalAPICState;

void atoms_smp_discover(void);
void atoms_smp_initialize_bsp(void);
void atoms_smp_prepare_aps(void);
const ATOMS_CPUTopology *atoms_smp_topology(void);
ATOMS_PerCPU *atoms_cpu_local(void);
ATOMS_PerCPU *atoms_cpu_by_id(uint32_t logical_id);
uint32_t atoms_cpu_id(void);
uint32_t atoms_cpu_online_count(void);
void atoms_cpu_bind_current(struct Task *task);
void atoms_cpu_bind_idle(struct Task *task);
void atoms_cpu_set_scheduler_enabled(bool enabled);

bool atoms_lapic_probe(void);
const ATOMS_LocalAPICState *atoms_lapic_state(void);
bool atoms_lapic_enable_foundation(void);
void atoms_lapic_eoi(void);
bool atoms_ipi_send(uint32_t logical_id, ATOMS_IPIMessage message);
void atoms_ipi_note_received(ATOMS_IPIMessage message);

void atoms_smp_register_ipi_handlers(void);
void ap_main(uint32_t logical_id);

bool atoms_smp_self_test(void);
void atoms_smp_print_diagnostics(void);

#endif
