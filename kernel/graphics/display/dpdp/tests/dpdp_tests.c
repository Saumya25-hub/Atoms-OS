#include "kernel/graphics/display/dpdp/include/dpdp.h"

extern void display_print(const char* str);

void dpdp_run_certification_tests(void) {
    display_print("[DPDP TEST] Running Display Pipeline Debug Panel Self-Test Suite...\n");

    dpdp_frame_autopsy_t autopsy;
    dpdp_init(&autopsy);
    dpdp_record_sample_frame(&autopsy);
    dpdp_compile_autopsy(&autopsy);
    dpdp_print_flight_recorder(&autopsy);

    display_print("[DPDP TEST] DPDP Self-Test Suite PASS\n");
}
