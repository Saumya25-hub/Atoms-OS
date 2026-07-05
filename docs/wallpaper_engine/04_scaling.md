# Scaling Engine

The wallpaper_scaler.c supports Stretch, Fit, Fill, Center, and Tile. It generates a pre-scaled BOSSurface matching the screen resolution, allowing the compositor to perform a fast 1:1 copy instead of calculating interpolation per-frame.