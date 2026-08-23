#include "desktop_shell.h"
#include "bomatrix.h"
#include "dom.h"
#include "desktop_vfs_sync.h"
#include "desktop_watcher.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/engine/horse_engine.h"
#include "kernel/media/bopawn/wallpaper/wallpaper_manager.h"
#include "kernel/shell/rook/include/rook.h"
#include "kernel/ui/boasset/boasset.h"
#include "kernel/ui/icon_engine/include/icon_engine.h"
#include "kernel/ui/bofont/bofont.h"
#include "kernel/ui/start_menu.h"
#include "kernel/ui/system_hub.h"
#include "kernel/ui/task_panel.h"
#include "kernel/gui/surface/surface.h"
#include "kernel/shell/apps/drive_icons_data.h"

// Telemetry counters
extern uint32_t g_hud_open_windows;
extern uint32_t g_hud_desktop_icons;
extern uint32_t g_hud_focused_window;
extern uint32_t g_hud_hovered_control;
extern uint32_t g_hud_taskbar_buttons;
extern uint32_t g_hud_notifications;
extern bool g_hud_visible;

// Screen Resolution
extern uint32_t g_kernel_screen_width;
extern uint32_t g_kernel_screen_height;

// Notification Queue
#define MAX_NOTIFICATIONS 4
typedef struct {
  char title[64];
  char message[128];
  uint64_t expire_ticks;
  bool active;
} ShellNotification;
static ShellNotification s_notifications[MAX_NOTIFICATIONS];

static struct BOSSurface *g_desktop_wallpaper = NULL;
static struct BOSSurface *g_old_desktop_wallpaper = NULL;
static uint32_t g_wallpaper_fade_alpha = 255;
extern void display_print(const char *str);
uint32_t g_wallpaper_bg_color = 0xFF0B1120;

// Desktop Clipboard State
static char g_desktop_clipboard_path[128] = "";
static bool g_desktop_clipboard_is_cut = false;

void desktop_set_wallpaper(struct BOSSurface *surface) {
  g_desktop_wallpaper = surface;
  g_wallpaper_fade_alpha = 255;
}

void desktop_set_wallpaper_transition(struct BOSSurface *old_surface,
                                      struct BOSSurface *new_surface) {
  g_old_desktop_wallpaper = old_surface;
  g_desktop_wallpaper = new_surface;
  g_wallpaper_fade_alpha = 0;
}

void desktop_set_wallpaper_alpha(uint32_t alpha) {
  g_wallpaper_fade_alpha = alpha;
}

void desktop_end_wallpaper_transition(void) {
  g_old_desktop_wallpaper = NULL;
  g_wallpaper_fade_alpha = 255;
  desktop_refresh_background();
  wallpaper_manager_transition_finished();
}

struct BOSSurface *desktop_get_wallpaper(void) { return g_desktop_wallpaper; }

void desktop_refresh_background(void) {
  BWE_InvalidateWindow(BWE_DESKTOP_ID);
  extern void BWE_InvalidateAllSurfaces(void);
  BWE_InvalidateAllSurfaces();

  extern BWE_Window g_windows[];
  for (uint32_t i = 0; i < BWE_MAX_WINDOWS; i++) {
    if (g_windows[i].state != BWE_STATE_DESTROYED) {
      BWE_InvalidateWindow(g_windows[i].id);
    }
  }
}

void Shell_DrawWallpaper(const BVFramebuffer *fb, const BWE_Rect *clip) {
  if (!fb || !fb->buffer || !clip) return;

  if (!g_desktop_wallpaper || !g_desktop_wallpaper->framebuffer || 
      g_desktop_wallpaper->width <= 0 || g_desktop_wallpaper->height <= 0) {
    BWE_FillRect(fb, clip->x, clip->y, clip->width, clip->height, g_wallpaper_bg_color);
    return;
  }

  int32_t dest_w = (int32_t)fb->width;
  int32_t dest_h = (int32_t)fb->height;
  int32_t src_w = g_desktop_wallpaper->width;
  int32_t src_h = g_desktop_wallpaper->height;

  uint32_t dest_max_pixels = (uint32_t)(dest_w * dest_h);
  uint32_t src_max_pixels = (uint32_t)(src_w * src_h);
  uint32_t dest_pitch_w = (fb->pitch > 0 && (fb->pitch / 4) <= (uint32_t)dest_w) ? (fb->pitch / 4) : (uint32_t)dest_w;

  for (int32_t y = clip->y; y < clip->y + clip->height; y++) {
    if (y < 0 || y >= dest_h) continue;

    int32_t sy = (y * src_h) / dest_h;
    if (sy < 0) sy = 0;
    if (sy >= src_h) sy = src_h - 1;

    uint32_t dest_row = (uint32_t)y * dest_pitch_w;
    uint32_t src_row = (uint32_t)sy * (uint32_t)src_w;

    for (int32_t x = clip->x; x < clip->x + clip->width; x++) {
      if (x < 0 || x >= dest_w) continue;

      int32_t sx = (x * src_w) / dest_w;
      if (sx < 0) sx = 0;
      if (sx >= src_w) sx = src_w - 1;

      uint32_t d_idx = dest_row + (uint32_t)x;
      uint32_t s_idx = src_row + (uint32_t)sx;

      if (d_idx >= dest_max_pixels || s_idx >= src_max_pixels) continue;

      if (g_old_desktop_wallpaper && g_old_desktop_wallpaper->framebuffer && g_wallpaper_fade_alpha < 255) {
        uint32_t old_pixel = g_old_desktop_wallpaper->framebuffer[s_idx];
        uint32_t new_pixel = g_desktop_wallpaper->framebuffer[s_idx];

        uint8_t a = (uint8_t)(g_wallpaper_fade_alpha);
        uint8_t inv_a = 255 - a;

        uint8_t r = (((old_pixel >> 16) & 0xFF) * inv_a + ((new_pixel >> 16) & 0xFF) * a) / 255;
        uint8_t g = (((old_pixel >> 8) & 0xFF) * inv_a + ((new_pixel >> 8) & 0xFF) * a) / 255;
        uint8_t b = ((old_pixel & 0xFF) * inv_a + (new_pixel & 0xFF) * a) / 255;

        fb->buffer[d_idx] = (0xFF << 24) | (r << 16) | (g << 8) | b;
      } else {
        fb->buffer[d_idx] = g_desktop_wallpaper->framebuffer[s_idx];
      }
    }
  }
}

// Selection Rectangle State
static bool s_desktop_selecting = false;
static int32_t s_select_start_x = 0;
static int32_t s_select_start_y = 0;
static int32_t s_select_current_x = 0;
static int32_t s_select_current_y = 0;

// Desktop Context Menu State
static bool s_desktop_ctx_open = false;
static int32_t s_desktop_ctx_x = 0;
static int32_t s_desktop_ctx_y = 0;
static const char* s_desktop_ctx_items[] = {
    "  View",
    "  Sort by",
    "  Refresh",
    "  + New Folder",
    "  + New Text Document",
    "  Paste",
    "  Display Settings",
    "  Personalize"
};

// Taskbar and Desktop Globals
extern uint32_t g_task_panel_win_id;
extern uint32_t g_start_menu_win_id;
extern bool g_start_menu_open;

// Forward declarations
static void icon_render_callback(BWE_Window *self);
static void icon_event_callback(uint32_t id, const BWE_Event *event);

char g_kbd_selected_icon_name[64] = "";
int32_t g_kbd_selected_icon_index = -1;
uint32_t g_kbd_selected_app_id = 0;

static void update_kbd_selection(void) {
  g_kbd_selected_icon_name[0] = '\0';
  g_kbd_selected_app_id = 0;

  if (g_kbd_selected_icon_index < 0) return;

  extern BWE_Window g_windows[];
  int count = 0;
  for (uint32_t i = 0; i < BWE_MAX_WINDOWS; i++) {
    if (g_windows[i].state != BWE_STATE_DESTROYED && g_windows[i].type == BWE_TYPE_DESKTOP_ICON) {
      if (count == g_kbd_selected_icon_index) {
        strcpy(g_kbd_selected_icon_name, g_windows[i].control_data.button.text);
        DesktopObject* obj = dom_find_by_win_id(g_windows[i].id);
        if (obj) {
          g_kbd_selected_app_id = obj->app_id;
          obj->is_selected = true;
        }
        break;
      }
      count++;
    }
  }
}

bwe_error_t Shell_ShowNotification(const char *title, const char *message, uint32_t duration_ms) {
  int slot = -1;
  for (int i = 0; i < MAX_NOTIFICATIONS; i++) {
    if (!s_notifications[i].active) {
      slot = i;
      break;
    }
  }
  if (slot == -1) slot = 0;

  strcpy(s_notifications[slot].title, title);
  strcpy(s_notifications[slot].message, message);
  extern uint64_t timer_get_ticks(void);
  s_notifications[slot].expire_ticks = timer_get_ticks() + duration_ms;
  s_notifications[slot].active = true;

  g_hud_notifications = 0;
  for (int i = 0; i < MAX_NOTIFICATIONS; i++) {
    if (s_notifications[i].active) g_hud_notifications++;
  }

  BWE_InvalidateWindow(BWE_DESKTOP_ID);
  return BWE_SUCCESS;
}

static void draw_notification_card(const BVFramebuffer *fb, const char *title, const char *msg, int32_t x, int32_t y) {
  int32_t w = 260;
  int32_t h = 60;

  BWE_FillRect(fb, x, y, w, h, 0xEE1E293B);    
  BWE_DrawRect(fb, x, y, w, h, 0xFF475569, 1); 
  BWE_FillRect(fb, x, y, 4, h, 0xFF3B82F6);    

  BWE_DrawText(fb, title, x + 12, y + 10, 0xFFF1F5F9, 0);
  BWE_DrawText(fb, msg, x + 12, y + 32, 0xFF94A3B8, 0);
}

static void render_notifications(const BVFramebuffer *fb) {
  extern uint64_t timer_get_ticks(void);
  uint64_t now = timer_get_ticks();
  int32_t base_x = (int32_t)g_kernel_screen_width - 275;
  int32_t base_y = (int32_t)g_kernel_screen_height - 48 - 70;

  g_hud_notifications = 0;
  for (int i = 0; i < MAX_NOTIFICATIONS; i++) {
    if (s_notifications[i].active) {
      if (now > s_notifications[i].expire_ticks) {
        s_notifications[i].active = false;
        BWE_InvalidateWindow(BWE_DESKTOP_ID);
        continue;
      }
      draw_notification_card(fb, s_notifications[i].title, s_notifications[i].message, base_x, base_y);
      base_y -= 70;
      g_hud_notifications++;
    }
  }
}

// Desktop Surface Event Handler
static void desktop_event_handler(uint32_t window_id, const BWE_Event *event) {
  (void)window_id;
  extern BWE_Window g_windows[];

  if (event->type == BWE_EVENT_MOUSE_DOWN) {
    if (event->data.mouse.buttons & 2) {
      s_desktop_ctx_open = true;
      s_desktop_ctx_x = event->data.mouse.x;
      s_desktop_ctx_y = event->data.mouse.y;
      BWE_InvalidateWindow(BWE_DESKTOP_ID);
      return;
    }

    if (event->data.mouse.buttons & 1) {
      if (s_desktop_ctx_open) {
        int32_t mx = event->data.mouse.x;
        int32_t my = event->data.mouse.y;
        if (mx >= s_desktop_ctx_x && mx <= s_desktop_ctx_x + 160 &&
            my >= s_desktop_ctx_y && my <= s_desktop_ctx_y + 218) {
          int32_t item_idx = (my - (s_desktop_ctx_y + 5)) / 26;
          s_desktop_ctx_open = false;
          BWE_InvalidateWindow(BWE_DESKTOP_ID);

          switch (item_idx) {
            case 0: Shell_ShowNotification("View", "Desktop View: Medium Icons", 3000); break;
            case 1: Shell_ShowNotification("Sort", "Sorted by Name", 3000); break;
            case 2:
              desktop_refresh_background();
              Shell_ShowNotification("Refresh", "Desktop Refreshed!", 3000);
              break;
            case 3: {
              desktop_crud_create_folder("New Folder");
              Shell_ShowNotification("VFS Folder Created", "Created /Desktop/New Folder", 3000);
              break;
            }
            case 4: {
              desktop_crud_create_file("New Document.txt", "ATOMS OS Desktop Notes");
              Shell_ShowNotification("VFS File Created", "Created /Desktop/New Document.txt", 3000);
              break;
            }
            case 5: {
              if (g_desktop_clipboard_path[0] != '\0') {
                desktop_crud_copy(g_desktop_clipboard_path, DESKTOP_VFS_PATH);
                Shell_ShowNotification("Paste", "Pasted copied file to Desktop", 3000);
              } else {
                Shell_ShowNotification("Paste", "Clipboard is empty", 3000);
              }
              break;
            }
            case 6: horse_launch(APP_ID_SETTINGS); break;
            case 7: {
              extern void wallpaper_reload(void);
              wallpaper_reload();
              Shell_ShowNotification("Personalize", "Wallpaper Reloaded!", 3000);
              break;
            }
          }
          return;
        }
        s_desktop_ctx_open = false;
        BWE_InvalidateWindow(BWE_DESKTOP_ID);
      }

      if (SystemHub_HandleCapsuleClick(event->data.mouse.x, event->data.mouse.y,
                                       (int32_t)g_kernel_screen_width,
                                       (int32_t)g_kernel_screen_height)) {
        if (g_start_menu_open) {
          g_start_menu_open = false;
          BOS_Hide(g_start_menu_win_id);
        }
        return;
      }

      SystemHub_ClosePanel();

      if (g_start_menu_open) {
        g_start_menu_open = false;
        BOS_Hide(g_start_menu_win_id);
        BWE_InvalidateWindow(BWE_DESKTOP_ID);
      }

      s_desktop_selecting = true;
      s_select_start_x = event->data.mouse.x;
      s_select_start_y = event->data.mouse.y;
      s_select_current_x = event->data.mouse.x;
      s_select_current_y = event->data.mouse.y;

      dom_clear_selection();
      for (uint32_t i = 0; i < BWE_MAX_WINDOWS; i++) {
        if (g_windows[i].state != BWE_STATE_DESTROYED &&
            g_windows[i].type == BWE_TYPE_DESKTOP_ICON) {
          g_windows[i].control_data.button.is_pressed = false;
        }
      }
      BWE_InvalidateWindow(BWE_DESKTOP_ID);
    }
  } else if (event->type == BWE_EVENT_MOUSE_MOVE) {
    if (s_desktop_selecting) {
      s_select_current_x = event->data.mouse.x;
      s_select_current_y = event->data.mouse.y;

      int32_t x1 = s_select_start_x < s_select_current_x ? s_select_start_x : s_select_current_x;
      int32_t y1 = s_select_start_y < s_select_current_y ? s_select_start_y : s_select_current_y;
      int32_t x2 = s_select_start_x > s_select_current_x ? s_select_start_x : s_select_current_x;
      int32_t y2 = s_select_start_y > s_select_current_y ? s_select_start_y : s_select_current_y;

      for (uint32_t i = 0; i < BWE_MAX_WINDOWS; i++) {
        BWE_Window *icon = &g_windows[i];
        if (icon->state != BWE_STATE_DESTROYED && icon->type == BWE_TYPE_DESKTOP_ICON) {
          BWE_Rect ib = icon->screen_bounds;
          bool intersect = (ib.x < x2 && ib.x + ib.width > x1 && ib.y < y2 && ib.y + ib.height > y1);
          icon->control_data.button.is_pressed = intersect;
          DesktopObject* obj = dom_find_by_win_id(icon->id);
          if (obj) obj->is_selected = intersect;
        }
      }
      BWE_InvalidateWindow(BWE_DESKTOP_ID);
    }
  } else if (event->type == BWE_EVENT_MOUSE_UP) {
    s_desktop_selecting = false;
    BWE_InvalidateWindow(BWE_DESKTOP_ID);
  } else if (event->type == BWE_EVENT_KEY_DOWN) {
    uint32_t kc = event->data.key.key_code;

    // F2 Rename
    if (kc == 0x71) {
      DesktopObject* selected[4];
      uint32_t sel_cnt = dom_get_selected_objects(selected, 4);
      if (sel_cnt > 0 && selected[0]) {
        char new_name[128];
        if (strstr(selected[0]->display_name, "New Folder") != NULL) {
          strcpy(new_name, "My Folder");
        } else if (strstr(selected[0]->display_name, "New Document.txt") != NULL) {
          strcpy(new_name, "Notes_Doc.txt");
        } else {
          strcpy(new_name, selected[0]->display_name);
          strcat(new_name, "_Renamed");
        }
        desktop_crud_rename(selected[0]->vfs_path, new_name);
        Shell_ShowNotification("Rename F2", "Renamed desktop item", 3000);
      }
    }
    // VK_DELETE (0x7F / 0x2E)
    else if (kc == 0x7F || kc == 0x2E) {
      DesktopObject* selected[8];
      uint32_t sel_cnt = dom_get_selected_objects(selected, 8);
      for (uint32_t s = 0; s < sel_cnt; s++) {
        desktop_crud_delete(selected[s]->vfs_path, false);
      }
      Shell_ShowNotification("Delete", "Deleted selected desktop item(s)", 3000);
    }
    // Navigation Keys
    else if (kc == 0x82 || kc == 0x80) { // Left / Up
      g_kbd_selected_icon_index--;
      if (g_kbd_selected_icon_index < 0) g_kbd_selected_icon_index = g_hud_desktop_icons - 1;
      update_kbd_selection();
      BWE_InvalidateWindow(BWE_DESKTOP_ID);
    } else if (kc == 0x83 || kc == 0x81) { // Right / Down
      g_kbd_selected_icon_index++;
      if (g_kbd_selected_icon_index >= (int32_t)g_hud_desktop_icons) g_kbd_selected_icon_index = 0;
      update_kbd_selection();
      BWE_InvalidateWindow(BWE_DESKTOP_ID);
    } else if (kc == 0x84) { // Esc
      g_kbd_selected_icon_index = -1;
      dom_clear_selection();
      update_kbd_selection();
      BWE_InvalidateWindow(BWE_DESKTOP_ID);
    } else if (kc == 10 || kc == 13) { // Enter
      DesktopObject* selected[4];
      uint32_t sel_cnt = dom_get_selected_objects(selected, 4);
      if (sel_cnt > 0 && selected[0]->bwe_win_id != 0) {
        BWE_Event fake_ev;
        fake_ev.type = BWE_EVENT_MOUSE_UP;
        fake_ev.target_id = selected[0]->bwe_win_id;
        icon_event_callback(selected[0]->bwe_win_id, &fake_ev);
        icon_event_callback(selected[0]->bwe_win_id, &fake_ev);
      }
    }
  }
}

static void desktop_paint_handler(BWE_Window *self) {
  extern const BVFramebuffer *BWE_GetRenderTarget(void);
  extern bool BWE_GetClip(BWE_Rect * out_rect);
  const BVFramebuffer *fb = BWE_GetRenderTarget();

  BWE_Rect clip;
  if (!BWE_GetClip(&clip)) clip = self->screen_bounds;
  Shell_DrawWallpaper(fb, &clip);
  SystemHub_RenderCapsule(fb, (int32_t)fb->width, (int32_t)fb->height);
}

void create_desktop_icon_from_object(DesktopObject* obj) {
  if (!obj) return;
  uint32_t icon_id;
  int32_t col = obj->grid_col;
  int32_t row = obj->grid_row;

  if (col < 0 || row < 0) {
    if (!bomatrix_get_next_free_cell(&col, &row)) {
      col = 0;
      row = 0;
    }
  }

  int32_t x = 0, y = 0;
  bomatrix_grid_to_pixel(col, row, &x, &y);

  BOS_CreateSurface(
      BWE_DESKTOP_ID, x, y, BOMATRIX_CELL_WIDTH, BOMATRIX_CELL_HEIGHT,
      BWE_WINDOW_CHILD | BWE_WINDOW_MOVABLE | BWE_WINDOW_BORDERLESS, &icon_id);
  BWE_Window *win = BWE_GetWindow(icon_id);
  if (win) {
    win->type = BWE_TYPE_DESKTOP_ICON;
    strcpy(win->control_data.button.text, obj->display_name);
    win->on_render = icon_render_callback;
    win->on_event = icon_event_callback;
    win->user_data = (void *)(uintptr_t)obj->obj_id;
    obj->bwe_win_id = icon_id;
    obj->grid_col = col;
    obj->grid_row = row;
    obj->x = x;
    obj->y = y;
    bomatrix_register_icon(icon_id, obj->app_id, col, row);
    g_hud_desktop_icons++;
  }
}

static void icon_render_callback(BWE_Window *self) {
  extern const BVFramebuffer *BWE_GetRenderTarget(void);
  const BVFramebuffer *fb = BWE_GetRenderTarget();

  BWE_Rect b = self->screen_bounds;
  bool is_selected = self->control_data.button.is_pressed;
  bool is_hovered = self->control_data.button.is_hovered;

  DesktopObject* obj = dom_find_by_win_id(self->id);
  if (obj && obj->is_selected) is_selected = true;

#include "kernel/wm/botheme/botheme.h"

  if (is_selected) {
    BWE_FillRect(fb, b.x + 4, b.y + 2, b.width - 8, b.height - 4, BOTHEME_GetColor(BOTHEME_DESKTOP_ICON_SELECT));
  } else if (is_hovered) {
    BWE_FillRect(fb, b.x + 4, b.y + 2, b.width - 8, b.height - 4, BOTHEME_GetColor(BOTHEME_DESKTOP_ICON_HOVER));
  }

  uint32_t asset_id = obj ? obj->icon_asset_id : ICON_FOLDER;
  int32_t icon_size = 44;
  int32_t ix = b.x + (b.width - icon_size) / 2;
  int32_t iy = b.y + 4;

  const char *btn_text = self->control_data.button.text;

  bool is_recycle_item = (asset_id == ICON_RECYCLE_BIN) ||
                         (btn_text && (strstr(btn_text, "Recycle") || strstr(btn_text, "Trash"))) ||
                         (obj && obj->vfs_path && (strstr(obj->vfs_path, "Recycle") || strstr(obj->vfs_path, "Trash"))) ||
                         (obj && obj->display_name && (strstr(obj->display_name, "Recycle") || strstr(obj->display_name, "Trash")));

  bool is_usb_item = (asset_id == ICON_USB_DISK) ||
                     (btn_text && (strstr(btn_text, "usb") || strstr(btn_text, "USB") || strstr(btn_text, "flash"))) ||
                     (obj && obj->vfs_path && (strstr(obj->vfs_path, "usb") || strstr(obj->vfs_path, "USB") || strstr(obj->vfs_path, "flash"))) ||
                     (obj && obj->display_name && (strstr(obj->display_name, "usb") || strstr(obj->display_name, "USB") || strstr(obj->display_name, "flash")));

  IconId ico_id = ICON_ID_FOLDER;
  if (btn_text) {
      if (strstr(btn_text, "Computer") || strstr(btn_text, "This PC")) ico_id = ICON_ID_COMPUTER;
      else if (strstr(btn_text, "Files") || strstr(btn_text, "Explorer")) ico_id = ICON_ID_EXPLORER;
      else if (strstr(btn_text, "Terminal")) ico_id = ICON_ID_TERMINAL;
      else if (strstr(btn_text, "Settings")) ico_id = ICON_ID_SETTINGS;
      else if (strstr(btn_text, "Calculator")) ico_id = ICON_ID_CALCULATOR;
      else if (strstr(btn_text, "Media") || strstr(btn_text, "Music")) ico_id = ICON_ID_MEDIA_PLAYER;
      else if (strstr(btn_text, "Trash") || strstr(btn_text, "Recycle")) ico_id = ICON_ID_DOOM;
  } else if (obj) {
      switch (obj->app_id) {
          case APP_ID_EXPLORER:   ico_id = ICON_ID_EXPLORER; break;
          case APP_ID_TERMINAL:   ico_id = ICON_ID_TERMINAL; break;
          case APP_ID_SETTINGS:   ico_id = ICON_ID_SETTINGS; break;
          case APP_ID_CALCULATOR: ico_id = ICON_ID_CALCULATOR; break;
          case APP_ID_MUSIC:      ico_id = ICON_ID_MEDIA_PLAYER; break;
          default: break;
      }
  }

  IconRenderContext dctx;
  dctx.x = ix;
  dctx.y = iy;
  dctx.width = icon_size;
  dctx.height = icon_size;
  dctx.state = is_selected ? ICON_STATE_ACTIVE : (is_hovered ? ICON_STATE_HOVER : ICON_STATE_NORMAL);
  dctx.accent_color = 0;
  dctx.clip = NULL;

  if (!IconEngine_Render(fb, ico_id, &dctx)) {
      if (!BOAsset_DrawAsset(asset_id, ix, iy, icon_size, icon_size)) {
          BWE_FillRect(fb, ix, iy, icon_size, icon_size, 0xFF3B82F6);
      }
  }

  const char *text = self->control_data.button.text;
  BOFontRole label_role = BOFONT_ROLE_UI_MEDIUM;
  BOTextMetrics tm = BOFont_MeasureTextRole(label_role, text);

  char safe_label[32];
  if (tm.width > b.width - 4) {
    int32_t copy_len = 10;
    int32_t orig_len = strlen(text);
    if (orig_len < copy_len) copy_len = orig_len;

    memcpy(safe_label, text, (size_t)copy_len);
    safe_label[copy_len] = '.';
    safe_label[copy_len + 1] = '.';
    safe_label[copy_len + 2] = '\0';
    text = safe_label;
    tm = BOFont_MeasureTextRole(label_role, text);
  }

  int32_t tx = b.x + (b.width - tm.width) / 2;
  if (tx < b.x + 2) tx = b.x + 2;
  BWE_DrawTextRole(fb, text, tx, b.y + 52, 0xFFFFFFFF, label_role);
}

static void icon_event_callback(uint32_t id, const BWE_Event *event) {
  if (!event) return;
  BWE_Window *self = BWE_GetWindow(id);
  if (!self) return;

  if (event->type == BWE_EVENT_MOUSE_DOWN) {
    self->control_data.button.is_pressed = true;
    DesktopObject* obj = dom_find_by_win_id(id);
    if (obj) obj->is_selected = true;
    BWE_InvalidateWindow(BWE_DESKTOP_ID);
  } else if (event->type == BWE_EVENT_MOUSE_UP) {
    int32_t snapped_x = 0, snapped_y = 0;
    bomatrix_snap_icon(id, self->local_bounds.x, self->local_bounds.y, &snapped_x, &snapped_y);
    BOS_SetBounds(id, snapped_x, snapped_y, BOMATRIX_CELL_WIDTH, BOMATRIX_CELL_HEIGHT);

    DesktopObject* obj = dom_find_by_win_id(id);
    if (obj) {
      bomatrix_pixel_to_grid(snapped_x, snapped_y, &obj->grid_col, &obj->grid_row);
      obj->x = snapped_x;
      obj->y = snapped_y;
      desktop_vfs_save_layout();
    }

    static uint64_t s_last_click_ticks = 0;
    static uint32_t s_last_click_id = 0;
    extern uint64_t timer_get_ticks(void);
    uint64_t now = timer_get_ticks();
    if (id == s_last_click_id && (now - s_last_click_ticks) < 400) {
      if (obj) {
        /* Set Windows 11 AppStarting animated cursor spinner (3.0s timed rotation) */
        extern uint32_t bos_cursor_set_active_type(uint32_t type);
        bos_cursor_set_active_type(4 /* BCE_CURSOR_APPSTARTING */);

        if (obj->type == DOM_OBJ_RECYCLE_BIN || strstr(obj->vfs_path, "Recycle") != NULL || strstr(obj->display_name, "Recycle") != NULL) {
          extern int Explorer_LaunchPath(const char* path);
          Explorer_LaunchPath("virtual://RecycleBin");
        } else if (obj->type == DOM_OBJ_USB || strstr(obj->vfs_path, "usb") != NULL || strstr(obj->display_name, "usb") != NULL) {
          extern int Explorer_LaunchPath(const char* path);
          Explorer_LaunchPath(obj->vfs_path);
        } else if (obj->type == DOM_OBJ_FOLDER || obj->type == DOM_OBJ_DRIVE) {
          extern int Explorer_LaunchPath(const char* path);
          Explorer_LaunchPath(obj->vfs_path);
        } else if (strstr(obj->vfs_path, "Media Player") != NULL || strstr(obj->display_name, "Media Player") != NULL || strstr(obj->display_name, "Media") != NULL || obj->app_id == APP_ID_MUSIC) {
          horse_launch(APP_ID_MUSIC);
        } else if (strstr(obj->vfs_path, "Terminal") != NULL || strstr(obj->display_name, "Terminal") != NULL || obj->app_id == APP_ID_TERMINAL) {
          horse_launch(APP_ID_TERMINAL);
        } else if (strstr(obj->vfs_path, "Settings") != NULL || strstr(obj->display_name, "Settings") != NULL || obj->app_id == APP_ID_SETTINGS) {
          horse_launch(APP_ID_SETTINGS);
        } else if (strstr(obj->vfs_path, "Explorer") != NULL || strstr(obj->display_name, "Explorer") != NULL || obj->app_id == APP_ID_EXPLORER) {
          extern int Explorer_LaunchPath(const char* path);
          Explorer_LaunchPath("virtual://ThisPC");
        } else if (obj->app_id > 0) {
          horse_launch(obj->app_id);
        } else if (strstr(obj->vfs_path, ".BOSX") != NULL || strstr(obj->vfs_path, ".bosx") != NULL || strstr(obj->vfs_path, ".elf") != NULL) {
          extern void bosx_loader_open(const char* filepath);
          bosx_loader_open(obj->vfs_path);
        }
      }
      s_last_click_ticks = 0;
    } else {
      s_last_click_ticks = now;
      s_last_click_id = id;
    }
    BWE_InvalidateWindow(BWE_DESKTOP_ID);
  }
}

bool Desktop_Shell_IsBootExperienceActive(void) {
  rook_page_t *p = rook_get_current_page();
  if (p == NULL) return true;
  return (p->id != ROOK_PAGE_DESKTOP);
}
bool Desktop_Shell_IsLoginActive(void) {
  rook_page_t *p = rook_get_current_page();
  return (p != NULL && p->id == ROOK_PAGE_LOGIN);
}
void Desktop_Shell_StartLoginExperience(void) {}
void Desktop_Shell_HandleLoginEvent(const BVEvent *ev) { (void)ev; }
void Desktop_Shell_StartBootExperience(void) {}
uint32_t *rook_get_wallpaper_buffer(void) { return NULL; }
bool rook_is_wallpaper_loaded(void) { return false; }

void Shell_PostComposeHook(const BVFramebuffer *fb) {
  static bool s_was_boot_active = true;
  rook_page_t *current_rook_page = rook_get_current_page();
  if (current_rook_page && current_rook_page->id != ROOK_PAGE_DESKTOP) {
    s_was_boot_active = true;
    rook_update(16);
    if (fb && fb->buffer && current_rook_page->ops.on_render) {
      current_rook_page->ops.on_render(current_rook_page, (uint32_t *)fb->buffer, fb->pitch);
    }
    return;
  }

  if (s_was_boot_active) {
    s_was_boot_active = false;
    extern void com1_puts(const char *s);
    com1_puts("[LOGIN_FLOW] LOADING_EXIT\r\n");
    extern void BWE_RequestFullRedraw(void);
    BWE_RequestFullRedraw();
  }

  if (s_desktop_selecting) {
    int32_t x1 = s_select_start_x < s_select_current_x ? s_select_start_x : s_select_current_x;
    int32_t y1 = s_select_start_y < s_select_current_y ? s_select_start_y : s_select_current_y;
    int32_t w = s_select_start_x > s_select_current_x ? s_select_start_x - x1 : x1 - s_select_start_x;
    int32_t h = s_select_start_y > s_select_current_y ? s_select_start_y - y1 : y1 - s_select_start_y;
    if (w < 0) w = -w;
    if (h < 0) h = -h;

    BWE_FillRect(fb, x1, y1, w, h, 0x333B82F6); 
    BWE_DrawRect(fb, x1, y1, w, h, 0xFF3B82F6, 1);
  }

  if (s_desktop_ctx_open) {
    int32_t cx = s_desktop_ctx_x;
    int32_t cy = s_desktop_ctx_y;
    int32_t cw = 160;
    int32_t ch = 218;
    BWE_FillRect(fb, cx, cy, cw, ch, 0xF00F172A); 
    BWE_DrawRect(fb, cx, cy, cw, ch, 0xFF475569, 1); 
    for (int i = 0; i < 8; i++) {
      int32_t iy = cy + 5 + i * 26;
      BWE_DrawText(fb, s_desktop_ctx_items[i], cx + 8, iy + 4, 0xFFF1F5F9, NULL);
    }
  }

  render_notifications(fb);
  g_hud_focused_window = BOS_GetFocus();

  extern BWE_Window g_windows[];
  uint32_t open_wins = 0;
  for (uint32_t i = 0; i < BWE_MAX_WINDOWS; i++) {
    if (g_windows[i].state != BWE_STATE_DESTROYED &&
        g_windows[i].parent_id == BWE_DESKTOP_ID &&
        g_windows[i].id != BWE_DESKTOP_ID &&
        g_windows[i].id != g_task_panel_win_id &&
        g_windows[i].type == BWE_TYPE_WINDOW) {
      open_wins++;
    }
  }
  g_hud_open_windows = open_wins;
}

bwe_error_t Desktop_Shell_Initialize(void) {
  display_print("[SHELL] Starting ATOMS OS Enterprise Desktop Session...\n");
  memset(s_notifications, 0, sizeof(s_notifications));

  BWE_Window *desktop = BWE_GetWindow(BWE_DESKTOP_ID);
  if (desktop) {
    desktop->on_render = desktop_paint_handler;
    desktop->on_event = desktop_event_handler;
  }

  horse_init();
  wallpaper_manager_init();
  BOAsset_Initialize();
  BOAsset_PreloadCritical();
  bomatrix_init();
  desktop_watcher_init();
  TaskPanel_Initialize();
  StartMenu_Initialize();

  g_hud_desktop_icons = 0;
  return BWE_SUCCESS;
}

void Desktop_Shell_PopulateDesktopIcons(void) {
  // Legacy Ring-0 Desktop population disabled for Milestone 3/Phase 1 Ring-3 Desktop Shell
}
