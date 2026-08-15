#ifndef CLOCK_ATLAS_H
#define CLOCK_ATLAS_H

#include <stdint.h>

#define CLOCK_DIGIT_W 76
#define CLOCK_DIGIT_H 160
#define CLOCK_COLON_W 28

#define DATE_FONT_W 20
#define DATE_FONT_H 26

#define NATIVE_ICON_SIZE 24

extern const uint8_t g_clock_digit_atlas[10][CLOCK_DIGIT_H * CLOCK_DIGIT_W];
extern const uint8_t g_clock_colon_atlas[CLOCK_DIGIT_H * CLOCK_COLON_W];

extern const uint8_t g_date_font_atlas[95][DATE_FONT_H * DATE_FONT_W];
extern const uint8_t g_date_font_widths[95];

extern const uint8_t g_lock_icon_atlas[NATIVE_ICON_SIZE * NATIVE_ICON_SIZE];
extern const uint8_t g_ethernet_icon_atlas[NATIVE_ICON_SIZE * NATIVE_ICON_SIZE];
extern const uint8_t g_chat_icon_atlas[NATIVE_ICON_SIZE * NATIVE_ICON_SIZE];

#endif // CLOCK_ATLAS_H
