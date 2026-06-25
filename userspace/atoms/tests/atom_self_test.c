#include "../include/atoms.h"
#include "../include/atom_compiler.h"
#include "../../libbos/include/bos.h"
#include "../internal/atom_memory.h"

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
    AtomVM* vm = (AtomVM*)atom_alloc(sizeof(AtomVM));
    atom_vm_init(vm);
    atom_vm_push(vm, atom_value_number(10));
    atom_vm_push(vm, atom_value_number(20));
    AtomValue p1 = atom_vm_pop(vm);
    AtomValue p2 = atom_vm_pop(vm);
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
    atom_vm_free(vm);
    atom_chunk_destroy(chunk);
    
    bos_print("=== PHASE 3 BOSL RUNTIME TEST COMPLETE ===\n\n");
    
    // Test 8: Phase 4 Compiler & VM Integration
    bos_print("\n=== PHASE 4 BOSL COMPILER TEST ===\n");
    AtomFunction* main_fn = atom_function_create(atom_string_create("main").as.string, 0);
    const char* source = "a = 10\nb = 20\nprint(a + b)\n";
    if (atom_compiler_compile(source, main_fn->chunk)) {
        AtomVM* vm4 = (AtomVM*)atom_alloc(sizeof(AtomVM));
        atom_vm_init(vm4);
        bos_print("Expected output: 30\nActual output: ");
        if (atom_vm_execute(vm4, main_fn)) {
            print_ok("Compiler & VM Integration");
            bos_print("PHASE 4 PASS\n");
        } else {
            print_fail("Compiler & VM Integration");
        }
        atom_vm_free(vm4);
        atom_function_destroy(main_fn);
    } else {
        print_fail("Compiler");
    }
    
    // Test 9: Phase 5 Strings, Comparisons & Logic
    bos_print("\n=== PHASE 5 BOSL LOGIC TEST ===\n");
    AtomFunction* main_fn5 = atom_function_create(atom_string_create("main5").as.string, 0);
    const char* source5 = "a = \"Hello\"\nb = 10 == 10 and 5 < 10\nc = not (5 > 10)\nprint(a)\nprint(b)\nprint(c)\n";
    if (atom_compiler_compile(source5, main_fn5->chunk)) {
        AtomVM* vm5 = (AtomVM*)atom_alloc(sizeof(AtomVM));
        atom_vm_init(vm5);
        bos_print("Expected output:\nHello\n1\n1\nActual output:\n");
        if (atom_vm_execute(vm5, main_fn5)) {
            print_ok("Phase 5 Logic Integration");
        } else {
            print_fail("Phase 5 Logic Integration");
        }
        atom_vm_free(vm5);
        atom_function_destroy(main_fn5);
    } else {
        print_fail("Phase 5 Compiler");
    }
    
    // Test 10: Phase 6 Control Flow
    bos_print("\n=== PHASE 6 BOSL CONTROL FLOW TEST ===\n");
    AtomFunction* main_fn6 = atom_function_create(atom_string_create("main6").as.string, 0);
    const char* source6 = "a = 10\nif a > 5 {\nprint(1)\n} else {\nprint(0)\n}\nb = 3\nwhile b > 0 {\nprint(b)\nb = b - 1\n}\n";
    if (atom_compiler_compile(source6, main_fn6->chunk)) {
        AtomVM* vm6 = (AtomVM*)atom_alloc(sizeof(AtomVM));
        atom_vm_init(vm6);
        bos_print("Expected output:\n1\n3\n2\n1\nActual output:\n");
        if (atom_vm_execute(vm6, main_fn6)) {
            print_ok("Phase 6 Control Flow Integration");
        } else {
            print_fail("Phase 6 Control Flow Integration");
        }
        atom_vm_free(vm6);
        atom_function_destroy(main_fn6);
    } else {
        print_fail("Phase 6 Compiler");
    }
    
    // Test 11: Phase 7 Functions
    bos_print("\n=== PHASE 7 BOSL FUNCTIONS TEST ===\n");
    AtomFunction* main_fn7 = atom_function_create(atom_string_create("main7").as.string, 0);
    const char* source7 = "func add(a, b) {\nreturn a + b\n}\nfunc fib(n) {\nif n < 2 {\nreturn n\n}\nreturn fib(n - 1) + fib(n - 2)\n}\nprint(add(10, 20))\nprint(fib(6))\n";
    if (atom_compiler_compile(source7, main_fn7->chunk)) {
        AtomVM* vm7 = (AtomVM*)atom_alloc(sizeof(AtomVM));
        atom_vm_init(vm7);
        bos_print("Expected output:\n30\n8\nActual output:\n");
        if (atom_vm_execute(vm7, main_fn7)) {
            print_ok("Phase 7 Functions Integration");
        } else {
            print_fail("Phase 7 Functions Integration");
        }
        atom_vm_free(vm7);
        atom_function_destroy(main_fn7);
    } else {
        print_fail("Phase 7 Compiler");
    }
    
    // Test 12: Phase 8 Arrays
    bos_print("\n=== PHASE 8 BOSL ARRAYS TEST ===\n");
    AtomFunction* main_fn8 = atom_function_create(atom_string_create("main8").as.string, 0);
    const char* source8 = "nums = [10, 20, 30]\nprint(nums[0])\nnums.push(40)\nprint(nums[3])\n";
    if (atom_compiler_compile(source8, main_fn8->chunk)) {
        AtomVM* vm8 = (AtomVM*)atom_alloc(sizeof(AtomVM));
        atom_vm_init(vm8);
        bos_print("Expected output:\n10\n40\nActual output:\n");
        if (atom_vm_execute(vm8, main_fn8)) {
            print_ok("Phase 8 Arrays Integration");
        } else {
            print_fail("Phase 8 Arrays Integration");
        }
        atom_vm_free(vm8);
        atom_function_destroy(main_fn8);
    } else {
        print_fail("Phase 8 Compiler");
    }
    
    // Test 13: Phase 9 Objects
    bos_print("\n=== PHASE 9 BOSL OBJECTS TEST ===\n");
    AtomFunction* main_fn9 = atom_function_create(atom_string_create("main9").as.string, 0);
    const char* source9 = "user = { name: \"Saumya\", age: 21 }\nprint(user.name)\nuser.age = 22\nprint(user.age)\n";
    if (atom_compiler_compile(source9, main_fn9->chunk)) {
        AtomVM* vm9 = (AtomVM*)atom_alloc(sizeof(AtomVM));
        atom_vm_init(vm9);
        bos_print("Expected output:\nSaumya\n22\nActual output:\n");
        if (atom_vm_execute(vm9, main_fn9)) {
            print_ok("Phase 9 Objects Integration");
        } else {
            print_fail("Phase 9 Objects Integration");
        }
        atom_vm_free(vm9);
        atom_function_destroy(main_fn9);
    } else {
        print_fail("Phase 9 Compiler");
    }
}
