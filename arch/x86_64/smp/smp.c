#include "smp.h"
#include "../../../kernel/drivers/display/display.h"
#include <stddef.h>

#define ACPI_MAX_TABLE_LENGTH (1024U * 1024U)
#define IA32_APIC_BASE_MSR 0x1BU
#define APIC_BASE_ENABLE (1ULL << 11)
#define APIC_BASE_BSP (1ULL << 8)
#define APIC_ID_REGISTER 0x20U
#define APIC_EOI_REGISTER 0xB0U
#define APIC_SPURIOUS_REGISTER 0xF0U
#define APIC_ICR_LOW 0x300U
#define APIC_ICR_HIGH 0x310U
#define APIC_ICR_PENDING (1U << 12)
#define APIC_VECTOR_RESCHEDULE 0xF0U
#define APIC_VECTOR_TLB 0xF1U
#define APIC_VECTOR_CALL 0xF2U
#define APIC_VECTOR_STOP 0xF3U
#define APIC_VECTOR_DIAGNOSTIC 0xF4U

typedef struct {
  char signature[8];
  uint8_t checksum;
  char oem_id[6];
  uint8_t revision;
  uint32_t rsdt_address;
  uint32_t length;
  uint64_t xsdt_address;
  uint8_t extended_checksum;
  uint8_t reserved[3];
} __attribute__((packed)) ACPIRSDP;

typedef struct {
  char signature[4];
  uint32_t length;
  uint8_t revision;
  uint8_t checksum;
  char oem_id[6];
  char oem_table_id[8];
  uint32_t oem_revision;
  uint32_t creator_id;
  uint32_t creator_revision;
} __attribute__((packed)) ACPISDTHeader;

typedef struct {
  ACPISDTHeader header;
  uint32_t local_apic_address;
  uint32_t flags;
  uint8_t entries[];
} __attribute__((packed)) ACPIMADT;

typedef struct {
  uint8_t type;
  uint8_t length;
} __attribute__((packed)) MADTEntry;

typedef struct {
  MADTEntry header;
  uint8_t processor_id;
  uint8_t apic_id;
  uint32_t flags;
} __attribute__((packed)) MADTLocalAPIC;

typedef struct {
  MADTEntry header;
  uint8_t id;
  uint8_t reserved;
  uint32_t address;
  uint32_t gsi_base;
} __attribute__((packed)) MADTIOAPIC;

typedef struct {
  MADTEntry header;
  uint8_t bus;
  uint8_t source;
  uint32_t gsi;
  uint16_t flags;
} __attribute__((packed)) MADTISO;

typedef struct {
  MADTEntry header;
  uint16_t reserved;
  uint64_t address;
} __attribute__((packed)) MADTLAPICOverride;

typedef struct {
  MADTEntry header;
  uint16_t reserved;
  uint32_t x2apic_id;
  uint32_t flags;
  uint32_t processor_uid;
} __attribute__((packed)) MADTX2APIC;

extern tss_t tss;

static ATOMS_CPUTopology g_topology;
static ATOMS_PerCPU g_percpu[ATOMS_MAX_CPUS];
static ATOMS_LocalAPICState g_lapic;
static ATOMS_PerCPU *g_cpu_local = &g_percpu[0];

static void zero_bytes(void *memory, uint64_t size) {
  uint8_t *bytes = (uint8_t *)memory;
  for (uint64_t i = 0; i < size; ++i)
    bytes[i] = 0;
}

static bool signature_equal(const char *left, const char *right,
                            uint32_t length) {
  for (uint32_t i = 0; i < length; ++i) {
    if (left[i] != right[i])
      return false;
  }
  return true;
}

static bool checksum_ok(const void *table, uint32_t length) {
  if (!table || !length || length > ACPI_MAX_TABLE_LENGTH)
    return false;
  const uint8_t *bytes = (const uint8_t *)table;
  uint8_t sum = 0;
  for (uint32_t i = 0; i < length; ++i)
    sum = (uint8_t)(sum + bytes[i]);
  return sum == 0;
}

static uint64_t read_ebda_base(void) {
  uint16_t segment = *(volatile uint16_t *)(uintptr_t)0x40EU;
  return (uint64_t)segment << 4;
}

static const ACPIRSDP *scan_rsdp(uint64_t start, uint64_t end) {
  if (end <= start)
    return NULL;
  start = (start + 15U) & ~15ULL;
  for (uint64_t address = start; address + 20U <= end; address += 16U) {
    const ACPIRSDP *rsdp = (const ACPIRSDP *)(uintptr_t)address;
    if (!signature_equal(rsdp->signature, "RSD PTR ", 8U))
      continue;
    if (!checksum_ok(rsdp, 20U))
      continue;
    if (rsdp->revision >= 2U) {
      if (rsdp->length < 36U || rsdp->length > sizeof(ACPIRSDP) ||
          !checksum_ok(rsdp, rsdp->length))
        continue;
    }
    return rsdp;
  }
  return NULL;
}

static const ACPIRSDP *find_rsdp(void) {
  uint64_t ebda = read_ebda_base();
  if (ebda >= 0x400U && ebda < 0xA0000U) {
    const ACPIRSDP *rsdp = scan_rsdp(ebda, ebda + 1024U);
    if (rsdp)
      return rsdp;
  }
  return scan_rsdp(0xE0000U, 0x100000U);
}

static bool valid_sdt(const ACPISDTHeader *header) {
  return header && header->length >= sizeof(ACPISDTHeader) &&
         header->length <= ACPI_MAX_TABLE_LENGTH &&
         checksum_ok(header, header->length);
}

static const ACPISDTHeader *find_table(const ACPIRSDP *rsdp,
                                       const char signature[4]) {
  bool use_xsdt = rsdp->revision >= 2U && rsdp->xsdt_address != 0;
  const ACPISDTHeader *root =
      (const ACPISDTHeader *)(uintptr_t)(use_xsdt ? rsdp->xsdt_address
                                                  : rsdp->rsdt_address);
  if (!valid_sdt(root))
    return NULL;
  if (use_xsdt && !signature_equal(root->signature, "XSDT", 4U))
    return NULL;
  if (!use_xsdt && !signature_equal(root->signature, "RSDT", 4U))
    return NULL;
  uint32_t width = use_xsdt ? 8U : 4U;
  uint32_t bytes = root->length - sizeof(ACPISDTHeader);
  if (bytes % width)
    return NULL;
  const uint8_t *entries = (const uint8_t *)root + sizeof(ACPISDTHeader);
  for (uint32_t offset = 0; offset < bytes; offset += width) {
    uint64_t address = use_xsdt ? *(const uint64_t *)(entries + offset)
                                : *(const uint32_t *)(entries + offset);
    const ACPISDTHeader *candidate = (const ACPISDTHeader *)(uintptr_t)address;
    if (valid_sdt(candidate) &&
        signature_equal(candidate->signature, signature, 4U))
      return candidate;
  }
  return NULL;
}

static void configure_fallback(void) {
  g_topology.bsp_only_fallback = true;
  g_topology.discovered_count = 1;
  g_topology.configured_count = 1;
  g_topology.cpus[0].enabled = true;
  g_topology.cpus[0].online_capable = true;
  g_topology.cpus[0].apic_id = (uint8_t)g_topology.bsp_apic_id;
}

static void add_cpu(uint32_t processor_id, uint32_t apic_id, uint32_t flags) {
  bool enabled = (flags & 1U) != 0;
  bool online_capable = (flags & 2U) != 0;
  if (!enabled && !online_capable)
    return;
  for (uint32_t i = 0; i < g_topology.discovered_count; ++i) {
    if (g_topology.cpus[i].apic_id == (uint8_t)apic_id)
      return;
  }
  if (g_topology.discovered_count >= ATOMS_MAX_CPUS || apic_id > 255U) {
    g_topology.truncated = true;
    return;
  }
  ATOMS_CPUDescriptor *cpu = &g_topology.cpus[g_topology.discovered_count++];
  cpu->processor_id = (uint8_t)processor_id;
  cpu->apic_id = (uint8_t)apic_id;
  cpu->enabled = enabled;
  cpu->online_capable = online_capable;
  if (enabled)
    ++g_topology.configured_count;
}

static bool parse_madt(const ACPIMADT *madt) {
  if (!madt || madt->header.length < sizeof(ACPIMADT))
    return false;
  g_topology.local_apic_address = madt->local_apic_address;
  const uint8_t *cursor = madt->entries;
  const uint8_t *end = (const uint8_t *)madt + madt->header.length;
  while (cursor + sizeof(MADTEntry) <= end) {
    const MADTEntry *entry = (const MADTEntry *)cursor;
    if (entry->length < sizeof(MADTEntry) || cursor + entry->length > end)
      return false;
    switch (entry->type) {
    case 0:
      if (entry->length >= sizeof(MADTLocalAPIC)) {
        const MADTLocalAPIC *lapic = (const MADTLocalAPIC *)entry;
        add_cpu(lapic->processor_id, lapic->apic_id, lapic->flags);
      }
      break;
    case 1:
      if (entry->length >= sizeof(MADTIOAPIC) &&
          g_topology.ioapic_count < ATOMS_MAX_IOAPICS) {
        const MADTIOAPIC *io = (const MADTIOAPIC *)entry;
        ATOMS_IOAPICDescriptor *out =
            &g_topology.ioapics[g_topology.ioapic_count++];
        out->id = io->id;
        out->address = io->address;
        out->gsi_base = io->gsi_base;
      }
      break;
    case 2:
      if (entry->length >= sizeof(MADTISO) &&
          g_topology.override_count < ATOMS_MAX_ISO) {
        const MADTISO *iso = (const MADTISO *)entry;
        ATOMS_InterruptOverride *out =
            &g_topology.overrides[g_topology.override_count++];
        out->bus = iso->bus;
        out->source = iso->source;
        out->gsi = iso->gsi;
        out->flags = iso->flags;
      }
      break;
    case 5:
      if (entry->length >= sizeof(MADTLAPICOverride))
        g_topology.local_apic_address =
            ((const MADTLAPICOverride *)entry)->address;
      break;
    case 9:
      if (entry->length >= sizeof(MADTX2APIC)) {
        const MADTX2APIC *x2 = (const MADTX2APIC *)entry;
        add_cpu(x2->processor_uid, x2->x2apic_id, x2->flags);
      }
      break;
    default:
      break;
    }
    cursor += entry->length;
  }
  return cursor == end;
}

static uint64_t read_msr(uint32_t msr) {
  uint32_t low;
  uint32_t high;
  __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
  return ((uint64_t)high << 32) | low;
}

static void write_msr(uint32_t msr, uint64_t value) {
  __asm__ volatile("wrmsr"
                   :
                   : "c"(msr), "a"((uint32_t)value),
                     "d"((uint32_t)(value >> 32))
                   : "memory");
}

static uint32_t lapic_read(uint32_t offset) {
  volatile uint32_t *register_address =
      (volatile uint32_t *)(uintptr_t)(g_lapic.physical_base + offset);
  return *register_address;
}

static void lapic_write(uint32_t offset, uint32_t value) {
  volatile uint32_t *register_address =
      (volatile uint32_t *)(uintptr_t)(g_lapic.physical_base + offset);
  *register_address = value;
  (void)*register_address;
}

void atoms_smp_discover(void) {
  zero_bytes(&g_topology, sizeof(g_topology));
  (void)atoms_lapic_probe();
  g_topology.bsp_apic_id = g_lapic.bsp_apic_id;
  const ACPIRSDP *rsdp = find_rsdp();
  if (!rsdp) {
    configure_fallback();
    return;
  }
  g_topology.acpi_found = true;
  g_topology.acpi_revision = rsdp->revision;
  g_topology.checksum_valid = true;
  const ACPISDTHeader *header = find_table(rsdp, "APIC");
  if (!header) {
    configure_fallback();
    return;
  }
  g_topology.madt_found = true;
  if (!parse_madt((const ACPIMADT *)header) ||
      g_topology.configured_count == 0) {
    zero_bytes(g_topology.cpus, sizeof(g_topology.cpus));
    g_topology.discovered_count = 0;
    g_topology.configured_count = 0;
    configure_fallback();
    return;
  }
  bool bsp_present = false;
  for (uint32_t i = 0; i < g_topology.discovered_count; ++i) {
    if (g_topology.cpus[i].apic_id == (uint8_t)g_topology.bsp_apic_id)
      bsp_present = true;
  }
  if (!bsp_present)
    add_cpu(0xFFU, g_topology.bsp_apic_id, 1U);
}

void atoms_smp_initialize_bsp(void) {
  zero_bytes(g_percpu, sizeof(g_percpu));
  uint32_t bsp_slot = 0;
  for (uint32_t i = 0; i < g_topology.discovered_count; ++i) {
    if (g_topology.cpus[i].apic_id == (uint8_t)g_topology.bsp_apic_id) {
      bsp_slot = i;
      break;
    }
  }
  if (bsp_slot != 0) {
    ATOMS_CPUDescriptor descriptor = g_topology.cpus[0];
    g_topology.cpus[0] = g_topology.cpus[bsp_slot];
    g_topology.cpus[bsp_slot] = descriptor;
  }
  for (uint32_t i = 0; i < g_topology.discovered_count; ++i) {
    g_percpu[i].logical_id = i;
    g_percpu[i].apic_id = g_topology.cpus[i].apic_id;
    g_percpu[i].state = ATOMS_CPU_CONFIGURED;
  }
  g_cpu_local = &g_percpu[0];
  g_cpu_local->is_bsp = true;
  g_cpu_local->state = ATOMS_CPU_ONLINE;
  g_cpu_local->tss = &tss;
  g_topology.online_count = 1;
  g_topology.scheduling_count = 0;

  write_msr(0xC0000101U, (uint64_t)&g_percpu[0]);
  write_msr(0xC0000102U, (uint64_t)&g_percpu[0]);
}

extern struct Task *scheduler_create_idle_task_cpu(uint32_t cpu_id);

extern uint8_t ap_trampoline_start[];
extern uint8_t ap_trampoline_end[];
extern uint8_t ap_trampoline_cpuid[];
extern uint8_t ap_trampoline_stack[];
extern void *kcalloc(size_t num, size_t size);
extern void syscall_entry(void);
extern void idt_init(void);
extern void isr_register_handler(uint8_t vector, void *handler);

static uint64_t ipi_reschedule_handler(void *regs) {
  (void)regs;
  atoms_ipi_note_received(ATOMS_IPI_RESCHEDULE);
  atoms_lapic_eoi();
  return 0;
}

static uint64_t ipi_tlb_handler(void *regs) {
  (void)regs;
  atoms_ipi_note_received(ATOMS_IPI_TLB_SHOOTDOWN);
  uint64_t cr3;
  __asm__ volatile("mov %%cr3, %0; mov %0, %%cr3" : "=r"(cr3) :: "memory");
  atoms_lapic_eoi();
  return 0;
}

static uint64_t ipi_call_handler(void *regs) {
  (void)regs;
  atoms_ipi_note_received(ATOMS_IPI_CALL_FUNCTION);
  atoms_lapic_eoi();
  return 0;
}

static uint64_t ipi_stop_handler(void *regs) {
  (void)regs;
  atoms_ipi_note_received(ATOMS_IPI_STOP);
  atoms_lapic_eoi();
  __asm__ volatile("cli");
  while (1) {
    __asm__ volatile("hlt");
  }
  return 0;
}

static uint64_t ipi_diag_handler(void *regs) {
  (void)regs;
  atoms_ipi_note_received(ATOMS_IPI_DIAGNOSTIC);
  atoms_lapic_eoi();
  return 0;
}

void atoms_smp_register_ipi_handlers(void) {
  isr_register_handler(APIC_VECTOR_RESCHEDULE, ipi_reschedule_handler);
  isr_register_handler(APIC_VECTOR_TLB, ipi_tlb_handler);
  isr_register_handler(APIC_VECTOR_CALL, ipi_call_handler);
  isr_register_handler(APIC_VECTOR_STOP, ipi_stop_handler);
  isr_register_handler(APIC_VECTOR_DIAGNOSTIC, ipi_diag_handler);
}

void ap_main(uint32_t logical_id) {
  if (logical_id >= ATOMS_MAX_CPUS)
    return;

  idt_init();
  gdt_init_cpu(logical_id);

  write_msr(0xC0000101U, (uint64_t)&g_percpu[logical_id]);
  write_msr(0xC0000102U, (uint64_t)&g_percpu[logical_id]);

  write_msr(0xC0000081U, ((uint64_t)0x08 << 32) | ((uint64_t)0x1B << 48));
  write_msr(0xC0000082U, (uint64_t)&syscall_entry);
  write_msr(0xC0000084U, 0x200U);

  (void)atoms_lapic_enable_foundation();

  g_percpu[logical_id].logical_id = logical_id;
  g_percpu[logical_id].apic_id = g_topology.cpus[logical_id].apic_id;
  g_percpu[logical_id].state = ATOMS_CPU_ONLINE;

  struct Task *idle = scheduler_create_idle_task_cpu(logical_id);
  g_percpu[logical_id].idle_task = idle;
  g_percpu[logical_id].current_task = idle;
  g_percpu[logical_id].scheduler_enabled = true;

  __sync_fetch_and_add(&g_topology.online_count, 1);
  __sync_fetch_and_add(&g_topology.scheduling_count, 1);

  __asm__ volatile("sti");

  while (1) {
    __asm__ volatile("hlt");
  }
}

static uint8_t g_ap_stacks[ATOMS_MAX_CPUS][16384] __attribute__((aligned(16)));

void atoms_smp_prepare_aps(void) {
  if (g_topology.discovered_count <= 1)
    return;

  atoms_smp_register_ipi_handlers();

  uint64_t tramp_len = (uint64_t)(ap_trampoline_end - ap_trampoline_start);
  uint8_t *tramp_dest = (uint8_t *)(uintptr_t)0x8000;
  for (uint64_t k = 0; k < tramp_len; k++) {
    tramp_dest[k] = ap_trampoline_start[k];
  }

  uint64_t cpuid_off = (uint64_t)(ap_trampoline_cpuid - ap_trampoline_start);
  uint64_t stack_off = (uint64_t)(ap_trampoline_stack - ap_trampoline_start);

  for (uint32_t i = 1; i < g_topology.discovered_count; ++i) {
    if (i >= ATOMS_MAX_CPUS)
      break;

    uint64_t ap_stack_top = (uint64_t)&g_ap_stacks[i][16384];
    g_percpu[i].syscall_stack_top = ap_stack_top;
    g_percpu[i].tss = gdt_get_tss_cpu(i);
    tss_set_kernel_stack_cpu(i, ap_stack_top);

    *(volatile uint32_t *)(uintptr_t)(0x8000 + cpuid_off) = i;
    *(volatile uint64_t *)(uintptr_t)(0x8000 + stack_off) = ap_stack_top;


    g_percpu[i].state = ATOMS_CPU_STARTING;

    lapic_write(APIC_ICR_HIGH, (uint32_t)g_topology.cpus[i].apic_id << 24);
    lapic_write(APIC_ICR_LOW, 0x00004500U);
    for (volatile int d = 0; d < 100000; d++)
      __asm__ volatile("pause");
    lapic_write(APIC_ICR_LOW, 0x00000500U);
    for (volatile int d = 0; d < 1000000; d++)
      __asm__ volatile("pause");

    lapic_write(APIC_ICR_HIGH, (uint32_t)g_topology.cpus[i].apic_id << 24);
    lapic_write(APIC_ICR_LOW, 0x00000608U);
    for (volatile int d = 0; d < 500000; d++)
      __asm__ volatile("pause");

    if (g_percpu[i].state != ATOMS_CPU_ONLINE) {
      lapic_write(APIC_ICR_HIGH, (uint32_t)g_topology.cpus[i].apic_id << 24);
      lapic_write(APIC_ICR_LOW, 0x00000608U);
    }

    uint32_t wait_timeout = 100000000U;
    while (g_percpu[i].state != ATOMS_CPU_ONLINE && --wait_timeout) {
      __asm__ volatile("pause");
    }

    if (g_percpu[i].state != ATOMS_CPU_ONLINE) {
      g_percpu[i].state = ATOMS_CPU_FAILED;
    }
  }
}

const ATOMS_CPUTopology *atoms_smp_topology(void) { return &g_topology; }

ATOMS_PerCPU *atoms_cpu_local(void) {
  uint32_t low, high;
  __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(0xC0000101U));
  uint64_t addr = ((uint64_t)high << 32) | low;
  if (addr >= (uint64_t)&g_percpu[0] && addr < (uint64_t)&g_percpu[ATOMS_MAX_CPUS]) {
    return (ATOMS_PerCPU *)(uintptr_t)addr;
  }
  return g_cpu_local;
}

ATOMS_PerCPU *atoms_cpu_by_id(uint32_t logical_id) {
  if (logical_id >= g_topology.discovered_count || logical_id >= ATOMS_MAX_CPUS)
    return NULL;
  return &g_percpu[logical_id];
}

uint32_t atoms_cpu_id(void) { return atoms_cpu_local()->logical_id; }

uint32_t atoms_cpu_online_count(void) { return g_topology.online_count; }

void atoms_cpu_bind_current(struct Task *task) {
  atoms_cpu_local()->current_task = task;
}

void atoms_cpu_bind_idle(struct Task *task) { atoms_cpu_local()->idle_task = task; }

void atoms_cpu_set_scheduler_enabled(bool enabled) {
  ATOMS_PerCPU *cpu = atoms_cpu_local();
  if (cpu->scheduler_enabled == enabled)
    return;
  cpu->scheduler_enabled = enabled;
  if (enabled)
    ++g_topology.scheduling_count;
  else if (g_topology.scheduling_count)
    --g_topology.scheduling_count;
}

bool atoms_lapic_probe(void) {
  uint32_t eax;
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;
  __asm__ volatile("cpuid"
                   : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                   : "a"(1U), "c"(0U));
  zero_bytes(&g_lapic, sizeof(g_lapic));
  g_lapic.supported = (edx & (1U << 9)) != 0;
  if (!g_lapic.supported)
    return false;
  uint64_t base = read_msr(IA32_APIC_BASE_MSR);
  g_lapic.physical_base = base & 0xFFFFF000ULL;
  g_lapic.enabled = (base & APIC_BASE_ENABLE) != 0;
  g_lapic.x2apic = (base & (1ULL << 10)) != 0;
  g_lapic.bsp_apic_id = g_lapic.x2apic ? (uint32_t)read_msr(0x802U)
                                       : (lapic_read(APIC_ID_REGISTER) >> 24);
  return true;
}

const ATOMS_LocalAPICState *atoms_lapic_state(void) { return &g_lapic; }

bool atoms_lapic_enable_foundation(void) {
  if (!g_lapic.supported || g_lapic.x2apic || !g_lapic.physical_base)
    return false;
  uint64_t base = read_msr(IA32_APIC_BASE_MSR);
  write_msr(IA32_APIC_BASE_MSR, base | APIC_BASE_ENABLE);
  g_lapic.enabled = true;
  uint32_t spurious = lapic_read(APIC_SPURIOUS_REGISTER);
  lapic_write(APIC_SPURIOUS_REGISTER,
              spurious | (1U << 8) | APIC_VECTOR_DIAGNOSTIC);
  return true;
}

void atoms_lapic_eoi(void) {
  if (g_lapic.enabled && !g_lapic.x2apic)
    lapic_write(APIC_EOI_REGISTER, 0U);
}

static uint8_t message_vector(ATOMS_IPIMessage message) {
  switch (message) {
  case ATOMS_IPI_RESCHEDULE:
    return APIC_VECTOR_RESCHEDULE;
  case ATOMS_IPI_TLB_SHOOTDOWN:
    return APIC_VECTOR_TLB;
  case ATOMS_IPI_CALL_FUNCTION:
    return APIC_VECTOR_CALL;
  case ATOMS_IPI_STOP:
    return APIC_VECTOR_STOP;
  case ATOMS_IPI_DIAGNOSTIC:
    return APIC_VECTOR_DIAGNOSTIC;
  default:
    return 0;
  }
}

bool atoms_ipi_send(uint32_t logical_id, ATOMS_IPIMessage message) {
  ATOMS_PerCPU *target = atoms_cpu_by_id(logical_id);
  uint8_t vector = message_vector(message);
  if (!target || message >= ATOMS_IPI_MESSAGE_COUNT || !vector ||
      target->state != ATOMS_CPU_ONLINE || !g_lapic.enabled || g_lapic.x2apic) {
    ++g_lapic.ipi_send_failures;
    return false;
  }
  uint32_t spins = 1000000U;
  while ((lapic_read(APIC_ICR_LOW) & APIC_ICR_PENDING) && --spins)
    __asm__ volatile("pause");
  if (!spins) {
    ++g_lapic.ipi_send_failures;
    return false;
  }
  lapic_write(APIC_ICR_HIGH, target->apic_id << 24);
  lapic_write(APIC_ICR_LOW, vector);
  ++g_cpu_local->ipi_sent[message];
  return true;
}

void atoms_ipi_note_received(ATOMS_IPIMessage message) {
  if (message < ATOMS_IPI_MESSAGE_COUNT)
    ++g_cpu_local->ipi_received[message];
}

bool atoms_smp_self_test(void) {
  if (g_topology.discovered_count == 0 ||
      g_topology.discovered_count > ATOMS_MAX_CPUS ||
      g_topology.configured_count == 0 || g_topology.online_count < 1 ||
      g_cpu_local != &g_percpu[0] || !g_cpu_local->is_bsp ||
      g_cpu_local->state != ATOMS_CPU_ONLINE)
    return false;
  for (uint32_t i = 0; i < g_topology.discovered_count; ++i) {
    for (uint32_t j = i + 1; j < g_topology.discovered_count; ++j) {
      if (g_topology.cpus[i].apic_id == g_topology.cpus[j].apic_id)
        return false;
    }
  }
  return true;
}

void atoms_smp_print_diagnostics(void) {
  display_print("[SMP] discovery=");
  display_print(g_topology.madt_found ? "ACPI-MADT" : "BSP-FALLBACK");
  display_print(" discovered=");
  display_print_dec(g_topology.discovered_count);
  display_print(" configured=");
  display_print_dec(g_topology.configured_count);
  display_print(" online=");
  display_print_dec(g_topology.online_count);
  display_print(" scheduling=");
  display_print_dec(g_topology.scheduling_count);
  display_print("\n");
  for (uint32_t i = 0; i < g_topology.discovered_count; ++i) {
    display_print("[SMP] CPU");
    display_print_dec(i);
    display_print(" apic=");
    display_print_dec(g_topology.cpus[i].apic_id);
    display_print(i == 0 ? " BSP " : " AP ");
    if (g_percpu[i].state == ATOMS_CPU_ONLINE)
      display_print("ONLINE");
    else if (g_percpu[i].state == ATOMS_CPU_CONFIGURED)
      display_print("CONFIGURED-NOT-STARTED");
    else
      display_print("NOT-ONLINE");
    display_print("\n");
  }
  display_print("[SMP-SELFTEST] ");
  display_print(atoms_smp_self_test() ? "PASS" : "FAIL");
  display_print("\n");
  if (g_topology.online_count == g_topology.discovered_count) {
    display_print("ATOMS OS Phase 6 SMP — COMPLETE\n");
  }
}

