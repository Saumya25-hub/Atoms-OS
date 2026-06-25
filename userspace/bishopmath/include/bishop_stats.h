#ifndef BISHOP_STATS_H
#define BISHOP_STATS_H

#include "bishop_error.h"
#include <stdint.h>

BishopError bishop_mean(const double* data, int count, double* out);
BishopError bishop_variance(const double* data, int count, double* out);
BishopError bishop_stddev(const double* data, int count, double* out);
BishopError bishop_min_array(const double* data, int count, double* out);
BishopError bishop_max_array(const double* data, int count, double* out);
BishopError bishop_sum(const double* data, int count, double* out);

#endif // BISHOP_STATS_H
