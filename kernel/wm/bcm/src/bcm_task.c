#include "../include/bcm.h"
#include "../include/bcm_internal.h"
#include "kernel/core/scheduler/include/scheduler.h"

/* Serial diagnostic functions */
extern void com1_puts(const char* s);
extern void serial_write_direct(const char* str);
extern void serial_write_dec_direct(int val);

/* Helper to check CPU Interrupt Flag (IF) */
static inline bool bcm_task_is_interrupt_enabled(void) {
    uint64_t rflags;
    __asm__ volatile("pushfq; popq %0" : "=r"(rflags));
    return (rflags & (1ULL << 9)) != 0;
}

/* Worker concurrency control */
static volatile bool s_bcm_worker_active = false;
static Task* s_bcm_task_handle = NULL;

/* ========================================================================= */
/* BCM Dedicated Compositor Worker Thread (Phase 4 Paced Loop)               */
/* ========================================================================= */

void bcm_compositor_thread(void) {
    com1_puts("[BCM] COMPOSITOR TASK CONTEXT: Entered bcm_compositor_thread\r\n");

    /* Verify Interrupt Context */
    if (!bcm_task_is_interrupt_enabled()) {
        com1_puts("[BCM_FATAL] bcm_compositor_thread running with IF=0! Interrupts disabled!\r\n");
        return;
    }

    com1_puts("[BCM] IF=1 VERIFIED: Dedicated task running in preemptible kernel context\r\n");
    com1_puts("[BCM] WORKER READY: Entering authoritative frame management loop\r\n");

    for (;;) {
        /* Execution Context Firewall Check */
        if (!bcm_task_is_interrupt_enabled()) {
            com1_puts("[BCM_WARN] bcm_compositor_thread detected IF=0 inside loop! Re-enabling interrupts.\r\n");
            __asm__ volatile("sti");
        }

        /* 0. Pump wallpaper service background timer & auto-rotation */
        extern void wallpaper_service_update(uint64_t delta_ms);
        wallpaper_service_update(5);

        /* 1. Check if damage exists */
        if (!BCM_HasPendingDamage()) {
            /* No visual damage: Sleep 5 ms to yield CPU and avoid busy spinning */
            scheduler_sleep(5);
            continue;
        }

        /* 2. Check if frame pacing deadline is reached (60 FPS nominal / ~16.6 ms) */
        if (!BCM_FrameDeadlineReached()) {
            /* Damage pending but pacing window not yet open: Coalesce and sleep briefly */
            g_bcm_state.telemetry.frames_coalesced++;
            scheduler_sleep(2);
            continue;
        }

        /* 3. Process frame pass in preemptible task context */
        if (!s_bcm_worker_active) {
            s_bcm_worker_active = true;

            bcm_error_t err = BCM_Process();
            if (err != BCM_OK && err != BCM_ERR_BUSY) {
                com1_puts("[BCM_WARN] BCM_Process returned error code\r\n");
            }

            s_bcm_worker_active = false;
        }

        /* 4. Cooperative yield to give userspace applications and drivers CPU time */
        scheduler_yield();
    }
}

/* ========================================================================= */
/* BCM Worker Startup API                                                    */
/* ========================================================================= */

bcm_error_t BCM_StartCompositorTask(void) {
    com1_puts("[BCM] Starting BCM Dedicated Compositor Task...\r\n");

    s_bcm_task_handle = scheduler_create_kernel_task("bcm_compositor", bcm_compositor_thread, 31);
    if (!s_bcm_task_handle) {
        com1_puts("[BCM_ERR] Failed to spawn bcm_compositor kernel task!\r\n");
        return BCM_ERR_INVALID_STATE;
    }

    com1_puts("[BCM] WORKER CREATED: bcm_compositor task registered (Priority 31 - Realtime Compositor)\r\n");
    return BCM_OK;
}
