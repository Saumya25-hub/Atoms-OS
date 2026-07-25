/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * sandbox_tests.c — Certification Test Suite Implementation
 */

#include "kernel/sandbox/tests/sandbox_tests.h"
#include "kernel/sandbox/include/bos_sandbox.h"
#include "kernel/sandbox/core/sandbox_manager.h"
#include "kernel/sandbox/process/sandbox_process.h"
#include "kernel/sandbox/token/sandbox_token.h"
#include "kernel/sandbox/capability/sandbox_capability.h"
#include "kernel/sandbox/permissions/sandbox_permissions.h"
#include "kernel/sandbox/memory/sandbox_memory.h"
#include "kernel/sandbox/syscall/sandbox_syscall.h"
#include "kernel/sandbox/ipc/sandbox_ipc.h"
#include "kernel/sandbox/filesystem/sandbox_filesystem.h"
#include "kernel/sandbox/network/sandbox_network.h"
#include "kernel/sandbox/resource/sandbox_resource.h"
#include "kernel/sandbox/audit/sandbox_audit.h"

extern void display_print(const char* str);

bos_sandbox_status_t bos_sandbox_run_certification_tests(void) {
    return sandbox_run_certification_tests();
}

bos_sandbox_status_t sandbox_run_certification_tests(void) {
    display_print("[SANDBOX TESTS]\n");

    bool all_passed = true;

    /* 1. Process Isolation Test */
    uint32_t ctx_id1 = 0, ctx_id2 = 0;
    bos_sandbox_status_t s1 = bos_process_isolate(101, 0x100000, 0x400000, 0x1000000, &ctx_id1);
    bos_sandbox_status_t s2 = bos_process_isolate(102, 0x200000, 0x2000000, 0x1000000, &ctx_id2);
    if (s1 == BOS_SANDBOX_OK && s2 == BOS_SANDBOX_OK && ctx_id1 != ctx_id2 && bos_sandbox_destroy(ctx_id2) == BOS_SANDBOX_OK) {
        display_print("Process Isolation.......PASS\n");
    } else {
        display_print("Process Isolation.......FAIL\n");
        all_passed = false;
    }

    /* 2. Memory Isolation Test */
    bool mem_ok = true;
    /* Cross Process Block */
    if (sandbox_memory_validate_cross_process(ctx_id1, 9999, 0x401000, 64) == BOS_SANDBOX_OK) mem_ok = false;
    /* Kernel Memory Protection */
    if (sandbox_memory_protect_kernel(0xFFFF800000001000ULL, 128) == BOS_SANDBOX_OK) mem_ok = false;
    /* Guard Page Check */
    if (sandbox_memory_guard_page_check(ctx_id1, 0x400000) == BOS_SANDBOX_OK) mem_ok = false;
    
    if (mem_ok) {
        display_print("Memory Isolation.......PASS\n");
    } else {
        display_print("Memory Isolation.......FAIL\n");
        all_passed = false;
    }

    /* 3. Capabilities Test */
    bool cap_ok = true;
    bos_capability_grant(ctx_id1, BOS_CAP_CLIPBOARD | BOS_CAP_CAMERA);
    if (!sandbox_capability_has(ctx_id1, BOS_CAP_CLIPBOARD)) cap_ok = false;
    bos_capability_revoke(ctx_id1, BOS_CAP_CLIPBOARD);
    if (sandbox_capability_has(ctx_id1, BOS_CAP_CLIPBOARD)) cap_ok = false;

    if (cap_ok) {
        display_print("Capabilities.......PASS\n");
    } else {
        display_print("Capabilities.......FAIL\n");
        all_passed = false;
    }

    /* 4. Permissions Test */
    bool perm_ok = true;
    /* Camera capability is granted */
    if (bos_permission_check(ctx_id1, BOS_PERM_OPEN_CAMERA, NULL) != BOS_SANDBOX_OK) perm_ok = false;
    /* Microphone capability is not granted */
    if (bos_permission_check(ctx_id1, BOS_PERM_ACCESS_MICROPHONE, NULL) == BOS_SANDBOX_OK) perm_ok = false;

    if (perm_ok) {
        display_print("Permissions.......PASS\n");
    } else {
        display_print("Permissions.......FAIL\n");
        all_passed = false;
    }

    /* 5. IPC Security Test */
    bool ipc_ok = true;
    uint32_t ctx_id3 = 0;
    bos_process_isolate(103, 0x300000, 0x3000000, 0x1000000, &ctx_id3);
    /* Valid IPC between isolated process 101 and 103 */
    if (sandbox_ipc_validate_request(101, 103, 1, 1024) != BOS_SANDBOX_OK) ipc_ok = false;
    /* Invalid IPC with un-isolated PID 999 */
    if (sandbox_ipc_validate_request(101, 999, 1, 1024) == BOS_SANDBOX_OK) ipc_ok = false;

    if (ipc_ok) {
        display_print("IPC Security.......PASS\n");
    } else {
        display_print("IPC Security.......FAIL\n");
        all_passed = false;
    }

    /* 6. Filesystem Test */
    bool fs_ok = true;
    bos_capability_grant(ctx_id1, BOS_CAP_READ_FILES);
    if (sandbox_fs_validate_path(ctx_id1, "/tmp/app.dat", 1) != BOS_SANDBOX_OK) fs_ok = false;
    if (sandbox_fs_validate_path(ctx_id1, "/kernel/secret.sys", 1) == BOS_SANDBOX_OK) fs_ok = false;

    if (fs_ok) {
        display_print("Filesystem.......PASS\n");
    } else {
        display_print("Filesystem.......FAIL\n");
        all_passed = false;
    }

    /* 7. Network Test */
    bool net_ok = true;
    bos_capability_grant(ctx_id1, BOS_CAP_NETWORK);
    if (sandbox_net_validate_connection(ctx_id1, 6 /* TCP */, "127.0.0.1", 443) != BOS_SANDBOX_OK) net_ok = false;
    if (sandbox_net_validate_connection(ctx_id1, 255 /* RAW */, "127.0.0.1", 80) == BOS_SANDBOX_OK) net_ok = false;

    if (net_ok) {
        display_print("Network.......PASS\n");
    } else {
        display_print("Network.......FAIL\n");
        all_passed = false;
    }

    /* 8. Crash Isolation Test */
    bool crash_ok = true;
    uint32_t crash_pid = 555;
    uint32_t crash_ctx = 0;
    bos_process_isolate(crash_pid, 0x400000, 0x4000000, 0x1000000, &crash_ctx);
    if (sandbox_process_handle_crash(crash_pid) != BOS_SANDBOX_OK) crash_ok = false;
    if (sandbox_process_is_isolated(crash_pid)) crash_ok = false;

    if (crash_ok) {
        display_print("Crash Isolation.......PASS\n");
    } else {
        display_print("Crash Isolation.......FAIL\n");
        all_passed = false;
    }

    /* 9. Stress Test */
    bool stress_ok = true;
    for (int i = 0; i < 10; i++) {
        uint32_t temp_pid = 200 + i;
        uint32_t temp_ctx = 0;
        if (bos_process_isolate(temp_pid, 0x500000, 0x5000000, 0x1000000, &temp_ctx) != BOS_SANDBOX_OK) {
            stress_ok = false;
            break;
        }
        sandbox_resource_track_alloc(temp_ctx, BOS_RES_RAM, 1024 * 1024);
        sandbox_resource_track_free(temp_ctx, BOS_RES_RAM, 1024 * 1024);
        bos_sandbox_destroy(temp_ctx);
    }

    if (stress_ok) {
        display_print("Stress Test.......PASS\n");
    } else {
        display_print("Stress Test.......FAIL\n");
        all_passed = false;
    }

    /* 10. Security Audit Test */
    bool audit_ok = true;
    if (sandbox_audit_get_count() == 0) audit_ok = false;

    if (audit_ok) {
        display_print("Security Audit.......PASS\n");
    } else {
        display_print("Security Audit.......FAIL\n");
        all_passed = false;
    }

    display_print("==============================================\n");
    if (all_passed) {
        display_print("SUCCESS\n");
        display_print("All Sandbox Certification Tests Passed\n");
    } else {
        display_print("FAILURE\n");
        display_print("Sandbox Certification Tests Failed\n");
    }
    display_print("==============================================\n\n");

    return all_passed ? BOS_SANDBOX_OK : BOS_SANDBOX_ERR_PERMISSION_DENIED;
}
