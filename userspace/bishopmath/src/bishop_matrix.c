#include "../include/bishop_matrix.h"
#include "../internal/bishop_internal.h"

void bishop_matrix_init(BishopMatrix* mat, int rows, int cols) {
    if (!mat || rows <= 0 || cols <= 0 || rows > BISHOP_MATRIX_MAX_DIM || cols > BISHOP_MATRIX_MAX_DIM) return;
    mat->rows = rows;
    mat->cols = cols;
    for (int i = 0; i < rows * cols; i++) mat->data[i] = 0.0;
}

void bishop_matrix_identity(BishopMatrix* mat, int size) {
    if (!mat || size <= 0 || size > BISHOP_MATRIX_MAX_DIM) return;
    bishop_matrix_init(mat, size, size);
    for (int i = 0; i < size; i++) mat->data[i * size + i] = 1.0;
}

BishopError bishop_matrix_get(const BishopMatrix* mat, int row, int col, double* out) {
    if (!mat || !out || row < 0 || row >= mat->rows || col < 0 || col >= mat->cols) return BISHOP_ERR_INVALID_ARG;
    *out = mat->data[row * mat->cols + col];
    return BISHOP_OK;
}

BishopError bishop_matrix_set(BishopMatrix* mat, int row, int col, double val) {
    if (!mat || row < 0 || row >= mat->rows || col < 0 || col >= mat->cols) return BISHOP_ERR_INVALID_ARG;
    mat->data[row * mat->cols + col] = val;
    return BISHOP_OK;
}

BishopError bishop_matrix_add(const BishopMatrix* a, const BishopMatrix* b, BishopMatrix* result) {
    if (!a || !b || !result) return BISHOP_ERR_INVALID_ARG;
    if (a->rows != b->rows || a->cols != b->cols) return BISHOP_ERR_DIM_MISMATCH;
    bishop_matrix_init(result, a->rows, a->cols);
    for (int i = 0; i < a->rows * a->cols; i++) result->data[i] = a->data[i] + b->data[i];
    return BISHOP_OK;
}

BishopError bishop_matrix_sub(const BishopMatrix* a, const BishopMatrix* b, BishopMatrix* result) {
    if (!a || !b || !result) return BISHOP_ERR_INVALID_ARG;
    if (a->rows != b->rows || a->cols != b->cols) return BISHOP_ERR_DIM_MISMATCH;
    bishop_matrix_init(result, a->rows, a->cols);
    for (int i = 0; i < a->rows * a->cols; i++) result->data[i] = a->data[i] - b->data[i];
    return BISHOP_OK;
}

BishopError bishop_matrix_multiply(const BishopMatrix* a, const BishopMatrix* b, BishopMatrix* result) {
    if (!a || !b || !result) return BISHOP_ERR_INVALID_ARG;
    if (a->cols != b->rows) return BISHOP_ERR_DIM_MISMATCH;
    bishop_matrix_init(result, a->rows, b->cols);
    for (int i = 0; i < a->rows; i++) {
        for (int j = 0; j < b->cols; j++) {
            double sum = 0.0;
            for (int k = 0; k < a->cols; k++) sum += a->data[i * a->cols + k] * b->data[k * b->cols + j];
            result->data[i * b->cols + j] = sum;
        }
    }
    return BISHOP_OK;
}

BishopError bishop_matrix_scale(const BishopMatrix* mat, double scalar, BishopMatrix* result) {
    if (!mat || !result) return BISHOP_ERR_INVALID_ARG;
    bishop_matrix_init(result, mat->rows, mat->cols);
    for (int i = 0; i < mat->rows * mat->cols; i++) result->data[i] = mat->data[i] * scalar;
    return BISHOP_OK;
}

BishopError bishop_matrix_transpose(const BishopMatrix* mat, BishopMatrix* result) {
    if (!mat || !result) return BISHOP_ERR_INVALID_ARG;
    bishop_matrix_init(result, mat->cols, mat->rows);
    for (int i = 0; i < mat->rows; i++)
        for (int j = 0; j < mat->cols; j++)
            result->data[j * mat->rows + i] = mat->data[i * mat->cols + j];
    return BISHOP_OK;
}

// Determinant via cofactor expansion — output via pointer to avoid SSE return
static void det_recursive(const double* data, int n, int stride, double* out) {
    if (n == 1) { *out = data[0]; return; }
    if (n == 2) { *out = data[0] * data[stride + 1] - data[1] * data[stride]; return; }
    double result = 0.0;
    double submatrix[BISHOP_MATRIX_MAX_DIM * BISHOP_MATRIX_MAX_DIM];
    for (int col = 0; col < n; col++) {
        int si = 0;
        for (int i = 1; i < n; i++)
            for (int j = 0; j < n; j++)
                if (j != col) submatrix[si++] = data[i * stride + j];
        double sub_det;
        det_recursive(submatrix, n - 1, n - 1, &sub_det);
        double cofactor = data[col] * sub_det;
        if (col & 1) result -= cofactor; else result += cofactor;
    }
    *out = result;
}

BishopError bishop_matrix_determinant(const BishopMatrix* mat, double* out) {
    if (!mat || !out) return BISHOP_ERR_INVALID_ARG;
    if (mat->rows != mat->cols) return BISHOP_ERR_DIM_MISMATCH;
    if (mat->rows > 8) return BISHOP_ERR_OVERFLOW;
    det_recursive(mat->data, mat->rows, mat->cols, out);
    return BISHOP_OK;
}

BishopError bishop_matrix_trace(const BishopMatrix* mat, double* out) {
    if (!mat || !out) return BISHOP_ERR_INVALID_ARG;
    if (mat->rows != mat->cols) return BISHOP_ERR_DIM_MISMATCH;
    double trace = 0.0;
    for (int i = 0; i < mat->rows; i++) trace += mat->data[i * mat->cols + i];
    *out = trace;
    return BISHOP_OK;
}

static void bishop_print_int(int64_t val) {
    if (val < 0) { bos_print("-"); val = -val; }
    char buf[20]; int i = 18; buf[19] = '\0';
    if (val == 0) { buf[i--] = '0'; }
    else { while (val > 0) { buf[i--] = (val % 10) + '0'; val /= 10; } }
    bos_print(&buf[i+1]);
}

void bishop_matrix_print(const BishopMatrix* mat) {
    if (!mat) return;
    bos_print("Matrix [");
    bishop_print_int(mat->rows);
    bos_print("x");
    bishop_print_int(mat->cols);
    bos_print("]:\n");
    for (int i = 0; i < mat->rows; i++) {
        bos_print("  | ");
        for (int j = 0; j < mat->cols; j++) {
            bishop_print_int((int64_t)mat->data[i * mat->cols + j]);
            bos_print(" ");
        }
        bos_print("|\n");
    }
}
