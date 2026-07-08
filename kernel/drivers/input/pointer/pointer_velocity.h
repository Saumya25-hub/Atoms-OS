#ifndef KERNEL_POINTER_VELOCITY_H
#define KERNEL_POINTER_VELOCITY_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// ATOMS OS Input Engine V2 - Phase 3: Configurable Velocity & Acceleration
// ============================================================================

typedef struct {
    int32_t base_sensitivity_fp16; // Base linear sensitivity (default: 1.0x = 65536)
    int32_t velocity_threshold;    // Speed in px/sec where acceleration begins (default: 50)
    int32_t acceleration_gain_fp16;// Gain multiplier rate (default: 0.5x = 32768)
    int32_t max_sensitivity_fp16;  // Ceiling clamp multiplier (default: 3.5x = 229376)
} PointerVelocityProfile;

// Initialize velocity tracker and set default profile
void pointer_velocity_init(void);

// Configure runtime ballistic acceleration parameters
void pointer_velocity_set_profile(const PointerVelocityProfile* profile);

// Calculate instantaneous velocity (px/sec) and return 16.16 fixed-point ballistic multiplier
int32_t pointer_velocity_calculate(int32_t dx, int32_t dy, uint64_t timestamp_us, int32_t* out_velocity_raw);

#endif // KERNEL_POINTER_VELOCITY_H
