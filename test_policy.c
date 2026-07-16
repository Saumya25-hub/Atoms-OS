#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

static uint32_t die_isqrt(uint32_t n) {
    uint32_t res = 0;
    uint32_t bit = 1 << 30;
    while (bit > n) bit >>= 2;
    while (bit != 0) {
        if (n >= res + bit) {
            n -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
        bit >>= 2;
    }
    return res;
}

int main() {
    uint32_t vram_size = 64 * 1024 * 1024;
    uint32_t modes[2][2] = {{1920, 1080}, {1280, 720}};
    
    for (int i = 0; i < 2; i++) {
        uint32_t w = modes[i][0];
        uint32_t h = modes[i][1];
        uint32_t pixel_area = w * h;
        uint32_t fb_size = pixel_area * 4;
        
        uint32_t area_score = die_isqrt(pixel_area) / 4;
        
        uint32_t aspect_score = 0;
        uint32_t ratio_x100 = (w * 100) / h;
        if (ratio_x100 >= 170 && ratio_x100 <= 180) aspect_score = 200;
        
        uint32_t alignment_score = 0;
        uint32_t pitch = w * 4;
        if ((pitch & 63) == 0) alignment_score = 100;
        
        uint32_t vram_score = 0;
        uint32_t util_pct = (fb_size / 1024) * 100 / (vram_size / 1024);
        if (util_pct > 50) vram_score = 0;
        else if (util_pct > 25) vram_score = 100;
        else vram_score = 200 - util_pct;
        if (vram_score > 200) vram_score = 200;
        
        uint32_t bandwidth_score = 0;
        uint32_t refresh = 60;
        uint32_t bandwidth_mb_s = (fb_size / 1024) * refresh / 1024;
        if (bandwidth_mb_s <= 200) bandwidth_score = 200;
        else if (bandwidth_mb_s <= 400) bandwidth_score = 200 - ((bandwidth_mb_s - 200) * 100) / 200;
        else bandwidth_score = 50;
        
        uint32_t geom_score = 200; // Assume true
        
        uint32_t total = area_score + aspect_score + alignment_score + vram_score + bandwidth_score + geom_score;
        printf("Mode %dx%d: Area=%d, Aspect=%d, Align=%d, VRAM=%d, BW=%d, Geom=%d | Total=%d\n",
               w, h, area_score, aspect_score, alignment_score, vram_score, bandwidth_score, geom_score, total);
    }
    return 0;
}
