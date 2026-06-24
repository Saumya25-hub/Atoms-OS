#include "../include/atoms.h"
#include "../../libbos/include/bos.h"

static void print_ok(const char* test_name) {
    bos_print("[TEST] ");
    bos_print(test_name);
    bos_print(" : PASS\n");
}

static void print_fail(const char* test_name) {
    bos_print("[TEST] ");
    bos_print(test_name);
    bos_print(" : FAIL\n");
}

void atom_self_test(void) {
    bos_print("\n=== ATOMS LIBRARY V1 SELF-TEST ===\n");
    
    // Test 1: Primitives
    AtomValue v_nil = atom_value_nil();
    if (atom_is_nil(v_nil)) print_ok("Primitive NIL"); else print_fail("Primitive NIL");
    
    AtomValue v_true = atom_value_bool(true);
    if (atom_is_bool(v_true) && v_true.as.boolean == true) print_ok("Primitive BOOL"); else print_fail("Primitive BOOL");
    
    AtomValue v_num = atom_value_number(42.5);
    if (atom_is_number(v_num) && v_num.as.number == 42.5) print_ok("Primitive NUMBER"); else print_fail("Primitive NUMBER");
    
    // Test 2: Strings
    AtomValue s1 = atom_string_create("Hello BOSL");
    AtomValue s2 = atom_string_create("Hello BOSL");
    AtomValue s3 = atom_string_create("Different");
    
    if (atom_is_string(s1) && atom_is_string(s2) && atom_is_string(s3)) {
        if (s1.as.string == s2.as.string) print_ok("String Interning"); else print_fail("String Interning");
        if (s1.as.string != s3.as.string) print_ok("String Distinct"); else print_fail("String Distinct");
        
        if (atom_values_equal(s1, s2)) print_ok("String Value Equality"); else print_fail("String Value Equality");
        if (!atom_values_equal(s1, s3)) print_ok("String Value Inequality"); else print_fail("String Value Inequality");
        
        if (s1.as.string->ref_count == 2) print_ok("String Ref Count"); else print_fail("String Ref Count");
    } else {
        print_fail("String Creation");
    }
    
    // Clean up
    atom_string_release(s1.as.string);
    atom_string_release(s2.as.string);
    atom_string_release(s3.as.string);
    
    bos_print("=== ATOMS SELF-TEST COMPLETE ===\n\n");
}
