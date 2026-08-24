#include "boot_assets.h"

const uint32_t g_boot_ico_lock_png_size = 10026;
const uint32_t g_boot_ico_ethernet_png_size = 4252;
const uint32_t g_boot_ico_chat_png_size = 8039;
const uint32_t g_boot_ico_user_png_size = 13248;

const uint32_t g_boot_wallpaper_qoi_0_size = 3196655;
const uint32_t g_boot_wallpaper_qoi_1_size = 3582959;
const uint32_t g_boot_wallpaper_qoi_2_size = 2620419;

const uint8_t* const g_boot_wallpapers_qoi[BOOT_WALLPAPERS_COUNT] = {
    g_boot_wallpaper_qoi_0,
    g_boot_wallpaper_qoi_1,
    g_boot_wallpaper_qoi_2,
};

const uint32_t g_boot_wallpapers_qoi_sizes[BOOT_WALLPAPERS_COUNT] = {
    3196655,
    3582959,
    2620419,
};

const uint32_t g_boot_wallpaper_qoi_size = 3196655;
