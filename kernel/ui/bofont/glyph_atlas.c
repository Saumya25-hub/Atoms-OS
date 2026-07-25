#include "glyph_atlas.h"
#include "kernel/core/lib/include/string.h"

static uint32_t s_fallback_atlas_pixels[128 * 128];
static BOTexture s_fallback_atlas_texture;
#include "kernel/core/memory/heap/include/heap.h"

int BOFontAtlas_Build(BOFont *font, const uint8_t *raw_bitmap_table) {
  if (!font || !raw_bitmap_table) {
    return BOFONT_ERR_INVALID_PARAM;
  }

  if (font->atlas_texture) {
    BOFontAtlas_Destroy(font);
  }

  int32_t char_w = font->glyph_table[32].width;
  int32_t char_h = font->glyph_table[32].height;
  if (char_w <= 0 || char_h <= 0) {
    return BOFONT_ERR_INVALID_FONT_DATA;
  }

  int32_t cols = 16;
  int32_t rows = (BOFONT_MAX_ASCII_GLYPHS + cols - 1) / cols;
  uint32_t atlas_w = (uint32_t)(cols * char_w);
  uint32_t atlas_h = (uint32_t)(rows * char_h);

  BOTexture *tex;
  if (atlas_w == 128 && atlas_h == 128) {
    BOImage_InitExternalTexture(&s_fallback_atlas_texture, atlas_w, atlas_h, 0,
                                (uint8_t *)s_fallback_atlas_pixels);
    tex = &s_fallback_atlas_texture;
  } else {
    tex = BOImage_CreateTexture(atlas_w, atlas_h, 0); // Format 0 = ARGB
    if (!tex || !tex->data) {
      if (tex)
        BOImage_DestroyTexture(tex);
      return BOFONT_ERR_OUT_OF_MEMORY;
    }
  }

  uint32_t *pixels = (uint32_t *)tex->data;
  int32_t bytes_per_row = (char_w + 7) / 8;
  int32_t bytes_per_glyph = bytes_per_row * char_h;

  for (int i = 0; i < BOFONT_MAX_ASCII_GLYPHS; i++) {
    BOGlyph *g = &font->glyph_table[i];
    int32_t col = i % cols;
    int32_t row = i / cols;
    int32_t atlas_x = col * char_w;
    int32_t atlas_y = row * char_h;

    g->atlas_x = atlas_x;
    g->atlas_y = atlas_y;
    g->advance_x = char_w;
    g->bearing_x = 0;
    g->bearing_y = font->ascender;
    g->u1 = (float)atlas_x / (float)atlas_w;
    g->v1 = (float)atlas_y / (float)atlas_h;
    g->u2 = (float)(atlas_x + char_w) / (float)atlas_w;
    g->v2 = (float)(atlas_y + char_h) / (float)atlas_h;
    g->cached = true;

    const uint8_t *glyph_bitmap = raw_bitmap_table + (i * bytes_per_glyph);

    for (int32_t dy = 0; dy < char_h; dy++) {
      for (int32_t dx = 0; dx < char_w; dx++) {
        uint32_t dest_idx = (atlas_y + dy) * atlas_w + (atlas_x + dx);
        bool pixel_on = false;

        if (i >= 32 && i <= 126) {
          uint8_t row_byte = glyph_bitmap[dy * bytes_per_row + (dx / 8)];
          pixel_on = (row_byte & (1 << (dx % 8))) != 0;
        } else if (i == 127 || i < 32) {
          if (dx == 0 || dx == char_w - 1 || dy == 0 || dy == char_h - 1) {
            if (i == 127)
              pixel_on = true;
          }
        }

        if (pixel_on) {
          pixels[dest_idx] = 0xFFFFFFFF;
        } else {
          pixels[dest_idx] = 0x00FFFFFF;
        }
      }
    }
  }

  font->atlas_texture = tex;
  font->is_alpha8 = false;
  font->is_loaded = true;
  return BOFONT_OK;
}

int BOFontAtlas_BuildFromAsset(BOFont *font, const BOFontAsset *asset) {
  if (!font || !asset || !asset->atlas_data || asset->atlas_w == 0 ||
      asset->atlas_h == 0) {
    return BOFONT_ERR_INVALID_PARAM;
  }

  if (font->atlas_texture) {
    BOFontAtlas_Destroy(font);
  }

  if (asset->name) {
    strncpy(font->name, asset->name, sizeof(font->name) - 1);
    font->name[sizeof(font->name) - 1] = '\0';
  }
  font->glyph_size = asset->glyph_size;
  font->line_height = asset->line_height;
  font->ascender = asset->ascender;
  font->descender = asset->descender;
  font->spacing = 0;

  uint64_t total_pixels = (uint64_t)asset->atlas_w * asset->atlas_h;
  if (total_pixels != asset->atlas_data_size) {
    return BOFONT_ERR_INVALID_FONT_DATA;
  }

  /* Generated font coverage is immutable and permanently resident in the
     kernel image. Keep it in place as Alpha8 instead of duplicating it as a
     four-byte ARGB heap texture. The renderer expands a sampled byte. */
  BOTexture *tex = &font->atlas_texture_storage;
  BOImage_InitExternalTexture(tex, asset->atlas_w, asset->atlas_h, 2,
                              (uint8_t *)asset->atlas_data);

  // Populate glyph table from asset metadata
  for (int i = 0; i < BOFONT_MAX_ASCII_GLYPHS; i++) {
    BOGlyph *g = &font->glyph_table[i];
    const BOGlyphMeta *meta = &asset->glyphs[i];

    g->codepoint = meta->codepoint;
    g->advance_x = meta->advance_x;
    g->bearing_x = meta->bearing_x;
    g->bearing_y = meta->bearing_y;
    g->width = meta->width;
    g->height = meta->height;
    g->atlas_x = meta->atlas_x;
    g->atlas_y = meta->atlas_y;
    g->u1 = meta->u1;
    g->v1 = meta->v1;
    g->u2 = meta->u2;
    g->v2 = meta->v2;
    g->cached = true;
  }

  font->atlas_texture = tex;
  font->is_alpha8 = true;
  font->is_loaded = true;
  return BOFONT_OK;
}

void BOFontAtlas_Destroy(BOFont *font) {
  if (!font)
    return;
  if (font->atlas_texture) {
    /* External textures are resident descriptors and own no heap storage. */
    BOImage_DestroyTexture(font->atlas_texture);
    font->atlas_texture = NULL;
  }
  font->is_loaded = false;
}
