#include "kernel/scheduler/include/task.h"
#include "kernel/scheduler/include/scheduler.h"
#include "kernel/display/display.h"

// Task management utility functions (to be expanded)
static uint64_t next_task_id = 1;

uint64_t task_generate_id(void) {
    return next_task_id++;
}

bool task_transition(Task* task, TaskState requested_state) {
    if (!task) return false;

    // Rule 52: Singular State Integrity
    if (task->state == requested_state) {
        return false;
    }

    // Rule 53: Immortal Idle Task
    if (task == scheduler_get_idle_task()) {
        if (requested_state == TASK_BLOCKED || requested_state == TASK_SLEEPING || requested_state == TASK_TERMINATED) {
            display_print("\n[TASK] PANIC: Attempted to block/kill Idle Task!\n");
            __asm__ volatile("cli");
            while (1) { __asm__ volatile("hlt"); }
        }
    }

    // Validation Pipeline (Sprint 2)
    bool valid = false;

    if (task->state == TASK_READY && requested_state == TASK_RUNNING) {
        valid = true;
    } else if (task->state == TASK_RUNNING && requested_state == TASK_READY) {
        valid = true;
    }

    if (valid) {
        task->state = requested_state;
        
        static uint8_t proof_count = 0;
        if (proof_count < 2) {
            display_print("\n[TASK TRANSITION]\n");
            display_print(task->name);
            display_print("\n");
            if (requested_state == TASK_RUNNING) {
                display_print("READY -> RUNNING\n");
            } else {
                display_print("RUNNING -> READY\n");
            }
            display_print("PASS\n");
            proof_count++;
        }

        return true;
    }

    display_print("\n[TASK] PANIC: Invalid State Transition!\n");
    __asm__ volatile("cli");
    while (1) { __asm__ volatile("hlt"); }
    
    return false;
}

TaskState task_get_state(const Task* task) {
    if (task) {
        return task->state;
    }
    return TASK_READY;
}
