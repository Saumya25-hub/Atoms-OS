#include "kernel/audio/core/audio_realtime_worker.h"
#include "kernel/audio/hal/audio_hal.h"
#include <stddef.h>

extern uint64_t step14_rdtsc(void);
extern uint64_t step14_cycles_to_us(uint64_t cycles);

static AudioRealtimeTelemetry g_rt_telemetry = {0};

void audio_realtime_worker_init(void) {
    g_rt_telemetry.total_pumps = 0;
    g_rt_telemetry.refill_cycles = 0;
    g_rt_telemetry.max_pump_duration_us = 0;
    g_rt_telemetry.timing_violations = 0;
    g_rt_telemetry.last_pump_duration_us = 0;
    g_rt_telemetry.is_active = false;
}

void audio_realtime_worker_start(void) {
    g_rt_telemetry.is_active = true;
}

void audio_realtime_worker_stop(void) {
    g_rt_telemetry.is_active = false;
}

/**
 * @brief Highest-Priority Realtime Audio Worker Execution Hook.
 *
 * Called directly from the hardware timer tick (IRQ 0 @ 1000 Hz) or scheduler high-priority hook.
 * Strictly forbidden from calling filesystem, VFS, malloc/free, printf, or blocking primitives.
 */
static uint64_t g_last_irq_enter_cycles = 0;

void audio_realtime_worker_pump(void) {
    if (!g_rt_telemetry.is_active) {
        return;
    }

    uint64_t start_cycles = step14_rdtsc();
    uint64_t irq_entry_delta_us = (g_last_irq_enter_cycles == 0) ? 1000 : step14_cycles_to_us(start_cycles - g_last_irq_enter_cycles);
    g_last_irq_enter_cycles = start_cycles;

    extern uint64_t g_tl_port_io_us, g_tl_desc_loop_us, g_tl_mixer_us, g_tl_lvi_us;
    extern uint64_t g_tl_stream_read_us;
    extern uint32_t g_tl_desc_count;
    g_tl_port_io_us = 0;
    g_tl_desc_loop_us = 0;
    g_tl_mixer_us = 0;
    g_tl_lvi_us = 0;
    g_tl_stream_read_us = 0;
    g_tl_desc_count = 0;

    // Execute the deterministic HAL hardware pointer update and mixer refill loop
    audio_hal_update_pointers(0);

    uint64_t end_cycles = step14_rdtsc();
    uint32_t dur_us = (uint32_t)step14_cycles_to_us(end_cycles - start_cycles);

    g_rt_telemetry.total_pumps++;
    g_rt_telemetry.last_pump_duration_us = dur_us;

    if (dur_us > g_rt_telemetry.max_pump_duration_us) {
        g_rt_telemetry.max_pump_duration_us = dur_us;
    }

    // If the pump itself took > 2000 microseconds (2 ms), mark it as a timing violation
    if (dur_us > 2000) {
        g_rt_telemetry.timing_violations++;
    }
}

void audio_realtime_worker_get_telemetry(AudioRealtimeTelemetry* out_stats) {
    if (out_stats) {
        *out_stats = g_rt_telemetry;
    }
}
