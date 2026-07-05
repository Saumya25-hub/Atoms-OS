#include <stdint.h>
#include <stddef.h>

static inline int abs_val(int x) { return x < 0 ? -x : x; }

static uint8_t paeth_predictor(uint8_t a, uint8_t b, uint8_t c) {
    int p = a + b - c;
    int pa = abs_val(p - a);
    int pb = abs_val(p - b);
    int pc = abs_val(p - c);
    
    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}

void png_unfilter_scanline(uint8_t filter_type, uint8_t* line, const uint8_t* prev_line, int bytes_per_pixel, int stride) {
    for (int i = 0; i < stride; i++) {
        uint8_t raw = line[i];
        uint8_t a = (i >= bytes_per_pixel) ? line[i - bytes_per_pixel] : 0;
        uint8_t b = prev_line ? prev_line[i] : 0;
        uint8_t c = (prev_line && i >= bytes_per_pixel) ? prev_line[i - bytes_per_pixel] : 0;
        
        switch (filter_type) {
            case 0: // None
                break;
            case 1: // Sub
                line[i] = raw + a;
                break;
            case 2: // Up
                line[i] = raw + b;
                break;
            case 3: // Average
                line[i] = raw + ((a + b) / 2);
                break;
            case 4: // Paeth
                line[i] = raw + paeth_predictor(a, b, c);
                break;
        }
    }
}
