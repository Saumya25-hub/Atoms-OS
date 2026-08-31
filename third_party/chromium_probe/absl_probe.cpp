/*
 * Abseil Primitives Probe Implementation
 * Adapted from Google Abseil C++ (Apache 2.0)
 */

#include "absl_probe.h"
#include "userspace/runtime/c/include/stdio.h"

namespace absl {

bool RunAbseilProbe(void) {
    puts("[CHROMIUM_PROBE] Level 3: Testing absl::string_view...");
    string_view sv = "Chromium_GN_Ninja_Toolchain";
    if (sv.size() != 27 || sv[0] != 'C' || sv[8] != '_' || sv != "Chromium_GN_Ninja_Toolchain") {
        puts("[CHROMIUM_PROBE] absl::string_view check failed!");
        return false;
    }

    puts("[CHROMIUM_PROBE] Level 3: Testing absl::Status...");
    Status ok = Status::OkStatus();
    if (!ok.ok()) return false;

    Status err(StatusCode::kNotFound, "Resource missing in ATOMS VFS");
    if (err.ok() || err.code() != StatusCode::kNotFound) return false;

    puts("[CHROMIUM_PROBE] Level 3 (Abseil foundational subset): PASS");
    return true;
}

} // namespace absl
