# Wallpaper Manager API

The wallpaper_manager.c acts as the coordinator. It provides the wallpaper_set() API which fetches from the registry, decodes via BOPAWN, scales via the scaling engine, and pushes the final surface to the desktop compositor.