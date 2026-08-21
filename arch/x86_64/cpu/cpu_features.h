#ifndef CPU_FEATURES_H
#define CPU_FEATURES_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool has_fpu;
    bool has_fxsr;
    bool has_sse;
    bool has_sse2;
    bool has_sse3;
    bool has_ssse3;
    bool has_sse4_1;
    bool has_sse4_2;
    bool has_xsave;
    bool has_xsaveopt;
    bool has_avx;
    bool has_avx2;
    uint32_t xsave_size_bytes;
    uint32_t xsave_max_size_bytes;
    uint64_t xfeature_supported_mask;
    char vendor_string[13];
} CPUFeatures;

void cpu_features_init(void);
const CPUFeatures* cpu_get_features(void);
void cpu_features_print(void);

#endif // CPU_FEATURES_H
