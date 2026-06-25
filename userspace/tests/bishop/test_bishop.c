#include "../test_framework.h"
#include "../../bishopmath/include/bishop_math.h"

void test_bishop_suite(void) {
    TEST_SUITE_START("BishopMath Engine Stress");

    bool success = true;
    double r;
    
    // Run thousands of iterations to ensure no leaks or stack issues
    for (int i = 0; i < 5000; i++) {
        BishopMatrix a, b, res;
        bishop_matrix_init(&a, 3, 3);
        bishop_matrix_init(&b, 3, 3);
        
        // Fill matrices
        bishop_matrix_set(&a, 0, 0, 1.0); bishop_matrix_set(&a, 0, 1, 2.0); bishop_matrix_set(&a, 0, 2, 3.0);
        bishop_matrix_set(&a, 1, 0, 4.0); bishop_matrix_set(&a, 1, 1, 5.0); bishop_matrix_set(&a, 1, 2, 6.0);
        bishop_matrix_set(&a, 2, 0, 7.0); bishop_matrix_set(&a, 2, 1, 8.0); bishop_matrix_set(&a, 2, 2, 9.0);
        
        bishop_matrix_set(&b, 0, 0, 9.0); bishop_matrix_set(&b, 0, 1, 8.0); bishop_matrix_set(&b, 0, 2, 7.0);
        bishop_matrix_set(&b, 1, 0, 6.0); bishop_matrix_set(&b, 1, 1, 5.0); bishop_matrix_set(&b, 1, 2, 4.0);
        bishop_matrix_set(&b, 2, 0, 3.0); bishop_matrix_set(&b, 2, 1, 2.0); bishop_matrix_set(&b, 2, 2, 1.0);
        
        BishopError err = bishop_matrix_multiply(&a, &b, &res);
        if (err != BISHOP_OK) { success = false; break; }
        
        // Some trig
        BishopError tr = bishop_sin(0.785398, &r);
        if (tr != BISHOP_OK) { success = false; break; }
    }
    
    ASSERT(success, "Matrix & Trig Stress (5000 iterations)");

    // Test extreme BigInt operations
    BishopBigInt b1 = bishop_bigint_from_int(999999999);
    BishopBigInt b2 = bishop_bigint_from_int(999999999);
    BishopBigInt b3;
    bool bi_success = true;
    for (int i = 0; i < 5000; i++) {
        if (bishop_bigint_mul(&b1, &b2, &b3) != BISHOP_OK) { bi_success = false; break; }
    }
    ASSERT(bi_success, "BigInt Stress (5000 iterations)");
}
