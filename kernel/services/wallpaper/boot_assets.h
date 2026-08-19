#ifndef BOOT_ASSETS_H
#define BOOT_ASSETS_H

#include <stdint.h>
#include <stddef.h>

extern const uint8_t g_boot_ico_lock_png[10026];
extern const uint32_t g_boot_ico_lock_png_size;

extern const uint8_t g_boot_ico_ethernet_png[4252];
extern const uint32_t g_boot_ico_ethernet_png_size;

extern const uint8_t g_boot_ico_chat_png[8039];
extern const uint32_t g_boot_ico_chat_png_size;

extern const uint8_t g_boot_ico_user_png[13248];
extern const uint32_t g_boot_ico_user_png_size;

#define BOOT_WALLPAPERS_COUNT 4

extern const uint8_t g_boot_wallpaper_qoi_0[3196655];
extern const uint32_t g_boot_wallpaper_qoi_0_size;

extern const uint8_t g_boot_wallpaper_qoi_1[3582959];
extern const uint32_t g_boot_wallpaper_qoi_1_size;

extern const uint8_t g_boot_wallpaper_qoi_2[3453200];
extern const uint32_t g_boot_wallpaper_qoi_2_size;

extern const uint8_t g_boot_wallpaper_qoi_3[3705893];
extern const uint32_t g_boot_wallpaper_qoi_3_size;

extern const uint8_t* const g_boot_wallpapers_qoi[BOOT_WALLPAPERS_COUNT];
extern const uint32_t g_boot_wallpapers_qoi_sizes[BOOT_WALLPAPERS_COUNT];

extern const uint8_t g_boot_wallpaper_qoi[3196655];
extern const uint32_t g_boot_wallpaper_qoi_size;

#endif // BOOT_ASSETS_H
