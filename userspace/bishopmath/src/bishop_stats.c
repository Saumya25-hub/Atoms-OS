#include "../include/bishop_stats.h"
#include "../include/bishop_basic.h"
#include "../internal/bishop_internal.h"

BishopError bishop_sum(const double* data, int count, double* out) {
    if (!data || count <= 0 || !out) return BISHOP_ERR_INVALID_ARG;
    double s = 0.0;
    for (int i = 0; i < count; i++) s += data[i];
    *out = s;
    return BISHOP_OK;
}

BishopError bishop_mean(const double* data, int count, double* out) {
    if (!data || count <= 0 || !out) return BISHOP_ERR_INVALID_ARG;
    double s;
    BishopError e = bishop_sum(data, count, &s);
    if (e != BISHOP_OK) return e;
    *out = s / (double)count;
    return BISHOP_OK;
}

BishopError bishop_variance(const double* data, int count, double* out) {
    if (!data || count <= 0 || !out) return BISHOP_ERR_INVALID_ARG;
    double m;
    BishopError e = bishop_mean(data, count, &m);
    if (e != BISHOP_OK) return e;
    double sum_sq = 0.0;
    for (int i = 0; i < count; i++) {
        double diff = data[i] - m;
        sum_sq += diff * diff;
    }
    *out = sum_sq / (double)count;
    return BISHOP_OK;
}

BishopError bishop_stddev(const double* data, int count, double* out) {
    if (!out) return BISHOP_ERR_INVALID_ARG;
    double v;
    BishopError e = bishop_variance(data, count, &v);
    if (e != BISHOP_OK) return e;
    return bishop_sqrt(v, out);
}

BishopError bishop_min_array(const double* data, int count, double* out) {
    if (!data || count <= 0 || !out) return BISHOP_ERR_INVALID_ARG;
    double min_val = data[0];
    for (int i = 1; i < count; i++) if (data[i] < min_val) min_val = data[i];
    *out = min_val;
    return BISHOP_OK;
}

BishopError bishop_max_array(const double* data, int count, double* out) {
    if (!data || count <= 0 || !out) return BISHOP_ERR_INVALID_ARG;
    double max_val = data[0];
    for (int i = 1; i < count; i++) if (data[i] > max_val) max_val = data[i];
    *out = max_val;
    return BISHOP_OK;
}
