#include "../test_framework.h"
#include "../../atoms/include/atoms.h"
#include "../../atoms/include/atom_compiler.h"

// Define a BOSL script with loops, arrays, objects, and math.
static const char* bosl_stress_script =
    "sum = 0\n"
    "for (i = 0; i < 1000; i = i + 1) {\n"
    "    sum = sum + i\n"
    "}\n"
    "arr = [10, 20, 30]\n"
    "obj = { x: 5, y: 10 }\n"
    "arr.push(sum)\n";

void test_bosl_suite(void) {
    TEST_SUITE_START("BOSL Engine Stress");

    bool success = true;
    
    // We run the VM compilation and execution 10 times to ensure
    // memory doesn't leak massively and crash the VM.
    for (int i = 0; i < 10; i++) {
        atoms_init();
        
        AtomFunction* main_fn = atom_function_create(atom_string_create("main").as.string, 0);
        if (atom_compiler_compile(bosl_stress_script, main_fn->chunk)) {
            AtomVM vm;
            atom_vm_init(&vm);
            
            if (!atom_vm_execute(&vm, main_fn)) {
                success = false;
            }
        } else {
            success = false;
        }
    }
    
    ASSERT(success, "BOSL Complex Script Loop (10 iterations)");
    
    // Let's also test syntax errors don't crash
    atoms_init();
    AtomFunction* bad_func = atom_function_create(atom_string_create("main").as.string, 0);
    bool compiled = atom_compiler_compile("for (i = 0 i < 10) { }", bad_func->chunk); // Missing semi-colons
    ASSERT(compiled == false, "BOSL Syntax Error Handling");
}
