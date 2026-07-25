#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/media/bopawn/formats/image_format.h"
#include "kernel/mm/amsss/amsss.h"
#include <stdbool.h>
#include <stddef.h>

#define CACHE_MAX_ENTRIES 32

static BOSImage *image_cache[CACHE_MAX_ENTRIES];
static int cache_count = 0;
static bool image_cache_provider_registered = false;

extern struct BOSSurface *wallpaper_get(void);

static bool image_cache_is_active_wallpaper(BOSImage *image) {
  struct BOSSurface *active = wallpaper_get();
  return image && image->surface && active == image->surface;
}

static void image_cache_destroy(BOSImage *image) {
  if (!image)
    return;
  surface_destroy(image->surface);
  kfree(image);
}

size_t image_cache_reclaim_registered(void *context, size_t target_bytes) {
  (void)context;
  size_t reclaimed = 0;

  for (int i = 0; i < CACHE_MAX_ENTRIES && reclaimed < target_bytes; ++i) {
    BOSImage *image = image_cache[i];
    if (!image || image->ref_count != 0 ||
        image_cache_is_active_wallpaper(image))
      continue;

    image_cache[i] = NULL;
    if (cache_count > 0)
      cache_count--;
    image_cache_destroy(image);
    reclaimed += sizeof(BOSImage);
  }

  return reclaimed;
}

void image_cache_init(void) {
  /* Initialization is idempotent and must not erase live decoded images. */
  for (int i = 0; i < CACHE_MAX_ENTRIES; i++) {
    if (image_cache[i])
      return;
  }
  cache_count = 0;
}

BOSImage *image_cache_get(const char *path) {
  if (!path)
    return NULL;

  for (int i = 0; i < CACHE_MAX_ENTRIES; i++) {
    if (image_cache[i]) {
      // Compare path
      extern int strcmp(const char *, const char *);
      if (strcmp(image_cache[i]->filepath, path) == 0) {
        image_cache[i]->ref_count++;
        return image_cache[i];
      }
    }
  }
  return NULL;
}

void image_cache_put(BOSImage *image) {
  if (!image)
    return;

  // Find empty slot
  for (int i = 0; i < CACHE_MAX_ENTRIES; i++) {
    if (!image_cache[i]) {
      image_cache[i] = image;
      image->ref_count = 1;
      cache_count++;
      return;
    }
  }

  // Cache full, find one with 0 refcount to evict
  for (int i = 0; i < CACHE_MAX_ENTRIES; i++) {
    if (image_cache[i] && image_cache[i]->ref_count == 0 &&
        !image_cache_is_active_wallpaper(image_cache[i])) {
      // Evict
      image_cache_destroy(image_cache[i]);

      image_cache[i] = image;
      image->ref_count = 1;
      return;
    }
  }

  // If no slot and no evictable, we don't cache (or we can just increase size,
  // but limit is 32) We'll still give it to caller, but ref_count = 1 and it's
  // not in array.
  image->ref_count = 1;
}

void image_cache_release(BOSImage *image) {
  if (!image)
    return;

  if (image->ref_count > 0) {
    image->ref_count--;
  }

  // We do NOT destroy it immediately, allowing LRU/0-ref eviction later
}

void image_cache_remove(const char *path) {
  if (!path)
    return;
  extern int strcmp(const char *, const char *);
  for (int i = 0; i < CACHE_MAX_ENTRIES; i++) {
    if (image_cache[i] && strcmp(image_cache[i]->filepath, path) == 0) {
      if (image_cache_is_active_wallpaper(image_cache[i]))
        continue;
      image_cache_destroy(image_cache[i]);
      image_cache[i] = NULL;
      cache_count--;
      break;
    }
  }
}
