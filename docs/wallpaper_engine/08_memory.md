# Memory Model

Wallpapers allocate heap memory for their BOSSurface. Since they are globally cached, they persist. In a future update, an eviction policy could be added to wallpaper_unload() to free older wallpapers.