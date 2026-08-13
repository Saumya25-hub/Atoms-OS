#include "../include/ame.h"

/*
 * 🌀 ATOMS Motion Engine (AME) — Spinner Module
 * Implements Windows 11 / Linux style fluid loading spinner dynamics.
 * Single authority animation, time-based, zero heap allocation, 60 FPS.
 */

static AME_Spinner g_boot_spinner_instance;
static bool g_boot_spinner_inited = false;

/* Fixed-point sine and cosine lookup tables (values scaled by 1000) for 0..359
 * deg */
static const int16_t g_cos_1000[360] = {
    1000, 999,  999,  998,  997,  996,  994,  992,  990,  987,  984,  981,
    978,  974,  970,  965,  961,  956,  951,  945,  939,  933,  927,  920,
    913,  906,  898,  891,  882,  874,  866,  857,  848,  838,  829,  819,
    809,  798,  788,  777,  766,  754,  743,  731,  719,  707,  694,  681,
    669,  656,  642,  629,  615,  601,  587,  573,  559,  544,  529,  515,
    500,  484,  469,  453,  438,  422,  406,  390,  374,  358,  342,  325,
    309,  292,  275,  258,  241,  224,  207,  190,  173,  156,  139,  121,
    104,  87,   69,   52,   34,   17,   0,    -17,  -34,  -52,  -69,  -87,
    -104, -121, -139, -156, -173, -190, -207, -224, -241, -258, -275, -292,
    -309, -325, -342, -358, -374, -390, -406, -422, -438, -453, -469, -484,
    -500, -515, -529, -544, -559, -573, -587, -601, -615, -629, -642, -656,
    -669, -681, -694, -707, -719, -731, -743, -754, -766, -777, -788, -798,
    -809, -819, -829, -838, -848, -857, -866, -874, -882, -891, -898, -906,
    -913, -920, -927, -933, -939, -945, -956, -961, -965, -970, -974, -978,
    -981, -984, -987, -990, -992, -994, -996, -997, -998, -999, -999, -1000,
    -999, -999, -998, -997, -996, -994, -992, -990, -987, -984, -981, -978,
    -974, -970, -965, -961, -956, -951, -945, -939, -933, -927, -920, -913,
    -906, -898, -891, -882, -874, -866, -857, -848, -838, -829, -819, -809,
    -798, -788, -777, -766, -754, -743, -731, -719, -707, -694, -681, -669,
    -656, -642, -629, -615, -601, -587, -573, -559, -544, -529, -515, -500,
    -484, -469, -453, -438, -422, -406, -390, -374, -358, -342, -325, -309,
    -292, -275, -258, -241, -224, -207, -190, -173, -156, -139, -121, -104,
    -87,  -69,  -52,  -34,  -17,  0,    17,   34,   52,   69,   87,   104,
    121,  139,  156,  173,  190,  207,  224,  241,  258,  275,  292,  309,
    325,  342,  358,  374,  390,  406,  422,  438,  453,  469,  484,  500,
    515,  529,  544,  559,  573,  587,  601,  615,  629,  642,  656,  669,
    681,  694,  707,  719,  731,  743,  754,  766,  777,  788,  798,  809,
    819,  829,  838,  848,  857,  866,  874,  882,  891,  898,  906,  913,
    920,  927,  933,  939,  945,  951,  956,  961,  965,  970,  974,  978,
    981,  984,  987,  990,  992,  994,  996,  997,  998,  999,  999};

static const int16_t g_sin_1000[360] = {
    0,    17,   34,   52,   69,   87,   104,   121,  139,  156,  173,  190,
    207,  224,  241,  258,  275,  292,  309,   325,  342,  358,  374,  390,
    406,  422,  438,  453,  469,  484,  500,   515,  529,  544,  559,  573,
    587,  601,  615,  629,  642,  656,  669,   681,  694,  707,  719,  731,
    743,  754,  766,  777,  788,  798,  809,   819,  829,  838,  848,  857,
    866,  874,  882,  891,  898,  906,  913,   920,  927,  933,  939,  945,
    951,  956,  961,  965,  970,  974,  978,   981,  984,  987,  990,  992,
    994,  996,  997,  998,  999,  999,  1000,  999,  999,  998,  997,  996,
    994,  992,  990,  987,  984,  981,  978,   974,  970,  965,  961,  956,
    951,  945,  939,  933,  927,  920,  913,   906,  898,  891,  882,  874,
    866,  857,  848,  838,  829,  819,  809,   798,  788,  777,  766,  754,
    743,  731,  719,  707,  694,  681,  669,   656,  642,  629,  615,  601,
    587,  573,  559,  544,  529,  515,  500,   484,  469,  453,  438,  422,
    406,  390,  374,  358,  342,  325,  309,   292,  275,  258,  241,  224,
    207,  190,  173,  156,  139,  121,  104,   87,   69,   52,   34,   17,
    0,    -17,  -34,  -52,  -69,  -87,  -104,  -121, -139, -156, -173, -190,
    -207, -224, -241, -258, -275, -292, -309,  -325, -342, -358, -374, -390,
    -406, -422, -438, -453, -469, -484, -500,  -515, -529, -544, -559, -573,
    -587, -601, -615, -629, -642, -656, -669,  -681, -694, -707, -719, -731,
    -743, -754, -766, -777, -788, -798, -809,  -819, -829, -838, -848, -857,
    -866, -874, -882, -891, -898, -906, -913,  -920, -927, -933, -939, -945,
    -951, -956, -961, -965, -970, -974, -978,  -981, -984, -987, -990, -992,
    -994, -996, -997, -998, -999, -999, -1000, -999, -999, -998, -997, -996,
    -994, -992, -990, -987, -984, -981, -978,  -974, -970, -965, -961, -956,
    -951, -945, -939, -933, -927, -920, -913,  -906, -898, -891, -882, -874,
    -866, -857, -848, -838, -829, -819, -809,  -798, -788, -777, -766, -754,
    -743, -731, -719, -707, -694, -681, -669,  -656, -642, -629, -615, -601,
    -587, -573, -559, -544, -529, -515, -500,  -484, -469, -453, -438, -422,
    -406, -390, -374, -358, -342, -325, -309,  -292, -275, -258, -241, -224,
    -207, -190, -173, -156, -139, -121, -104,  -87,  -69,  -52,  -34,  -17};

/* Opacity decay profile along dot trail (out of 255) */
static const uint8_t g_dot_opacity_trail[12] = {255, 230, 195, 155, 115, 80,
                                                52,  32,  20,  10,  4,   0};

void AME_Spinner_Init(AME_Spinner *sp, int cx, int cy, int radius,
                      int num_dots) {
  if (!sp)
    return;
  sp->center_x = cx;
  sp->center_y = cy;
  sp->radius = (radius > 0) ? radius : 18;
  sp->dot_radius = 3;
  sp->num_dots = (num_dots > 0) ? num_dots : 12;
  if (sp->num_dots > 12)
    sp->num_dots = 12;
  sp->color = 0x00FFFFFF;
  sp->alpha = 255;
  sp->elapsed_ms = 0;
  sp->speed_scale = 1.0f;
  sp->base_angle = 0;
  sp->active = true;
}

void AME_Spinner_SetPosition(AME_Spinner *sp, int cx, int cy) {
  if (sp) {
    sp->center_x = cx;
    sp->center_y = cy;
  }
}

void AME_Spinner_SetSpeed(AME_Spinner *sp, float speed_scale) {
  if (sp && speed_scale > 0.0f) {
    sp->speed_scale = speed_scale;
  }
}

void AME_Spinner_SetAlpha(AME_Spinner *sp, uint8_t alpha) {
  if (sp)
    sp->alpha = alpha;
}

static inline int32_t get_cos_subdeg(int32_t angle_subdeg) {
    while (angle_subdeg < 0) angle_subdeg += 360 * 256;
    angle_subdeg %= (360 * 256);
    int deg = angle_subdeg / 256;
    int frac = angle_subdeg % 256;
    int next_deg = (deg + 1) % 360;
    int32_t c0 = g_cos_1000[deg];
    int32_t c1 = g_cos_1000[next_deg];
    return c0 + (((c1 - c0) * frac) >> 8);
}

static inline int32_t get_sin_subdeg(int32_t angle_subdeg) {
    while (angle_subdeg < 0) angle_subdeg += 360 * 256;
    angle_subdeg %= (360 * 256);
    int deg = angle_subdeg / 256;
    int frac = angle_subdeg % 256;
    int next_deg = (deg + 1) % 360;
    int32_t s0 = g_sin_1000[deg];
    int32_t s1 = g_sin_1000[next_deg];
    return s0 + (((s1 - s0) * frac) >> 8);
}

void AME_Spinner_Update(AME_Spinner *sp, uint64_t delta_ms) {
  if (!sp || !sp->active)
    return;
  sp->elapsed_ms += delta_ms;

  /* Base rotation in 256 sub-degree units: 360 * 256 units per 1400ms */
  uint64_t base_subdeg = ((sp->elapsed_ms * 360ULL * 256ULL) / 1400ULL) % (360ULL * 256ULL);
  sp->base_angle = (uint32_t)base_subdeg;
}

static void draw_filled_circle(uint32_t *fb, uint32_t fb_w, uint32_t fb_h,
                               uint32_t stride_pixels, int cx, int cy, int r,
                               uint32_t color) {
  int r2 = r * r;
  for (int dy = -r; dy <= r; dy++) {
    int py = cy + dy;
    if (py < 0 || py >= (int)fb_h)
      continue;
    int dy2 = dy * dy;
    for (int dx = -r; dx <= r; dx++) {
      int px = cx + dx;
      if (px < 0 || px >= (int)fb_w)
        continue;
      if (dx * dx + dy2 <= r2) {
        fb[py * stride_pixels + px] = color;
      }
    }
  }
}

void AME_Spinner_Render(const AME_Spinner *sp, uint32_t *framebuffer,
                        uint32_t fb_width, uint32_t fb_height,
                        uint32_t fb_stride) {
  if (!sp || !sp->active || !framebuffer || fb_width == 0 || fb_height == 0)
    return;
  if (sp->alpha == 0)
    return;

  uint32_t stride_pixels = (fb_stride >= (fb_width * 4)) ? (fb_stride / 4) : fb_stride;
  if (stride_pixels < fb_width)
    stride_pixels = fb_width;

  int n = sp->num_dots;
  if (n <= 0)
    n = 12;

  /* Windows 11 / Linux Fluent Dynamic Continuous Arc Ring Renderer */
  uint32_t cycle_ms = (uint32_t)(sp->elapsed_ms % 1600ULL);
  int32_t progress_fixed16 = (int32_t)((cycle_ms * 65536ULL) / 1600ULL);
  int32_t ease_val = AME_EvaluateCurve(EASE_IN_OUT_CUBIC, progress_fixed16);

  /* Dynamic arc span expands & contracts between 80 deg and 270 deg */
  int arc_span_deg = 80 + ((ease_val * 190) >> 16);

  /* Render 72 overlapping sub-steps (5-degree increments) for 100% gapless continuous smooth ring */
  int sub_steps = 72;
  int dot_r = (sp->dot_radius > 0) ? sp->dot_radius : 3;

  for (int step = 0; step < sub_steps; step++) {
    int rel_deg = (step * 360) / sub_steps;
    if (rel_deg > arc_span_deg) continue;

    int32_t angle_subdeg = (sp->base_angle + ((arc_span_deg - rel_deg) * 256));

    int dx = (sp->radius * get_cos_subdeg(angle_subdeg)) / 1000;
    int dy = (sp->radius * get_sin_subdeg(angle_subdeg)) / 1000;

    int dot_x = sp->center_x + dx;
    int dot_y = sp->center_y + dy;

    /* Leading head is brilliant white (255), trailing tail fades out smoothly */
    uint32_t opacity = 255 - ((uint32_t)rel_deg * 240ULL / (uint32_t)arc_span_deg);
    uint8_t final_alpha = (uint8_t)((opacity * (uint32_t)sp->alpha) / 255);
    if (final_alpha == 0) continue;

    uint32_t color_val = ((uint32_t)final_alpha << 16) |
                         ((uint32_t)final_alpha << 8) |
                          (uint32_t)final_alpha;

    draw_filled_circle(framebuffer, fb_width, fb_height, stride_pixels, dot_x,
                       dot_y, dot_r, color_val);
  }
}

AME_Spinner *AME_GetBootSpinner(void) {
  if (!g_boot_spinner_inited) {
    AME_Spinner_Init(&g_boot_spinner_instance, 0, 0, 18, 12);
    g_boot_spinner_inited = true;
  }
  return &g_boot_spinner_instance;
}
