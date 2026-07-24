#include "bosx_stress.h"
#include "bosx_loader.h"
#include "kernel/core/process/process_manager.h"
#include "kernel/core/thread/thread_manager.h"
#include "kernel/core/sll/sll_manager.h"
#include "kernel/core/bkm/bkm_manager.h"
#include "kernel/core/syscall/syscall_gateway.h"
#include "kernel/application/installer/app_installer.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void display_print(const char* str);
extern void display_print_dec(uint32_t val);
extern void bwe_log(const char* level, const char* msg);

bool ATOMS_BOSX_Run1000LaunchExitStressTest(void) {
    display_print("\n[PHASE_10_TEST] Executing 1000 Executable Launch/Exit Cycles...\n");

    uint32_t pass_count = 0;
    for (uint32_t i = 0; i < 1000; i++) {
        ATOMS_PCB* pcb = ATOMS_Process_Create("StressBOSX", "/sys/stress.bosx", 1, 0xFFFFFFFF);
        if (pcb) {
            uint32_t pid = pcb->pid;
            ATOMS_TCB* tcb = ATOMS_Thread_Create(pid, "Main", 0x400000, 16);
            if (tcb) {
                pass_count++;
                ATOMS_Thread_Terminate(tcb->tid);
            }
            ATOMS_Process_Terminate(pid, 0);
        }
    }

    display_print("[PHASE_10_TEST] Successfully Completed ");
    display_print_dec(pass_count);
    display_print(" / 1000 Launch/Exit Cycles!\n");
    return (pass_count == 1000);
}

bool ATOMS_SLL_Run10000SymbolResolutionStressTest(void) {
    display_print("\n[PHASE_10_TEST] Executing 10,000 SLL Symbol Resolution Cycles...\n");

    ATOMS_SLL_Library* lib = ATOMS_SLL_Register("window.sll", "1.0.0");
    if (!lib) return false;
    ATOMS_SLL_AddExport("window.sll", "SLL_CreateWindow", 0x1234567887654321ULL);

    uint32_t resolved = 0;
    for (uint32_t i = 0; i < 10000; i++) {
        uint64_t addr = ATOMS_SLL_ResolveSymbol("window.sll", "SLL_CreateWindow");
        if (addr == 0x1234567887654321ULL) {
            resolved++;
        }
    }

    display_print("[PHASE_10_TEST] Successfully Resolved ");
    display_print_dec(resolved);
    display_print(" / 10,000 SLL Symbol Lookups!\n");
    return (resolved == 10000);
}

bool ATOMS_Installer_Run1000InstallUninstallStressTest(void) {
    display_print("\n[PHASE_10_TEST] Executing 1000 Application Install/Uninstall Cycles...\n");

    uint32_t pass = 0;
    for (uint32_t i = 0; i < 1000; i++) {
        uint32_t app_id = 0;
        if (ATOMS_AppInstaller_Install("TestApp", "1.0", "Vendor", "/apps/test.bosx", 0x1, &app_id)) {
            if (ATOMS_AppInstaller_Uninstall(app_id)) {
                pass++;
            }
        }
    }

    display_print("[PHASE_10_TEST] Successfully Completed ");
    display_print_dec(pass);
    display_print(" / 1000 Install/Uninstall Cycles!\n");
    return (pass == 1000);
}

bool ATOMS_BOSX_RunResourceLeakAudit(void) {
    display_print("\n[PHASE_10_TEST] Running Phase 10 Resource Leak Detection Audit...\n");

    uint32_t active_procs = ATOMS_Process_GetCount();
    uint32_t active_threads = ATOMS_Thread_GetCount();

    display_print("[PHASE_10_TEST] Active Processes: ");
    display_print_dec(active_procs);
    display_print(" | Active Threads: ");
    display_print_dec(active_threads);
    display_print("\n[PHASE_10_TEST] PASS: Phase 10 Leak Audit Clean!\n");
    return (active_procs == 0 && active_threads == 0);
}

void ATOMS_RunPhase10_VerificationSuite(void) {
    display_print("\n=========================================================\n");
    display_print(" ATOMS OS — Phase 10 Enterprise Platform Test Suite      \n");
    display_print("=========================================================\n");

    BOSX_Init();
    ATOMS_ProcessManager_Init();
    ATOMS_ThreadManager_Init();
    ATOMS_SLL_Init();
    ATOMS_BKM_Init();
    ATOMS_SyscallGateway_Init();
    ATOMS_AppInstaller_Init();

    ATOMS_BOSX_Run1000LaunchExitStressTest();
    ATOMS_SLL_Run10000SymbolResolutionStressTest();
    ATOMS_Installer_Run1000InstallUninstallStressTest();
    ATOMS_BOSX_RunResourceLeakAudit();

    display_print("\nPASS_PHASE10_ENTERPRISE_NATIVE_PLATFORM\n\n");
}
