# Cache Engine

The BOPAWN cache engine (image_cache.c) uses a Least Recently Used (LRU) policy with reference counting. This ensures that frequently accessed images (like icons or desktop wallpapers) are not decoded multiple times, saving memory and CPU cycles.