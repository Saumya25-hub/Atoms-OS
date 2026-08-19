#include "wallpaper_manager.h"
#include "kernel/display/agdae/agdae.h"
#include "kernel/gui/animation/animation_fade.h"
#include "kernel/gui/surface/surface.h"
#include "kernel/media/bopawn/bopawn.h"
#include "kernel/media/bopawn/formats/image_format.h"
#include "kernel/shell/desktop_shell/desktop_shell.h"
#include <stddef.h>

extern void image_cache_remove(const char *path);
extern void display_print(const char *text);

static WallpaperEntry *g_current_wallpaper = NULL;
static WallpaperScaleMode g_current_scale_mode = WALLPAPER_SCALE_STRETCH;

/* WSE owns these surfaces. The desktop shell only borrows them for painting. */
static struct BOSSurface *g_active_surface = NULL;
static struct BOSSurface *g_retiring_surface = NULL;

static void wse_log(const char *text) { display_print(text); }

void wallpaper_manager_init(void) {
  wse_log("[WSE] Init\n");
  wallpaper_registry_init();
  wallpaper_set(1);
}

static bool _apply_wallpaper(struct BOSSurface *raw_surf) {
  const AGDAE_Metrics *metrics = AGDAE_GetMetrics();
  if (!raw_surf || !raw_surf->framebuffer) {
    wse_log("[WSE] Draw Skipped (invalid decoded surface)\n");
    return false;
  }
  
  extern uint32_t g_kernel_screen_width;
  extern uint32_t g_kernel_screen_height;
  int32_t target_w = (metrics && metrics->desktop_rect.width > 0) ? metrics->desktop_rect.width : (g_kernel_screen_width > 0 ? (int32_t)g_kernel_screen_width : 1024);
  int32_t target_h = (metrics && metrics->desktop_rect.height > 0) ? metrics->desktop_rect.height : (g_kernel_screen_height > 0 ? (int32_t)g_kernel_screen_height : 768);

  struct BOSSurface *scaled = wallpaper_scaler_scale(
      raw_surf, target_w, target_h,
      g_current_scale_mode, 0xFF0B1120);
  if (!scaled) {
    wse_log("[WSE] Draw Skipped (scale failed)\n");
    return false;
  }
  wse_log("[WSE] Scale Success\n");
  wse_log("[WSE] Surface Created\n");

  /*
   * wallpaper_transition() cancels an older fade before installing the
   * candidate. Its completion path releases g_retiring_surface, so do not
   * destroy that surface here while the desktop still references it.
   */
  struct BOSSurface *old_surface = g_active_surface;
  g_active_surface = scaled;
  g_retiring_surface = old_surface;

  wse_log("[WSE] Wallpaper Loaded\n");
  wse_log("[WSE] Wallpaper Cached\n");
  wallpaper_transition(scaled, 250);
  wse_log("[WSE] Registered To Desktop\n");
  wse_log("[WSE] Wallpaper Registered\n");
  return true;
}

bool wallpaper_load(const char *path) { return wallpaper_set_path(path); }

bool wallpaper_set(uint32_t id) {
  WallpaperEntry *entry = wallpaper_registry_get_by_id(id);
  if (!entry) {
    wse_log("[WSE] Draw Skipped (registry miss)\n");
    return false;
  }

  wse_log("[WSE] Asset Load\n");
  struct BOSImage *img = bopawn_load(entry->path);
  if (!img || !img->surface) {
    wse_log("[WSE] Draw Skipped (asset load or decode failed)\n");
    return false;
  }
  wse_log("[WSE] Decode Success\n");

  entry->width = img->width;
  entry->height = img->height;
  entry->loaded = true;
  g_current_wallpaper = entry;

  bool applied = _apply_wallpaper(img->surface);
  /* Scaler makes a deep copy; release only the decoded source image. */
  image_cache_remove(entry->path);
  entry->cached_surface = NULL;
  if (applied)
    wse_log("[WSE] Wallpaper Changed\n");
  return applied;
}

bool wallpaper_set_path(const char *path) {
  if (!path) {
    wse_log("[WSE] Draw Skipped (null asset path)\n");
    return false;
  }

  wse_log("[WSE] Asset Load\n");
  struct BOSImage *img = bopawn_load(path);
  if (!img || !img->surface) {
    wse_log("[WSE] Draw Skipped (asset load or decode failed)\n");
    return false;
  }
  wse_log("[WSE] Decode Success\n");

  bool applied = _apply_wallpaper(img->surface);
  image_cache_remove(path);
  if (applied)
    wse_log("[WSE] Wallpaper Changed\n");
  return applied;
}
WallpaperEntry *wallpaper_current(void) { return g_current_wallpaper; }

struct BOSSurface *wallpaper_get(void) { return g_active_surface; }
void wallpaper_reload(void) {
  if (g_current_wallpaper)
    wallpaper_set(g_current_wallpaper->id);
}

void wallpaper_manager_transition_finished(void) {
  if (g_retiring_surface) {
    surface_destroy(g_retiring_surface);
    g_retiring_surface = NULL;
    wse_log("[WSE] Wallpaper Released\n");
  }
}

void wallpaper_unload(void) {
  if (g_retiring_surface && g_retiring_surface != g_active_surface) {
    surface_destroy(g_retiring_surface);
  }
  if (g_active_surface)
    surface_destroy(g_active_surface);
  g_retiring_surface = NULL;
  g_active_surface = NULL;
  desktop_set_wallpaper(NULL);
  desktop_refresh_background();
  wse_log("[WSE] Wallpaper Released\n");
}

void wallpaper_cache(void) {
  /* Prepared surfaces are retained by WSE; decoding is never done in paint. */
}

void wallpaper_destroy(void) { wallpaper_unload(); }

WallpaperScaleMode wallpaper_get_scale_mode(void) {
  return g_current_scale_mode;
}

void wallpaper_set_scale_mode(WallpaperScaleMode mode) {
  g_current_scale_mode = mode;
  wallpaper_reload();
}
