#ifndef SPINNER_H
#define SPINNER_H

#include <stdint.h>
#include <stdbool.h>

#define SPINNER_DEFAULT_NUM_DOTS 12
#define SPINNER_DEFAULT_RADIUS   18
#define SPINNER_DEFAULT_DOT_R    3

typedef struct {
    int center_x;
    int center_y;
    int radius;           /* Ring radius */
    int dot_radius;       /* Individual dot radius */
    int num_dots;         /* Number of circular dots (e.g. 12) */
    uint32_t color;       /* Color (0x00FFFFFF by default) */
    uint64_t elapsed_ms;  /* Animation timer in ms */
    uint32_t current_angle_deg; /* 0..359 degrees */
} spinner_t;

/* Public API */
void spinner_init(spinner_t* sp, int cx, int cy, int radius, int num_dots);
void spinner_set_position(spinner_t* sp, int cx, int cy);
void spinner_update(spinner_t* sp, uint64_t delta_ms);
void spinner_render(const spinner_t* sp, uint32_t* framebuffer, uint32_t fb_width, uint32_t fb_height, uint32_t fb_stride);

#endif /* SPINNER_H */
