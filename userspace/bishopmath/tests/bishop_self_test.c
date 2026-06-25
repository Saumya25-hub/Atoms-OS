#include "../include/bishop_math.h"
#include "../../libbos/include/bos.h"
#include "../internal/bishop_internal.h"

// =============================================
// BISHOP X ENGINE - Self-Test Suite (X-5)
// =============================================

static int g_pass = 0;
static int g_fail = 0;

static void tp(const char* name) {
    bos_print("[TEST] ");
    bos_print(name);
    int len = 0; const char* p = name; while (*p) { len++; p++; }
    for (int i = len; i < 28; i++) bos_print(".");
    bos_print("PASS\n");
    g_pass++;
}

static void tf(const char* name) {
    bos_print("[TEST] ");
    bos_print(name);
    int len = 0; const char* p = name; while (*p) { len++; p++; }
    for (int i = len; i < 28; i++) bos_print(".");
    bos_print("FAIL\n");
    g_fail++;
}

static bool approx(double a, double b, double tol) {
    double d = a - b; if (d < 0) d = -d; return d < tol;
}

// ---- Basic Math ----
static void test_basic(void) {
    double r; bool ok = true;
    if (bishop_add(10.0, 20.0, &r) != BISHOP_OK || r != 30.0) ok = false;
    if (bishop_sub(50.0, 20.0, &r) != BISHOP_OK || r != 30.0) ok = false;
    if (bishop_mul(6.0, 7.0, &r) != BISHOP_OK || r != 42.0) ok = false;
    if (bishop_div(100.0, 4.0, &r) != BISHOP_OK || r != 25.0) ok = false;
    if (bishop_mod(17.0, 5.0, &r) != BISHOP_OK || r != 2.0) ok = false;
    ok ? tp("Basic Add/Sub/Mul/Div/Mod") : tf("Basic Add/Sub/Mul/Div/Mod");
}

// ---- Power & Sqrt ----
static void test_pow_sqrt(void) {
    double r; bool ok = true;
    if (bishop_pow(2.0, 10, &r) != BISHOP_OK || r != 1024.0) ok = false;
    if (bishop_pow(5.0, 0, &r) != BISHOP_OK || r != 1.0) ok = false;
    if (bishop_sqrt(16.0, &r) != BISHOP_OK || !approx(r, 4.0, 0.0001)) ok = false;
    if (bishop_sqrt(2.0, &r) != BISHOP_OK || !approx(r, 1.41421356, 0.0001)) ok = false;
    if (bishop_sqrt(0.0, &r) != BISHOP_OK || r != 0.0) ok = false;
    ok ? tp("Power/Sqrt") : tf("Power/Sqrt");
}

// ---- Div Zero Safety ----
static void test_div_zero(void) {
    bishop_clear_error();
    double r;
    (bishop_div(10.0, 0.0, &r) == BISHOP_ERR_DIV_ZERO) ? tp("Div Zero Safety") : tf("Div Zero Safety");
}

// ---- Overflow Safety ----
static void test_overflow(void) {
    bishop_clear_error();
    double r;
    (bishop_pow(999999.0, 999, &r) == BISHOP_ERR_OVERFLOW) ? tp("Overflow Safety") : tf("Overflow Safety");
}

// ---- Domain Safety ----
static void test_domain(void) {
    bishop_clear_error();
    double r;
    (bishop_sqrt(-1.0, &r) == BISHOP_ERR_DOMAIN) ? tp("Domain Safety") : tf("Domain Safety");
}

// ---- Sin/Cos/Tan ----
static void test_trig(void) {
    double r; bool ok = true;
    if (bishop_sin(0.0, &r) != BISHOP_OK || !approx(r, 0.0, 0.0001)) ok = false;
    if (bishop_cos(0.0, &r) != BISHOP_OK || !approx(r, 1.0, 0.0001)) ok = false;
    if (bishop_sin(BISHOP_PI_2, &r) != BISHOP_OK || !approx(r, 1.0, 0.001)) ok = false;
    if (bishop_cos(BISHOP_PI_2, &r) != BISHOP_OK || !approx(r, 0.0, 0.001)) ok = false;
    if (bishop_sin(BISHOP_PI, &r) != BISHOP_OK || !approx(r, 0.0, 0.001)) ok = false;
    ok ? tp("Sin/Cos/Tan") : tf("Sin/Cos/Tan");
}

// ---- Log/Exp ----
static void test_log_exp(void) {
    double r; bool ok = true;
    if (bishop_exp(0.0, &r) != BISHOP_OK || !approx(r, 1.0, 0.0001)) ok = false;
    if (bishop_exp(1.0, &r) != BISHOP_OK || !approx(r, BISHOP_E, 0.001)) ok = false;
    if (bishop_log(1.0, &r) != BISHOP_OK || !approx(r, 0.0, 0.0001)) ok = false;
    if (bishop_log(BISHOP_E, &r) != BISHOP_OK || !approx(r, 1.0, 0.001)) ok = false;
    if (bishop_log(-1.0, &r) != BISHOP_ERR_DOMAIN) ok = false;
    ok ? tp("Log/Exp") : tf("Log/Exp");
}

// ---- Matrix Create ----
static void test_mat_create(void) {
    BishopMatrix id; bishop_matrix_identity(&id, 3);
    double v; bool ok = true;
    if (id.rows != 3 || id.cols != 3) ok = false;
    if (bishop_matrix_get(&id, 0, 0, &v) != BISHOP_OK || v != 1.0) ok = false;
    if (bishop_matrix_get(&id, 0, 1, &v) != BISHOP_OK || v != 0.0) ok = false;
    if (bishop_matrix_get(&id, 1, 1, &v) != BISHOP_OK || v != 1.0) ok = false;
    ok ? tp("Matrix Create") : tf("Matrix Create");
}

// ---- Matrix Multiply ----
static void test_mat_mul(void) {
    BishopMatrix a, b, res;
    bishop_matrix_init(&a, 2, 2); bishop_matrix_init(&b, 2, 2);
    bishop_matrix_set(&a, 0, 0, 1.0); bishop_matrix_set(&a, 0, 1, 2.0);
    bishop_matrix_set(&a, 1, 0, 3.0); bishop_matrix_set(&a, 1, 1, 4.0);
    bishop_matrix_set(&b, 0, 0, 5.0); bishop_matrix_set(&b, 0, 1, 6.0);
    bishop_matrix_set(&b, 1, 0, 7.0); bishop_matrix_set(&b, 1, 1, 8.0);
    BishopError err = bishop_matrix_multiply(&a, &b, &res);
    double v; bool ok = (err == BISHOP_OK);
    bishop_matrix_get(&res, 0, 0, &v); if (v != 19.0) ok = false;
    bishop_matrix_get(&res, 0, 1, &v); if (v != 22.0) ok = false;
    bishop_matrix_get(&res, 1, 0, &v); if (v != 43.0) ok = false;
    bishop_matrix_get(&res, 1, 1, &v); if (v != 50.0) ok = false;
    ok ? tp("Matrix Multiply") : tf("Matrix Multiply");
}

// ---- Matrix Dimension Safety ----
static void test_mat_dim(void) {
    BishopMatrix a, b, r;
    bishop_matrix_init(&a, 2, 3); bishop_matrix_init(&b, 2, 2);
    (bishop_matrix_multiply(&a, &b, &r) == BISHOP_ERR_DIM_MISMATCH) ? tp("Matrix Dimension Safety") : tf("Matrix Dimension Safety");
}

// ---- Matrix Determinant ----
static void test_mat_det(void) {
    BishopMatrix m; bishop_matrix_init(&m, 2, 2);
    bishop_matrix_set(&m, 0, 0, 3.0); bishop_matrix_set(&m, 0, 1, 8.0);
    bishop_matrix_set(&m, 1, 0, 4.0); bishop_matrix_set(&m, 1, 1, 6.0);
    double det;
    BishopError e = bishop_matrix_determinant(&m, &det);
    (e == BISHOP_OK && approx(det, -14.0, 0.001)) ? tp("Matrix Determinant") : tf("Matrix Determinant");
}

// ---- Vector Ops ----
static void test_vec(void) {
    bool ok = true;
    BishopVec3 a, b, cross_r;
    bishop_vec3_create(&a, 1.0, 2.0, 3.0);
    bishop_vec3_create(&b, 4.0, 5.0, 6.0);
    double dot;
    if (bishop_vec3_dot(&a, &b, &dot) != BISHOP_OK || dot != 32.0) ok = false;
    bishop_vec3_cross(&a, &b, &cross_r);
    if (cross_r.x != -3.0 || cross_r.y != 6.0 || cross_r.z != -3.0) ok = false;
    BishopVec3 c; bishop_vec3_create(&c, 3.0, 4.0, 0.0);
    double mag;
    if (bishop_vec3_magnitude(&c, &mag) != BISHOP_OK || !approx(mag, 5.0, 0.0001)) ok = false;
    ok ? tp("Vector Ops") : tf("Vector Ops");
}

// ---- Vector Zero Safety ----
static void test_vec_zero(void) {
    BishopVec3 zero, out; bishop_vec3_zero(&zero);
    (bishop_vec3_normalize(&zero, &out) == BISHOP_ERR_DIV_ZERO) ? tp("Vector Zero Safety") : tf("Vector Zero Safety");
}

// ---- Complex Numbers ----
static void test_complex(void) {
    bool ok = true;
    BishopComplex a, b_c, prod; double mag;
    bishop_complex_create(&a, 3.0, 4.0);
    if (bishop_complex_magnitude(&a, &mag) != BISHOP_OK || !approx(mag, 5.0, 0.0001)) ok = false;
    bishop_complex_create(&b_c, 1.0, 2.0);
    BishopComplex c_c; bishop_complex_create(&c_c, 3.0, 4.0);
    bishop_complex_mul(&b_c, &c_c, &prod);
    if (!approx(prod.real, -5.0, 0.001) || !approx(prod.imag, 10.0, 0.001)) ok = false;
    BishopComplex conj; bishop_complex_conjugate(&a, &conj);
    if (conj.real != 3.0 || conj.imag != -4.0) ok = false;
    ok ? tp("Complex Numbers") : tf("Complex Numbers");
}

// ---- BigInt ----
static void test_bigint(void) {
    bool ok = true;
    BishopBigInt a = bishop_bigint_from_int(999999999);
    BishopBigInt b = bishop_bigint_from_int(999999999);
    BishopBigInt result;
    if (bishop_bigint_add(&a, &b, &result) != BISHOP_OK) ok = false;
    BishopBigInt expected = bishop_bigint_from_int(1999999998);
    if (bishop_bigint_compare(&result, &expected) != 0) ok = false;
    BishopBigInt m1 = bishop_bigint_from_int(12345);
    BishopBigInt m2 = bishop_bigint_from_int(67890);
    BishopBigInt mul_r;
    if (bishop_bigint_mul(&m1, &m2, &mul_r) != BISHOP_OK) ok = false;
    BishopBigInt exp_mul = bishop_bigint_from_int(838102050);
    if (bishop_bigint_compare(&mul_r, &exp_mul) != 0) ok = false;
    ok ? tp("BigInt Arithmetic") : tf("BigInt Arithmetic");
}

// ---- Statistics ----
static void test_stats(void) {
    bool ok = true;
    double data[] = { 2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0 };
    double r;
    if (bishop_mean(data, 8, &r) != BISHOP_OK || !approx(r, 5.0, 0.001)) ok = false;
    if (bishop_variance(data, 8, &r) != BISHOP_OK || !approx(r, 4.0, 0.001)) ok = false;
    if (bishop_stddev(data, 8, &r) != BISHOP_OK || !approx(r, 2.0, 0.001)) ok = false;
    if (bishop_min_array(data, 8, &r) != BISHOP_OK || r != 2.0) ok = false;
    if (bishop_max_array(data, 8, &r) != BISHOP_OK || r != 9.0) ok = false;
    ok ? tp("Statistics") : tf("Statistics");
}

// ---- Kernel Safety ----
static void test_safety(void) {
    bool ok = true; double r;
    bishop_clear_error();
    if (bishop_div(0.0, 0.0, &r) == BISHOP_OK) ok = false;
    if (bishop_sqrt(-100.0, &r) == BISHOP_OK) ok = false;
    if (bishop_log(0.0, &r) == BISHOP_OK) ok = false;
    if (bishop_log(-5.0, &r) == BISHOP_OK) ok = false;
    if (bishop_mod(10.0, 0.0, &r) == BISHOP_OK) ok = false;
    if (bishop_matrix_multiply(0, 0, 0) == BISHOP_OK) ok = false;
    if (bishop_mean(0, 0, &r) == BISHOP_OK) ok = false;
    ok ? tp("Kernel Safety") : tf("Kernel Safety");
}

static void print_num(int n) {
    char buf[16]; int i = 14; buf[15] = '\0';
    if (n == 0) { buf[i--] = '0'; } else { while (n > 0) { buf[i--] = (n % 10) + '0'; n /= 10; } }
    bos_print(&buf[i+1]);
}

// =============================================
// MAIN ENTRY
// =============================================
void bishop_self_test(void) {
    bos_print("\n=====================================\n");
    bos_print("  BISHOP X ENGINE INITIALIZING\n");
    bos_print("=====================================\n\n");
    g_pass = 0; g_fail = 0;

    test_basic();
    test_pow_sqrt();
    test_div_zero();
    test_overflow();
    test_domain();
    test_trig();
    test_log_exp();
    test_mat_create();
    test_mat_mul();
    test_mat_dim();
    test_mat_det();
    test_vec();
    test_vec_zero();
    test_complex();
    test_bigint();
    test_stats();
    test_safety();

    bos_print("\n=====================================\n");
    if (g_fail == 0) {
        bos_print("  BISHOP X ENGINE READY (");
        print_num(g_pass); bos_print("/"); print_num(g_pass);
        bos_print(" PASS)\n");
    } else {
        bos_print("  BISHOP X ENGINE ERRORS (");
        print_num(g_fail); bos_print(" FAILED)\n");
    }
    bos_print("=====================================\n\n");
}
