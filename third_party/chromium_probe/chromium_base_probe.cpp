/*
 * Chromium Base Primitives Probe Implementation
 * Adapted from Chromium base/ (BSD 3-Clause)
 */

#include "chromium_base_probe.h"
#include "userspace/runtime/c/include/stdio.h"

namespace base {

bool RunChromiumBaseProbe(void) {
    puts("[CHROMIUM_PROBE] Level 2: Testing base::TimeTicks...");
    TimeTicks t1 = TimeTicks::Now();
    if (t1.is_null()) {
        puts("[CHROMIUM_PROBE] TimeTicks::Now() returned null!");
        return false;
    }

    puts("[CHROMIUM_PROBE] Level 2: Testing base::AtomicRefCount...");
    AtomicRefCount ref(1);
    ref.Increment();
    if (ref.IsOne()) return false;
    ref.Decrement();
    if (!ref.IsOne()) return false;
    ref.Decrement();
    if (!ref.IsZero()) return false;

    puts("[CHROMIUM_PROBE] Level 2: Testing base::span...");
    int raw_array[4] = {10, 20, 30, 40};
    span<int> s(raw_array, 4);
    if (s.size() != 4 || s[2] != 30) return false;

    puts("[CHROMIUM_PROBE] Level 2 (Chromium base subset): PASS");
    return true;
}

} // namespace base
