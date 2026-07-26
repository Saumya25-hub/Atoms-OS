#ifndef TMH_PROVIDER_H
#define TMH_PROVIDER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TMH_MAX_CORES 16
#define TMH_MAX_THREAD_SAMPLES 16

typedef struct {
    uint32_t core_id;
    uint32_t apic_id;
    uint32_t usage_percent;
    uint32_t frequency_mhz;
    uint32_t active_threads;
    uint32_t idle_percent;
    bool online;
} TMH_CoreStats;

typedef struct {
    uint32_t thread_id;
    uint32_t owner_pid;
    uint32_t core_id;
    char name[32];
} TMH_ThreadMapping;

typedef struct {
    // CPU Global Metrics
    char cpu_model[64];
    uint32_t total_cpu_usage;
    uint32_t current_freq_mhz;
    uint32_t base_clock_mhz;
    uint32_t physical_cores;
    uint32_t logical_threads;
    uint32_t online_cores;
    const char* scheduler_state;
    
    TMH_CoreStats cores[TMH_MAX_CORES];
    
    // Live Thread Mapping & Load Balance
    TMH_ThreadMapping active_threads[TMH_MAX_THREAD_SAMPLES];
    uint32_t active_thread_sample_count;
    uint32_t threads_per_core[TMH_MAX_CORES];
    
    // RAM / Memory Metrics
    uint64_t total_ram_bytes;
    uint64_t used_ram_bytes;
    uint64_t free_ram_bytes;
    uint64_t kernel_ram_bytes;
    uint64_t user_ram_bytes;
    
    // Storage Metrics
    uint64_t total_storage_bytes;
    uint64_t used_storage_bytes;
    uint64_t free_storage_bytes;
    const char* filesystem_type;
    uint32_t read_speed_mbps;
    uint32_t write_speed_mbps;
    
    // Ethernet Network Metrics
    const char* net_status;
    char net_ip[32];
    uint64_t net_rx_bytes;
    uint64_t net_tx_bytes;
    const char* net_speed;
    
    // Graphics VBE Metrics
    uint32_t screen_width;
    uint32_t screen_height;
    uint32_t bpp;
    uint32_t refresh_rate_hz;
    uint64_t framebuffer_phys;
    
    // Overall Counts & Uptime
    uint32_t total_processes;
    uint32_t total_threads;
    uint32_t uptime_seconds;
    char uptime_str[32];
} TMH_TelemetryData;

void TMH_GatherTelemetry(TMH_TelemetryData* data);

#ifdef __cplusplus
}
#endif

#endif // TMH_PROVIDER_H
