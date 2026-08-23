#include "boasset.h"
#include "asset_cache.h"
#include "asset_loader.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/gui/surface/surface.h"

static BOAtlas *s_master_atlas = NULL;

/* The system icon atlas is permanent boot UI state, not a variable-lifetime
   heap object. Its backing store therefore belongs to the BOASSET resident
   pool (.bss), while BOTexture/BOAtlas APIs remain unchanged for callers. */
static BOAtlas s_master_atlas_storage;
static BOTexture s_master_texture_storage;
static uint32_t s_master_pixels[512 * 512];
static uint8_t s_master_occupancy[8 * 8];

static int boimage_png_decode_wrapper(const uint8_t *in_data, uint32_t in_size,
                                      uint8_t **out_pixels, uint32_t *out_width,
                                      uint32_t *out_height) {
  extern struct BOSSurface *png_decode(const uint8_t *buffer, uint32_t size);
  struct BOSSurface *surface = png_decode(in_data, in_size);
  if (!surface || !surface->framebuffer)
    return -1;

  *out_pixels = (uint8_t *)surface->framebuffer;
  *out_width = (uint32_t)surface->width;
  *out_height = (uint32_t)surface->height;
  kfree(surface); // Free surface container struct; pixel buffer is retained by
                  // BOImage
  return 0;
}

void BOAsset_Initialize(void) {
  BOAssetCache_Initialize();

  // Bind BOIMAGE PNG decoder hook to native png_decode()
  BOImage_SetPNGDecoderHook(boimage_png_decode_wrapper);

  // Register predefined system assets into cache table matching FAT32 VFS
  // filenames
  BOAssetCache_Insert(ICON_FOLDER, "folder.png", ASSET_TYPE_ICON);
  BOAssetCache_Insert(ICON_FILE, "file.png", ASSET_TYPE_ICON);
  BOAssetCache_Insert(ICON_TERMINAL, "TERMINAL.PNG", ASSET_TYPE_ICON);
  BOAssetCache_Insert(ICON_EXPLORER, "EXPLORER.PNG", ASSET_TYPE_ICON);
  BOAssetCache_Insert(ICON_SETTINGS, "SETTINGS.PNG", ASSET_TYPE_ICON);
  BOAssetCache_Insert(ICON_CALCULATOR, "CALCULAT.PNG", ASSET_TYPE_ICON);
  BOAssetCache_Insert(ICON_STRESS_TEST, "STRESST.PNG", ASSET_TYPE_ICON);
  BOAssetCache_Insert(ICON_MUSIC, "MUSIC.PNG", ASSET_TYPE_ICON);
  BOAssetCache_Insert(ICON_DOOM, "DOOM.PNG", ASSET_TYPE_ICON);
  BOAssetCache_Insert(ICON_INPUT_LAB, "INPUTLAB.PNG", ASSET_TYPE_ICON);
  BOAssetCache_Insert(ICON_ATRIX, "ATRIX.PNG", ASSET_TYPE_ICON);
  BOAssetCache_Insert(ICON_GRAPH_3D, "GRAPH3D.PNG", ASSET_TYPE_ICON);
  BOAssetCache_Insert(ICON_TMH, "TMH.PNG", ASSET_TYPE_ICON);
  BOAssetCache_Insert(ICON_CLOSE, "close.png", ASSET_TYPE_ICON);
  BOAssetCache_Insert(ICON_MINIMIZE, "minimize.png", ASSET_TYPE_ICON);
  BOAssetCache_Insert(ICON_MAXIMIZE, "maximize.png", ASSET_TYPE_ICON);
  BOAssetCache_Insert(CURSOR_ARROW, "arrow.png", ASSET_TYPE_CURSOR);
  BOAssetCache_Insert(ASSET_LOGO, "logo.png", ASSET_TYPE_IMAGE);
  BOAssetCache_Insert(ASSET_WALLPAPER, "W1.PNG", ASSET_TYPE_WALLPAPER);

  // System Status Icon Set V1.1 (Isolated Namespace)
  BOAssetCache_Insert(ICON_SYS_WIFI_CONN, "icon_system_wifi_connected.png",
                      ASSET_TYPE_ICON);
  BOAssetCache_Insert(ICON_SYS_WIFI_WEAK, "icon_system_wifi_weak.png",
                      ASSET_TYPE_ICON);
  BOAssetCache_Insert(ICON_SYS_WIFI_DISC, "icon_system_wifi_disconnected.png",
                      ASSET_TYPE_ICON);
  BOAssetCache_Insert(ICON_SYS_VOL_NORM, "icon_system_volume_normal.png",
                      ASSET_TYPE_ICON);
  BOAssetCache_Insert(ICON_SYS_VOL_LOW, "icon_system_volume_low.png",
                      ASSET_TYPE_ICON);
  BOAssetCache_Insert(ICON_SYS_VOL_MUTE, "icon_system_volume_muted.png",
                      ASSET_TYPE_ICON);
  BOAssetCache_Insert(ICON_SYS_BAT_NORM, "icon_system_battery_normal.png",
                      ASSET_TYPE_ICON);
  BOAssetCache_Insert(ICON_SYS_BAT_CHG, "icon_system_battery_charging.png",
                      ASSET_TYPE_ICON);
  BOAssetCache_Insert(ICON_SYS_BAT_LOW, "icon_system_battery_low.png",
                      ASSET_TYPE_ICON);
  BOAssetCache_Insert(ICON_SYS_BELL_NORM, "icon_system_notification_normal.png",
                      ASSET_TYPE_ICON);
  BOAssetCache_Insert(ICON_SYS_BELL_UNREAD,
                      "icon_system_notification_unread.png", ASSET_TYPE_ICON);
}

void BOAsset_Shutdown(void) { BOAssetCache_Clear(); }

BOAssetHandle *BOAsset_Load(uint32_t asset_id, const char *filepath,
                            BOAssetType type) {
  BOAssetHandle *handle = BOAssetCache_Insert(asset_id, filepath, type);
  if (!handle)
    return NULL;

  if (handle->loaded) {
    handle->ref_count++;
    return handle;
  }

  int status = BOAssetLoader_LoadFromVFS(handle);
  if (status != BOASSET_OK || !handle->image_data) {
    return NULL;
  }

  handle->ref_count = 1;

  // Bind the permanent 8x8-cell atlas to BOASSET's resident memory pool.
  if (!s_master_atlas) {
    BOImage_InitExternalTexture(&s_master_texture_storage, 512, 512, 0,
                                (uint8_t *)s_master_pixels);
    s_master_atlas_storage.atlas_texture = &s_master_texture_storage;
    s_master_atlas_storage.cell_size = 64;
    s_master_atlas_storage.width_cells = 8;
    s_master_atlas_storage.height_cells = 8;
    s_master_atlas_storage.occupancy_map = s_master_occupancy;
    s_master_atlas = &s_master_atlas_storage;
  }

  if (s_master_atlas) {
    BOImage_AtlasInsert(s_master_atlas, handle->image_data, &handle->u1,
                        &handle->v1, &handle->u2, &handle->v2);
  }

  return handle;
}

BOAssetHandle *BOAsset_Get(uint32_t asset_id) {
  BOAssetHandle *handle = BOAssetCache_Lookup(asset_id);
  if (!handle)
    return NULL;
  if (!handle->loaded) {
    return BOAsset_Load(handle->id, handle->filepath, handle->type);
  }
  handle->ref_count++;
  return handle;
}

void BOAsset_Release(BOAssetHandle *handle) {
  if (!handle || handle->ref_count == 0)
    return;
  handle->ref_count--;
}

void BOAsset_Unload(BOAssetHandle *handle) {
  if (!handle)
    return;
  BOAssetCache_Remove(handle);
}

BOAssetHandle *BOAsset_Reload(BOAssetHandle *handle) {
  if (!handle)
    return NULL;
  BOAsset_Unload(handle);
  return BOAsset_Load(handle->id, handle->filepath, handle->type);
}

void BOAsset_PreloadCritical(void) {
  // Preload all critical desktop app icons into master texture atlas in
  // canonical order
  BOAsset_Get(ASSET_LOGO);
  BOAsset_Get(ICON_FOLDER);
  BOAsset_Get(ICON_FILE);
  BOAsset_Get(ICON_EXPLORER);
  BOAsset_Get(ICON_TERMINAL);
  BOAsset_Get(ICON_SETTINGS);
  BOAsset_Get(ICON_CALCULATOR);
  BOAsset_Get(ICON_STRESS_TEST);
  BOAsset_Get(ICON_MUSIC);
  BOAsset_Get(ICON_DOOM);
  BOAsset_Get(ICON_INPUT_LAB);
  BOAsset_Get(ICON_ATRIX);
  BOAsset_Get(ICON_GRAPH_3D);
  BOAsset_Get(ICON_CLOSE);
}

#include "kernel/ui/icon_engine/include/icon_engine.h"

bool BOAsset_DrawAsset(uint32_t asset_id, int32_t x, int32_t y, int32_t w,
                       int32_t h) {
  extern const BVFramebuffer* BWE_GetRenderTarget(void);
  const BVFramebuffer* fb = BWE_GetRenderTarget();

  // 1. Authoritative HD Icon Engine Fast Path (Zero Heap Allocation)
  if (fb && fb->buffer) {
    IconId icon_id = ICON_ID_NONE;
    switch (asset_id) {
      case ICON_EXPLORER:       icon_id = ICON_ID_EXPLORER; break;
      case ICON_TERMINAL:       icon_id = ICON_ID_TERMINAL; break;
      case ICON_SETTINGS:       icon_id = ICON_ID_SETTINGS; break;
      case ICON_CALCULATOR:     icon_id = ICON_ID_CALCULATOR; break;
      case ICON_FILE:
      case ICON_FOLDER:         icon_id = ICON_ID_FOLDER; break;
      case ICON_STRESS_TEST:
      case ICON_TMH:            icon_id = ICON_ID_TASK_MANAGER; break;
      case ICON_MUSIC:          icon_id = ICON_ID_MEDIA_PLAYER; break;
      case ICON_DOOM:           icon_id = ICON_ID_DOOM; break;
      case ICON_INPUT_LAB:      icon_id = ICON_ID_INPUT_LAB; break;
      case ICON_ATRIX:          icon_id = ICON_ID_ATRIX; break;
      case ICON_GRAPH_3D:       icon_id = ICON_ID_GRAPH_3D; break;
      case ASSET_LOGO:          icon_id = ICON_ID_ATOMS_START; break;
      case ICON_SYS_WIFI_CONN:
      case ICON_SYS_WIFI_WEAK:
      case ICON_SYS_WIFI_DISC:  icon_id = ICON_ID_SYS_WIFI; break;
      case ICON_SYS_VOL_NORM:
      case ICON_SYS_VOL_LOW:
      case ICON_SYS_VOL_MUTE:   icon_id = ICON_ID_SYS_VOLUME; break;
      case ICON_SYS_BAT_NORM:
      case ICON_SYS_BAT_CHG:
      case ICON_SYS_BAT_LOW:    icon_id = ICON_ID_SYS_BATTERY; break;
      case ICON_SYS_BELL_NORM:
      case ICON_SYS_BELL_UNREAD:icon_id = ICON_ID_SYS_BELL; break;
      default: break;
    }

    if (icon_id != ICON_ID_NONE) {
      IconRenderContext ctx;
      ctx.x = x;
      ctx.y = y;
      ctx.width = w;
      ctx.height = h;
      ctx.state = ICON_STATE_NORMAL;
      ctx.accent_color = 0;
      ctx.clip = NULL;
      if (IconEngine_Render(fb, icon_id, &ctx)) {
        return true;
      }
    }
  }

  // 2. Legacy fallback path
  BOAssetHandle *handle = BOAsset_Get(asset_id);

  if (!handle || !handle->loaded) {
    return false;
  }

  if (s_master_atlas && s_master_atlas->atlas_texture) {
    if (fb && fb->buffer) {
      extern void BOImage_DrawGlyphSpriteDirect(const BVFramebuffer *target_fb,
                                                BOTexture *texture, int32_t x, int32_t y,
                                                int32_t width, int32_t height, float u1,
                                                float v1, float u2, float v2,
                                                uint32_t tint_color);
      BOImage_DrawGlyphSpriteDirect(fb, s_master_atlas->atlas_texture, x, y, w, h,
                                    handle->u1, handle->v1, handle->u2, handle->v2,
                                    0xFFFFFFFF);
      return true;
    }
    BOImage_BatchDrawSprite(s_master_atlas->atlas_texture, x, y, w, h,
                            handle->u1, handle->v1, handle->u2, handle->v2);
    return true;
  }

  if (handle->image_data) {
    BOImage_DrawEx(handle->image_data, x, y, w, h, BO_FILTER_BILINEAR);
    return true;
  }
  return false;
}
