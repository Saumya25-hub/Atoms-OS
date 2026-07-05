# Event Flow

1. Mouse Click in Settings -> tn_wallpaper_click
2. wallpaper_set(id)
3. BOPAWN Decoding/Cache Hit
4. desktop_set_wallpaper()
5. desktop_refresh_background() -> BWE_InvalidateWindow(BWE_DESKTOP_ID)
6. Compositor calls Shell_DrawWallpaper() for the dirty rect.