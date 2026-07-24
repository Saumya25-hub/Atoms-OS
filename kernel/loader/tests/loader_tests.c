#include "loader_tests.h"
#include "../include/loader_api.h"
#include "../elf/elf_parser.h"
#include "../symbols/symbol_resolver.h"
#include "../reloc/reloc_engine.h"
#include "../library/library_manager.h"
#include "../debug/loader_debug.h"

extern void display_print(const char* s);

bool loader_run_unit_tests(void) {
    display_print("\n=========================================================\n");
    display_print(" [LOADER_TESTS] Executing Dynamic Loader Unit Test Suite\n");
    display_print("=========================================================\n");

    // Test 1: Verify Invalid ELF Header Rejection
    display_print("[TEST 1] Testing Invalid ELF Header Verification... ");
    uint8_t bad_hdr[64] = {0x00, 'N', 'O', 'T', 2, 1, 1, 0};
    loader_status_t status = elf_parser_verify(bad_hdr, sizeof(bad_hdr));
    if (status == LOADER_ERR_INVALID_MAGIC) {
        display_print("PASSED [Corrupted Magic Rejected Cleanly]\n");
    } else {
        display_print("FAILED!\n");
        return false;
    }

    // Test 2: Symbol Registration & Lookup
    display_print("[TEST 2] Testing Symbol Resolver Engine... ");
    static int test_symbol_var = 42;
    symbol_resolver_register("test_symbol_var", &test_symbol_var, SYM_BIND_GLOBAL, SYM_TYPE_OBJECT);
    void* resolved = symbol_resolver_lookup("test_symbol_var");
    if (resolved == &test_symbol_var) {
        display_print("PASSED [Symbol Resolved Correctly]\n");
    } else {
        display_print("FAILED!\n");
        return false;
    }

    // Test 3: Table-Driven Relocation Processor
    display_print("[TEST 3] Testing Relocation Engine Table-Driven Handler... ");
    uint64_t target_slot = 0;
    status = reloc_engine_process_entry(R_X86_64_GLOB_DAT, (uint64_t)(uintptr_t)&target_slot, 0x1000, 0x20, 0x0);
    if (status == LOADER_SUCCESS && target_slot == 0x1020) {
        display_print("PASSED [GLOB_DAT Relocation Handled Correctly]\n");
    } else {
        display_print("FAILED!\n");
        return false;
    }

    // Test 4: Dynamic Linker dlopen & dlsym APIs
    display_print("[TEST 4] Testing Runtime Linker (bos_dlopen & bos_dlsym)... ");
    library_handle_t handle = bos_dlopen("libm.so", RTLD_NOW);
    if (handle) {
        void* sym_ptr = bos_dlsym(handle, "test_symbol_var");
        if (sym_ptr == &test_symbol_var) {
            bos_dlclose(handle);
            display_print("PASSED [bos_dlopen & bos_dlsym Functional]\n");
        } else {
            display_print("FAILED (dlsym lookup failed)!\n");
            return false;
        }
    } else {
        display_print("FAILED (dlopen failed)!\n");
        return false;
    }

    display_print("=========================================================\n");
    display_print(" [LOADER_TESTS] SUCCESS: All Dynamic Loader Tests Passed!\n");
    display_print("=========================================================\n\n");
    return true;
}
