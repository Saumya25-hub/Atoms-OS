#ifndef BOS_TASKMGR_TYPES_H
#define BOS_TASKMGR_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define TASKMGR_MAX_PROCESSES   512
#define TASKMGR_MAX_THREADS     2048
#define TASKMGR_MAX_DRIVERS     256
#define TASKMGR_MAX_MODULES     512
#define TASKMGR_MAX_HANDLES     4096
#define TASKMGR_MAX_SERVICES    256
#define TASKMGR_MAX_SENSORS     64
#define TASKMGR_GRAPH_SAMPLES   256

typedef enum {
    TASKMGR_OK          = 0,
    TASKMGR_ERR         = -1,
    TASKMGR_ERR_ARG     = -2,
    TASKMGR_ERR_MEM     = -3,
    TASKMGR_ERR_ACCESS  = -4,
    TASKMGR_NOT_FOUND   = -5,
} TASKMGR_RESULT;

// ---- Process Entry ----
typedef struct _TASKMGR_PROCESS {
    uint32_t pid;
    char     name[64];
    char     path[256];
    uint32_t thread_count;
    uint32_t handle_count;
    uint64_t memory_bytes;
    uint32_t cpu_percent;      // 0-100
    uint32_t priority;
    bool     is_suspended;
    uint32_t session_id;
} TASKMGR_PROCESS;

// ---- Thread Entry ----
typedef struct _TASKMGR_THREAD {
    uint32_t tid;
    uint32_t pid;
    uint32_t state;         // 0=running 1=ready 2=waiting 3=suspended
    uint32_t priority;
    uint64_t cpu_cycles;
    uint32_t stack_size;
} TASKMGR_THREAD;

// ---- Memory Stats ----
typedef struct _TASKMGR_MEMORY {
    uint64_t total_bytes;
    uint64_t used_bytes;
    uint64_t available_bytes;
    uint64_t kernel_heap_bytes;
    uint64_t user_heap_bytes;
    uint64_t page_fault_count;
    uint32_t paged_pool_kb;
    uint32_t non_paged_pool_kb;
    uint32_t virtual_size_mb;
    uint32_t commit_limit_mb;
    char     ddr_type[16];
    uint32_t speed_mhz;
    uint32_t channels;
} TASKMGR_MEMORY;

// ---- CPU Stats ----
typedef struct _TASKMGR_CPU {
    uint32_t logical_cores;
    uint32_t physical_cores;
    uint32_t usage_percent;    // overall
    uint32_t core_usage[64];   // per-core
    uint64_t base_freq_hz;
    uint64_t current_freq_hz;
    uint64_t turbo_freq_hz;
    char     vendor[32];
    char     brand[64];
    uint32_t temperature_c;
    uint64_t context_switches;
    uint64_t interrupts_per_sec;
    uint64_t syscalls_per_sec;
    uint64_t dpc_count;
    uint32_t l1_kb;
    uint32_t l2_kb;
    uint32_t l3_mb;
    bool     has_sse;
    bool     has_avx;
    bool     has_avx2;
    bool     has_aes;
    bool     has_vmx;
} TASKMGR_CPU;

// ---- GPU Stats ----
typedef struct _TASKMGR_GPU {
    char     vendor[32];
    char     model[64];
    char     driver_version[32];
    char     opengl_version[16];
    char     renderer[128];
    uint64_t vram_bytes;
    uint32_t clock_mhz;
    uint32_t mem_clock_mhz;
    uint32_t temperature_c;
    uint32_t fan_rpm;
    uint32_t utilization_percent;
    uint32_t pci_bus;
    uint32_t pci_device;
    uint32_t cmd_queue_depth;
} TASKMGR_GPU;

// ---- Storage Entry ----
typedef struct _TASKMGR_STORAGE {
    char     label[32];
    char     filesystem[16];
    uint64_t capacity_bytes;
    uint64_t free_bytes;
    uint32_t read_mbps;
    uint32_t write_mbps;
    uint32_t temperature_c;
    bool     smart_ok;
} TASKMGR_STORAGE;

// ---- Network Stats ----
typedef struct _TASKMGR_NETWORK {
    char     adapter[64];
    char     mac[24];
    char     ipv4[16];
    char     ipv6[40];
    char     gateway[16];
    char     dns[16];
    uint64_t upload_bps;
    uint64_t download_bps;
    uint64_t packets_sent;
    uint64_t packets_recv;
    uint64_t errors;
    uint32_t socket_count;
    uint32_t rtt_ms;
    uint32_t packet_loss_pct;
} TASKMGR_NETWORK;

// ---- Power Stats ----
typedef struct _TASKMGR_POWER {
    bool     ac_connected;
    uint32_t battery_percent;
    uint32_t charge_cycles;
    uint32_t voltage_mv;
    uint32_t current_ma;
    uint32_t power_mw;
    uint32_t estimated_hours;
} TASKMGR_POWER;

// ---- Service Entry ----
typedef struct _TASKMGR_SERVICE {
    char     name[64];
    char     display_name[128];
    bool     running;
    bool     auto_start;
    uint32_t pid;
} TASKMGR_SERVICE;

// ---- Driver Entry ----
typedef struct _TASKMGR_DRIVER {
    char     name[64];
    char     path[256];
    uint32_t load_order;
    bool     loaded;
} TASKMGR_DRIVER;

// ---- Sensor Entry ----
typedef struct _TASKMGR_SENSOR {
    char     name[64];
    int32_t  value;
    char     unit[8];
    int32_t  min_value;
    int32_t  max_value;
} TASKMGR_SENSOR;

// ---- Kernel Diagnostics ----
typedef struct _TASKMGR_KERNEL_DIAG {
    uint32_t running_threads;
    uint64_t kernel_heap_used;
    uint64_t user_heap_used;
    uint64_t page_faults;
    uint64_t interrupts;
    uint64_t exceptions;
    uint32_t scheduler_load_pct;
    uint32_t handle_count;
    uint32_t open_files;
    uint32_t ipc_objects;
    uint32_t mutexes;
    uint32_t semaphores;
    uint32_t events;
    uint32_t shared_mem_objects;
    uint32_t timers;
    // Forensic Mode extras
    uint64_t syscalls_per_sec;
    uint64_t context_switches_per_sec;
    uint64_t interrupts_per_sec;
    uint64_t dpc_count;
    uint64_t file_io_latency_us;
    uint64_t net_rtt_ms;
} TASKMGR_KERNEL_DIAG;

// ---- Performance Graph Sample ----
typedef struct _TASKMGR_GRAPH {
    uint32_t cpu_samples[TASKMGR_GRAPH_SAMPLES];
    uint32_t ram_samples[TASKMGR_GRAPH_SAMPLES];
    uint32_t gpu_samples[TASKMGR_GRAPH_SAMPLES];
    uint32_t disk_samples[TASKMGR_GRAPH_SAMPLES];
    uint32_t net_samples[TASKMGR_GRAPH_SAMPLES];
    uint32_t temp_samples[TASKMGR_GRAPH_SAMPLES];
    uint32_t fps_samples[TASKMGR_GRAPH_SAMPLES];
    uint32_t sample_head;
} TASKMGR_GRAPH;

#endif // BOS_TASKMGR_TYPES_H
