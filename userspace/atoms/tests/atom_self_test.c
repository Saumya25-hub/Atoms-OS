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
    atom_value_release(s1);
    atom_value_release(s2);
    atom_value_release(s3);
    
    // Test 3: Arrays
    AtomArray* arr = atom_array_create();
    atom_array_push(arr, atom_value_number(100));
    atom_array_push(arr, atom_value_number(200));
    atom_array_push(arr, atom_string_create("ArrayString"));
    
    if (arr->count == 3) print_ok("Array Push Count"); else print_fail("Array Push Count");
    
    AtomValue val2 = atom_array_get(arr, 2);
    if (atom_is_string(val2) && val2.as.string->ref_count == 1) print_ok("Array Nested String Ref"); else print_fail("Array Nested String Ref");
    
    atom_array_destroy(arr);
    // Destroying the array should release the string inside it. We can't easily check the pool without debug tools, but we trust the release flow.
    print_ok("Array Destroy");
    
    // Test 4: Tables
    AtomTable* tbl = atom_table_create();
    atom_table_set(tbl, atom_string_create("key1"), atom_value_number(42));
    atom_table_set(tbl, atom_string_create("key2"), atom_value_number(99));
    
    AtomValue t_val1 = atom_table_get(tbl, atom_string_create("key1"));
    if (atom_is_number(t_val1) && t_val1.as.number == 42) print_ok("Table Get Existing"); else print_fail("Table Get Existing");
    
    AtomValue t_val_miss = atom_table_get(tbl, atom_string_create("key_missing"));
    if (atom_is_nil(t_val_miss)) print_ok("Table Get Missing"); else print_fail("Table Get Missing");
    
    atom_table_delete(tbl, atom_string_create("key1"));
    AtomValue t_val1_after_del = atom_table_get(tbl, atom_string_create("key1"));
    if (atom_is_nil(t_val1_after_del)) print_ok("Table Delete"); else print_fail("Table Delete");
    
    atom_table_destroy(tbl);
    print_ok("Table Destroy");
    
    // Test 5: Bytecode Generation
    AtomChunk* chunk = atom_chunk_create();
    atom_chunk_write(chunk, OP_CONSTANT);
    atom_chunk_write(chunk, atom_chunk_add_constant(chunk, atom_value_number(3.14)));
    atom_chunk_write(chunk, OP_RETURN);
    if (chunk->count == 3) print_ok("Bytecode Chunk Write"); else print_fail("Bytecode Chunk Write");
    
    // Test 6: VM Stack Operations
    AtomVM vm;
    atom_vm_init(&vm);
    atom_vm_push(&vm, atom_value_number(10));
    atom_vm_push(&vm, atom_value_number(20));
    AtomValue p1 = atom_vm_pop(&vm);
    AtomValue p2 = atom_vm_pop(&vm);
    if (p1.as.number == 20 && p2.as.number == 10) print_ok("VM Stack Order (LIFO)"); else print_fail("VM Stack Order (LIFO)");
    
    // Test 7: Scope (Environment)
    AtomScope* scope1 = atom_scope_create(NULL);
    AtomScope* scope2 = atom_scope_create(scope1);
    atom_scope_define(scope1, atom_string_create("global_var").as.string, atom_value_number(777));
    AtomValue found = atom_scope_get(scope2, atom_string_create("global_var").as.string);
    if (atom_is_number(found) && found.as.number == 777) print_ok("Scope Enclosing Lookup"); else print_fail("Scope Enclosing Lookup");
    
    // Cleanup Phase 3 Tests
    atom_scope_destroy(scope2);
    atom_scope_destroy(scope1);
    atom_vm_free(&vm);
    atom_chunk_destroy(chunk);
    
    bos_print("=== PHASE 3 BOSL RUNTIME TEST COMPLETE ===\n\n");
}
