#ifndef BISHOP_MATRIX_H
#define BISHOP_MATRIX_H

#include "bishop_error.h"
#include <stdint.h>

#define BISHOP_MATRIX_MAX_DIM 16

typedef struct {
    double  data[BISHOP_MATRIX_MAX_DIM * BISHOP_MATRIX_MAX_DIM];
    int     rows;
    int     cols;
} BishopMatrix;

void          bishop_matrix_init(BishopMatrix* mat, int rows, int cols);
void          bishop_matrix_identity(BishopMatrix* mat, int size);
BishopError   bishop_matrix_get(const BishopMatrix* mat, int row, int col, double* out);
BishopError   bishop_matrix_set(BishopMatrix* mat, int row, int col, double val);
BishopError   bishop_matrix_add(const BishopMatrix* a, const BishopMatrix* b, BishopMatrix* result);
BishopError   bishop_matrix_sub(const BishopMatrix* a, const BishopMatrix* b, BishopMatrix* result);
BishopError   bishop_matrix_multiply(const BishopMatrix* a, const BishopMatrix* b, BishopMatrix* result);
BishopError   bishop_matrix_scale(const BishopMatrix* mat, double scalar, BishopMatrix* result);
BishopError   bishop_matrix_transpose(const BishopMatrix* mat, BishopMatrix* result);
BishopError   bishop_matrix_determinant(const BishopMatrix* mat, double* out);
BishopError   bishop_matrix_trace(const BishopMatrix* mat, double* out);
void          bishop_matrix_print(const BishopMatrix* mat);

#endif // BISHOP_MATRIX_H
