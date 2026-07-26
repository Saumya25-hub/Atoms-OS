#include "desktop_shell.h"
#include "bomatrix.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/engine/horse_engine.h"
#include "kernel/media/bopawn/wallpaper/wallpaper_manager.h"
#include "kernel/ui/boasset/boasset.h"
#include "kernel/ui/bofont/bofont.h"
#include "kernel/ui/start_menu.h"
#include "kernel/ui/system_hub.h"
#include "kernel/ui/task_panel.h"

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

// Wallpaper config
#include "kernel/gui/surface/surface.h"

static struct BOSSurface *g_desktop_wallpaper = NULL;
static struct BOSSurface *g_old_desktop_wallpaper = NULL;
static uint32_t g_wallpaper_fade_alpha = 255;
extern void display_print(const char *str);
uint32_t g_wallpaper_bg_color = 0xFF0B1120;

void desktop_set_wallpaper(struct BOSSurface *surface) {
  /* WSE owns wallpaper surfaces; desktop only borrows them for painting. */
  g_desktop_wallpaper = surface;
  g_wallpaper_fade_alpha = 255;
}

void desktop_set_wallpaper_transition(struct BOSSurface *old_surface,
                                      struct BOSSurface *new_surface) {
  /* WSE retires the old surface after compositor transition commit. */
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
  wallpaper_manager_transition_finished();
}

struct BOSSurface *desktop_get_wallpaper(void) { return g_desktop_wallpaper; }

void desktop_refresh_background(void) { BWE_InvalidateWindow(BWE_DESKTOP_ID); }

void Shell_DrawWallpaper(const BVFramebuffer *fb, const BWE_Rect *clip) {
  if (!fb || !fb->buffer || !clip) {
    display_print("[WSE] Draw Skipped (invalid framebuffer or clip)\n");
    return;
  }
  if (!g_desktop_wallpaper) {
    display_print("[WSE] Draw Skipped (no active surface)\n");
    BWE_FillRect(fb, clip->x, clip->y, clip->width, clip->height,
                 g_wallpaper_bg_color);
    return;
  }
  if (!g_desktop_wallpaper->framebuffer || g_desktop_wallpaper->width <= 0 ||
      g_desktop_wallpaper->height <= 0) {
    display_print("[WSE] Draw Skipped (invalid active surface)\n");
    BWE_FillRect(fb, clip->x, clip->y, clip->width, clip->height,
                 g_wallpaper_bg_color);
    return;
  }

  display_print("[WSE] Draw Begin\n");
  display_print("[WSE] Wallpaper Draw\n");

  int32_t dest_w = (int32_t)fb->width;
  int32_t dest_h = (int32_t)fb->height;
  int32_t src_w = g_desktop_wallpaper->width;
  int32_t src_h = g_desktop_wallpaper->height;

  for (int32_t y = clip->y; y < clip->y + clip->height; y++) {
    if (y < 0 || y >= dest_h || y >= src_h)
      continue;

    uint32_t dest_row = y * (fb->pitch / 4);
    uint32_t src_row = y * src_w;

    for (int32_t x = clip->x; x < clip->x + clip->width; x++) {
      if (x < 0 || x >= dest_w || x >= src_w)
        continue;

      if (g_old_desktop_wallpaper && g_wallpaper_fade_alpha < 255) {
        uint32_t old_pixel = g_old_desktop_wallpaper->framebuffer[src_row + x];
        uint32_t new_pixel = g_desktop_wallpaper->framebuffer[src_row + x];

        uint8_t a = (uint8_t)(g_wallpaper_fade_alpha);
        uint8_t inv_a = 255 - a;

        uint8_t r = (((old_pixel >> 16) & 0xFF) * inv_a +
                     ((new_pixel >> 16) & 0xFF) * a) /
                    255;
        uint8_t g = (((old_pixel >> 8) & 0xFF) * inv_a +
                     ((new_pixel >> 8) & 0xFF) * a) /
                    255;
        uint8_t b = ((old_pixel & 0xFF) * inv_a + (new_pixel & 0xFF) * a) / 255;

        fb->buffer[dest_row + x] = (0xFF << 24) | (r << 16) | (g << 8) | b;
      } else {
        fb->buffer[dest_row + x] =
            g_desktop_wallpaper->framebuffer[src_row + x];
      }
    }
  }
  display_print("[WSE] Draw End\n");
}

// Selection Rectangle State
static bool s_desktop_selecting = false;
static int32_t s_select_start_x = 0;
static int32_t s_select_start_y = 0;
static int32_t s_select_current_x = 0;
static int32_t s_select_current_y = 0;

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

  if (g_kbd_selected_icon_index < 0)
    return;

  extern BWE_Window g_windows[];
  int count = 0;
  for (uint32_t i = 0; i < BWE_MAX_WINDOWS; i++) {
    if (g_windows[i].state != BWE_STATE_DESTROYED &&
        g_windows[i].type == BWE_TYPE_DESKTOP_ICON) {
      if (count == g_kbd_selected_icon_index) {
        strcpy(g_kbd_selected_icon_name, g_windows[i].control_data.button.text);
        g_kbd_selected_app_id = (uint32_t)(uintptr_t)g_windows[i].user_data;
        break;
      }
      count++;
    }
  }
}

// Notification System API
bwe_error_t Shell_ShowNotification(const char *title, const char *message,
                                   uint32_t duration_ms) {
  int slot = -1;
  for (int i = 0; i < MAX_NOTIFICATIONS; i++) {
    if (!s_notifications[i].active) {
      slot = i;
      break;
    }
  }
  if (slot == -1) {
    // Reuse slot 0
    slot = 0;
  }

  strcpy(s_notifications[slot].title, title);
  strcpy(s_notifications[slot].message, message);
  extern uint64_t timer_get_ticks(void);
  s_notifications[slot].expire_ticks = timer_get_ticks() + duration_ms;
  s_notifications[slot].active = true;

  g_hud_notifications = 0;
  for (int i = 0; i < MAX_NOTIFICATIONS; i++) {
    if (s_notifications[i].active)
      g_hud_notifications++;
  }

  BWE_InvalidateWindow(BWE_DESKTOP_ID);
  return BWE_SUCCESS;
}

// Render priority borders and details for notifications
static void draw_notification_card(const BVFramebuffer *fb, const char *title,
                                   const char *msg, int32_t x, int32_t y) {
  int32_t w = 260;
  int32_t h = 60;

  BWE_FillRect(fb, x, y, w, h, 0xEE1E293B);    // Slate-800 backdrop
  BWE_DrawRect(fb, x, y, w, h, 0xFF475569, 1); // Slate-600 border
  BWE_FillRect(fb, x, y, 4, h, 0xFF3B82F6);    // Blue indicator

  BWE_DrawText(fb, title, x + 12, y + 10, 0xFFF1F5F9, 0);
  BWE_DrawText(fb, msg, x + 12, y + 32, 0xFF94A3B8, 0);
}

static void render_notifications(const BVFramebuffer *fb) {
  extern uint64_t timer_get_ticks(void);
  uint64_t now = timer_get_ticks();
  int32_t base_x = (int32_t)g_kernel_screen_width - 275;
  int32_t base_y =
      (int32_t)g_kernel_screen_height - 48 - 70; // 48px is taskbar height

  g_hud_notifications = 0;
  for (int i = 0; i < MAX_NOTIFICATIONS; i++) {
    if (s_notifications[i].active) {
      if (now > s_notifications[i].expire_ticks) {
        s_notifications[i].active = false;
        BWE_InvalidateWindow(BWE_DESKTOP_ID);
        continue;
      }
      draw_notification_card(fb, s_notifications[i].title,
                             s_notifications[i].message, base_x, base_y);
      base_y -= 70;
      g_hud_notifications++;
    }
  }
}

// Desktop Surface Event Handler (Selection rect & desktop clicks)
static void desktop_event_handler(uint32_t window_id, const BWE_Event *event) {
  (void)window_id;
  extern BWE_Window g_windows[];

  if (event->type == BWE_EVENT_MOUSE_DOWN) {
    if (event->data.mouse.buttons & 1) {
      // Check click on Top-Right System Status Capsule
      if (SystemHub_HandleCapsuleClick(event->data.mouse.x, event->data.mouse.y,
                                       (int32_t)g_kernel_screen_width,
                                       (int32_t)g_kernel_screen_height)) {
        if (g_start_menu_open) {
          g_start_menu_open = false;
          BOS_Hide(g_start_menu_win_id);
        }
        return;
      }

      // Close System Hub panel if open
      SystemHub_ClosePanel();

      // Toggle off Start Menu if open
      if (g_start_menu_open) {
        g_start_menu_open = false;
        BOS_Hide(g_start_menu_win_id);
        BWE_InvalidateWindow(BWE_DESKTOP_ID);
      }

      // Start selection rectangle
      s_desktop_selecting = true;
      s_select_start_x = event->data.mouse.x;
      s_select_start_y = event->data.mouse.y;
      s_select_current_x = event->data.mouse.x;
      s_select_current_y = event->data.mouse.y;

      // Deselect all icons
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

      // Calculate bounding box
      int32_t x1 = s_select_start_x < s_select_current_x ? s_select_start_x
                                                         : s_select_current_x;
      int32_t y1 = s_select_start_y < s_select_current_y ? s_select_start_y
                                                         : s_select_current_y;
      int32_t x2 = s_select_start_x > s_select_current_x ? s_select_start_x
                                                         : s_select_current_x;
      int32_t y2 = s_select_start_y > s_select_current_y ? s_select_start_y
                                                         : s_select_current_y;

      // Highlight intersecting icons
      for (uint32_t i = 0; i < BWE_MAX_WINDOWS; i++) {
        BWE_Window *icon = &g_windows[i];
        if (icon->state != BWE_STATE_DESTROYED &&
            icon->type == BWE_TYPE_DESKTOP_ICON) {
          BWE_Rect ib = icon->screen_bounds;
          bool intersect = (ib.x < x2 && ib.x + ib.width > x1 && ib.y < y2 &&
                            ib.y + ib.height > y1);
          icon->control_data.button.is_pressed = intersect;
        }
      }
      BWE_InvalidateWindow(BWE_DESKTOP_ID);
    }
  } else if (event->type == BWE_EVENT_MOUSE_UP) {
    s_desktop_selecting = false;
    BWE_InvalidateWindow(BWE_DESKTOP_ID);
  } else if (event->type == BWE_EVENT_KEY_DOWN) {
    if (event->data.key.key_code == 0x82 ||
        event->data.key.key_code == 0x80) { // Left or Up
      g_kbd_selected_icon_index--;
      if (g_kbd_selected_icon_index < 0)
        g_kbd_selected_icon_index = g_hud_desktop_icons - 1;
      update_kbd_selection();
      BWE_InvalidateWindow(BWE_DESKTOP_ID);
    } else if (event->data.key.key_code == 0x83 ||
               event->data.key.key_code == 0x81) { // Right or Down
      g_kbd_selected_icon_index++;
      if (g_kbd_selected_icon_index >= (int32_t)g_hud_desktop_icons)
        g_kbd_selected_icon_index = 0;
      update_kbd_selection();
      BWE_InvalidateWindow(BWE_DESKTOP_ID);
    } else if (event->data.key.key_code == 0x84) { // Esc
      g_kbd_selected_icon_index = -1;
      update_kbd_selection();
      BWE_InvalidateWindow(BWE_DESKTOP_ID);
    } else if (event->data.key.key_code == 10 ||
               event->data.key.key_code == 13) { // Enter
      if (g_kbd_selected_icon_index >= 0) {
        int count = 0;
        for (uint32_t i = 0; i < BWE_MAX_WINDOWS; i++) {
          if (g_windows[i].state != BWE_STATE_DESTROYED &&
              g_windows[i].type == BWE_TYPE_DESKTOP_ICON) {
            if (count == g_kbd_selected_icon_index) {
              // Synthesize a double-click event (two MOUSE_UPs) to invoke exact
              // same launch path
              BWE_Event fake_ev;
              fake_ev.type = BWE_EVENT_MOUSE_UP;
              fake_ev.target_id = g_windows[i].id;
              icon_event_callback(g_windows[i].id, &fake_ev);
              icon_event_callback(g_windows[i].id,
                                  &fake_ev); // 2nd time triggers double-click
              break;
            }
            count++;
          }
        }
      }
    }
  }
}

// Desktop custom paint callback to render wallpaper & selection rectangle
static void desktop_paint_handler(BWE_Window *self) {
  extern const BVFramebuffer *BWE_GetRenderTarget(void);
  extern bool BWE_GetClip(BWE_Rect * out_rect);
  const BVFramebuffer *fb = BWE_GetRenderTarget();

  // Draw Wallpaper cropped to active clip region
  BWE_Rect clip;
  if (!BWE_GetClip(&clip)) {
    clip = self->screen_bounds;
  }
  Shell_DrawWallpaper(fb, &clip);

  // Render Top-Right System Status Capsule
  SystemHub_RenderCapsule(fb, (int32_t)fb->width, (int32_t)fb->height);
}

// Snapping/layout desktop icons helper
static void create_desktop_icon(const char *name, uint32_t app_id,
                                int32_t grid_x, int32_t grid_y) {
  uint32_t icon_id;
  int32_t col = grid_x;
  int32_t row = grid_y;

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
    strcpy(win->control_data.button.text, name);
    win->on_render = icon_render_callback;
    win->on_event = icon_event_callback;
    win->user_data = (void *)(uintptr_t)app_id; // Store Application Registry ID
    bomatrix_register_icon(icon_id, app_id, col, row);
    g_hud_desktop_icons++;
  }
}

// Icon Render Engine
static void icon_render_callback(BWE_Window *self) {
  extern const BVFramebuffer *BWE_GetRenderTarget(void);
  const BVFramebuffer *fb = BWE_GetRenderTarget();

  BWE_Rect b = self->screen_bounds;
  bool is_selected = self->control_data.button.is_pressed;
  bool is_hovered = self->control_data.button.is_hovered;

  bool is_kbd_selected =
      (g_kbd_selected_icon_index != -1 &&
       (uint32_t)(uintptr_t)self->user_data == g_kbd_selected_app_id);

#include "kernel/wm/botheme/botheme.h"

  // Compact rounded capsule selection / hover visuals
  if (is_selected || is_kbd_selected) {
    BWE_FillRect(fb, b.x + 4, b.y + 2, b.width - 8, b.height - 4,
                 BOTHEME_GetColor(BOTHEME_DESKTOP_ICON_SELECT));
  } else if (is_hovered) {
    BWE_FillRect(fb, b.x + 4, b.y + 2, b.width - 8, b.height - 4,
                 BOTHEME_GetColor(BOTHEME_DESKTOP_ICON_HOVER));
  }

  // Draw BOASSET icon
  uint32_t app_id = (uint32_t)(uintptr_t)self->user_data;
  uint32_t asset_id = ICON_FOLDER;
  if (app_id == APP_ID_EXPLORER)
    asset_id = ICON_EXPLORER;
  else if (app_id == APP_ID_TERMINAL)
    asset_id = ICON_TERMINAL;
  else if (app_id == APP_ID_SETTINGS)
    asset_id = ICON_SETTINGS;
  else if (app_id == APP_ID_CALCULATOR)
    asset_id = ICON_CALCULATOR;
  else if (app_id == APP_ID_STRESS_TEST)
    asset_id = ICON_STRESS_TEST;
  else if (app_id == APP_ID_MUSIC)
    asset_id = ICON_MUSIC;
  else if (app_id == APP_ID_DOOM)
    asset_id = ICON_DOOM;
  else if (app_id == APP_ID_INPUT_LAB)
    asset_id = ICON_INPUT_LAB;
  else if (app_id == APP_ID_ATRIX)
    asset_id = ICON_ATRIX;
  else if (app_id == APP_ID_GRAPH_3D)
    asset_id = ICON_GRAPH_3D;
  else if (app_id == APP_ID_TMH)
    asset_id = ICON_TMH;

  int32_t icon_size = 44;
  int32_t ix = b.x + (b.width - icon_size) / 2;
  int32_t iy = b.y + 4;

  if (!BOAsset_DrawAsset(asset_id, ix, iy, icon_size, icon_size)) {
    // Safe procedural fallback if draw fails
    BWE_FillRect(fb, ix, iy, icon_size, icon_size, 0xFF3B82F6);
  }

  const char *text = self->control_data.button.text;
  BOFontRole label_role = BOFONT_ROLE_UI_MEDIUM;
  BOTextMetrics tm = BOFont_MeasureTextRole(label_role, text);

  char safe_label[32];
  if (tm.width > b.width - 4) {
    int32_t copy_len = 10;
    int32_t orig_len = strlen(text);
    if (orig_len < copy_len)
      copy_len = orig_len;

    memcpy(safe_label, text, (size_t)copy_len);
    safe_label[copy_len] = '.';
    safe_label[copy_len + 1] = '.';
    safe_label[copy_len + 2] = '\0';
    text = safe_label;
    tm = BOFont_MeasureTextRole(label_role, text);
  }

  int32_t tx = b.x + (b.width - tm.width) / 2;
  if (tx < b.x + 2)
    tx = b.x + 2;
  BWE_DrawTextRole(fb, text, tx, b.y + 52, 0xFFFFFFFF, label_role);
}

// Snapping implementation on dragging end
static void icon_event_callback(uint32_t id, const BWE_Event *event) {
  if (!event)
    return;
  BWE_Window *self = BWE_GetWindow(id);
  if (!self)
    return;

  if (event->type == BWE_EVENT_MOUSE_DOWN) {
    // Highlight selection
    self->control_data.button.is_pressed = true;
    BWE_InvalidateWindow(BWE_DESKTOP_ID);
  } else if (event->type == BWE_EVENT_MOUSE_UP) {
    int32_t snapped_x = 0, snapped_y = 0;
    bomatrix_snap_icon(id, self->local_bounds.x, self->local_bounds.y,
                       &snapped_x, &snapped_y);

    BOS_SetBounds(id, snapped_x, snapped_y, BOMATRIX_CELL_WIDTH,
                  BOMATRIX_CELL_HEIGHT);

    // Handle double click logic
    static uint64_t s_last_click_ticks = 0;
    static uint32_t s_last_click_id = 0;
    extern uint64_t timer_get_ticks(void);
    uint64_t now = timer_get_ticks();
    if (id == s_last_click_id && (now - s_last_click_ticks) < 400) {
      uint32_t app_id = (uint32_t)(uintptr_t)self->user_data;
      horse_launch(app_id);
      s_last_click_ticks = 0;
    } else {
      s_last_click_ticks = now;
      s_last_click_id = id;
    }
    BWE_InvalidateWindow(BWE_DESKTOP_ID);
  }
}

extern uint64_t timer_get_ticks(void);
#include "kernel/core/memory/heap/include/heap.h"

#include "kernel/ame/include/ame.h"
#include "kernel/shell/rook/include/rook.h"
#include "kernel/shell/rook/include/rook_pages.h"

extern rook_page_t *rook_page_welcome_get(void);
extern rook_page_t *rook_page_login_get(void);
extern void page_login_handle_event(const BVEvent *ev);

extern void display_print(const char *str);
extern void AME_DestroyAnimation(AME_Handle handle);

typedef enum {
  BOOT_NOT_STARTED = 0,
  BOOT_LOGIN,
  BOOT_RUNNING,
  BOOT_FADING,
  BOOT_FINISHED
} BootExperienceState;

#define DEBUG_DOOM_DIRECT_BOOT 1

#ifdef DEBUG_DOOM_DIRECT_BOOT
static BootExperienceState s_boot_state = BOOT_FINISHED;
#else
static BootExperienceState s_boot_state = BOOT_NOT_STARTED;
#endif
static bool s_boot_experience_active = false;
static bool s_boot_audio_started = false;
static AME_Handle s_boot_fade_handle = AME_INVALID_HANDLE;
static uint32_t *s_welcome_buffer = 0;

bool Desktop_Shell_IsBootExperienceActive(void) {
  return s_boot_experience_active && (s_boot_state != BOOT_FINISHED);
}

bool Desktop_Shell_IsLoginActive(void) { return (s_boot_state == BOOT_LOGIN); }

void Desktop_Shell_StartLoginExperience(void) {
  if (s_boot_state != BOOT_NOT_STARTED) {
    return;
  }
  s_boot_state = BOOT_LOGIN;
  s_boot_experience_active = true;
  s_boot_audio_started = false;

  extern rook_page_t *rook_page_login_get(void);
  rook_page_t *l = rook_page_login_get();
  if (l && l->ops.on_enter) {
    l->ops.on_enter(l);
  }
  display_print("[BOOT_EXP] LoginExperience Start\n");
}

void Desktop_Shell_HandleLoginEvent(const BVEvent *ev) {
  if (s_boot_state != BOOT_LOGIN || !ev)
    return;
  page_login_handle_event(ev);
}

void Desktop_Shell_StartBootExperience(void) {
  if (s_boot_state != BOOT_NOT_STARTED && s_boot_state != BOOT_LOGIN) {
    display_print("[BOOT_EXP] WARNING: Attempted to start boot experience "
                  "twice! Ignored.\n");
    return;
  }

  s_boot_state = BOOT_RUNNING;
  display_print("[BOOT_EXP] BootExperience Welcome Start\n");

  s_boot_experience_active = true;
  s_boot_audio_started = false;
  AME_SetBootExperienceActive(true);

  uint32_t total_pixels = g_kernel_screen_width * g_kernel_screen_height;
  if (!s_welcome_buffer) {
    // [GUARDED ALLOCATION]
    // We need 3686400 bytes. 3686400 / 4096 = 900 pages exactly.
    extern void *vmm_get_active_pml4(void);
    extern void *vmm_alloc_mapped_page(void *pml4, uint64_t virt_addr,
                                       uint32_t flags);
    extern void vmm_unmap_page(void *pml4, uint64_t virt_addr);

    void *pml4 = vmm_get_active_pml4();
    uint64_t base = 0x60000000; // Arbitrary high unmapped virtual address

    // Guard page 1 (before)
    vmm_unmap_page(pml4, base);

    // The buffer (900 pages)
    for (int i = 0; i < 900; i++) {
      vmm_alloc_mapped_page(pml4, base + 0x1000 + (i * 0x1000),
                            3); // Present | RW
    }

    // Guard page 2 (after)
    vmm_unmap_page(pml4, base + 0x1000 + (900 * 0x1000));

    s_welcome_buffer = (uint32_t *)(base + 0x1000);

    display_print("[GUARDED ALLOC] s_welcome_buffer allocated at 0x60001000 "
                  "with guard pages!\n");
  }

  // Reset ROOK welcome page animation state
  rook_page_t *w = rook_page_welcome_get();
  if (w && w->ops.on_enter) {
    w->ops.on_enter(w);
  }

  // Create declarative AME animation: hold at 255 for 3000ms, then fade 255 ->
  // 0 over 2000ms with EASE_IN_OUT_CUBIC Do NOT set AME_FLAG_AUTO_FREE so we
  // can explicitly manage handle destruction when transitioning to
  // BOOT_FINISHED.
  s_boot_fade_handle =
      AME_CreateAnimation(NULL, PROP_OPACITY, 255, 0, 2000, EASE_IN_OUT_CUBIC);
  AME_SetDelay(s_boot_fade_handle, 3000);
  AME_Play(s_boot_fade_handle);
}

// Master Hook in BWE_ComposeFrame for extra overlay renderings
void Shell_PostComposeHook(const BVFramebuffer *fb) {
  // --- BOOT EXPERIENCE OVERLAY ---
  if (s_boot_state == BOOT_FINISHED || !s_boot_experience_active) {
    return;
  }

  if (s_boot_state == BOOT_LOGIN) {
    rook_page_t *l = rook_page_login_get();
    if (l) {
      if (l->ops.on_update) {
        l->ops.on_update(l, 16);
      }
      if (l->ops.on_render) {
        l->ops.on_render(l, (uint32_t *)fb->buffer, fb->pitch);
      }
    }
    return;
  }

  if (!s_welcome_buffer) {
    return;
  }

  // Start audio on first frame AFTER sti (deferred from init)
  if (!s_boot_audio_started) {
    s_boot_audio_started = true;
    extern void audio_player_open(const char *path);
    extern void audio_player_play(void);
    audio_player_open("/BOOT1.WAV");
    audio_player_play();
  }

  rook_page_t *w = rook_page_welcome_get();
  if (!w)
    return;

  // Update welcome screen animation
  if (w->ops.on_update) {
    w->ops.on_update(w, 16);
  }

  AME_State state = AME_GetState(s_boot_fade_handle);

  if (s_boot_state == BOOT_RUNNING && state == AME_STATE_RUNNING) {
    s_boot_state = BOOT_FADING;
    display_print("[BOOT_EXP] BootExperience Fade Start\n");
    // Texture Caching: Render welcome screen into s_welcome_buffer exactly ONCE
    // before fade out begins!
    if (w->ops.on_render) {
      w->ops.on_render(w, s_welcome_buffer,
                       g_kernel_screen_width * sizeof(uint32_t));
    }
  }

  if (s_boot_state == BOOT_RUNNING || state == AME_STATE_QUEUED) {
    // Phase 1: Holding at 255 (0s - 3s)
    if (w->ops.on_render) {
      w->ops.on_render(w, (uint32_t *)fb->buffer, fb->pitch);
    }
  } else if (s_boot_state == BOOT_FADING && state == AME_STATE_RUNNING) {
    // Phase 2: Smooth time-based fade out (3s - 5s)
    // Using immutable cached texture s_welcome_buffer (0 redraws during fade!)

    int32_t alpha = AME_GetCurrentValue(s_boot_fade_handle);
    if (alpha < 0)
      alpha = 0;
    if (alpha > 255)
      alpha = 255;
    int32_t inv_alpha = 255 - alpha;

    uint32_t total_pixels = g_kernel_screen_width * g_kernel_screen_height;
    uint32_t *dst = (uint32_t *)fb->buffer;
    uint32_t *src = s_welcome_buffer;

    extern void heap_check_external_write(uint64_t dst_addr, size_t len,
                                          const char *caller, uint64_t rip);
    heap_check_external_write((uint64_t)dst, total_pixels * sizeof(uint32_t),
                              "BootExp_FadeLoop",
                              (uint64_t)__builtin_return_address(0));

    uint32_t i = 0;
    for (; i + 3 < total_pixels; i += 4) {
      uint32_t desk0 = dst[i], welc0 = src[i];
      uint32_t desk1 = dst[i + 1], welc1 = src[i + 1];
      uint32_t desk2 = dst[i + 2], welc2 = src[i + 2];
      uint32_t desk3 = dst[i + 3], welc3 = src[i + 3];

      uint32_t rb0 = (((desk0 & 0x00FF00FF) * inv_alpha) +
                      ((welc0 & 0x00FF00FF) * alpha)) >>
                     8;
      uint32_t g0 = (((desk0 & 0x0000FF00) * inv_alpha) +
                     ((welc0 & 0x0000FF00) * alpha)) >>
                    8;
      dst[i] = (rb0 & 0x00FF00FF) | (g0 & 0x0000FF00) | 0xFF000000;

      uint32_t rb1 = (((desk1 & 0x00FF00FF) * inv_alpha) +
                      ((welc1 & 0x00FF00FF) * alpha)) >>
                     8;
      uint32_t g1 = (((desk1 & 0x0000FF00) * inv_alpha) +
                     ((welc1 & 0x0000FF00) * alpha)) >>
                    8;
      dst[i + 1] = (rb1 & 0x00FF00FF) | (g1 & 0x0000FF00) | 0xFF000000;

      uint32_t rb2 = (((desk2 & 0x00FF00FF) * inv_alpha) +
                      ((welc2 & 0x00FF00FF) * alpha)) >>
                     8;
      uint32_t g2 = (((desk2 & 0x0000FF00) * inv_alpha) +
                     ((welc2 & 0x0000FF00) * alpha)) >>
                    8;
      dst[i + 2] = (rb2 & 0x00FF00FF) | (g2 & 0x0000FF00) | 0xFF000000;

      uint32_t rb3 = (((desk3 & 0x00FF00FF) * inv_alpha) +
                      ((welc3 & 0x00FF00FF) * alpha)) >>
                     8;
      uint32_t g3 = (((desk3 & 0x0000FF00) * inv_alpha) +
                     ((welc3 & 0x0000FF00) * alpha)) >>
                    8;
      dst[i + 3] = (rb3 & 0x00FF00FF) | (g3 & 0x0000FF00) | 0xFF000000;
    }
    for (; i < total_pixels; i++) {
      uint32_t desk = dst[i], welc = src[i];
      uint32_t rb =
          (((desk & 0x00FF00FF) * inv_alpha) + ((welc & 0x00FF00FF) * alpha)) >>
          8;
      uint32_t g =
          (((desk & 0x0000FF00) * inv_alpha) + ((welc & 0x0000FF00) * alpha)) >>
          8;
      dst[i] = (rb & 0x00FF00FF) | (g & 0x0000FF00) | 0xFF000000;
    }
  } else if (state == AME_STATE_COMPLETED || state == AME_STATE_IDLE ||
             state == AME_STATE_CANCELLED) {
    // Phase 3: Boot experience complete
    if (s_boot_state != BOOT_FINISHED) {
      s_boot_state = BOOT_FINISHED;
      display_print("[BOOT_EXP] BootExperience Fade Complete\n");

      s_boot_experience_active = false;
      AME_SetBootExperienceActive(false);

      if (s_boot_fade_handle != AME_INVALID_HANDLE) {
        AME_DestroyAnimation(s_boot_fade_handle);
        s_boot_fade_handle = AME_INVALID_HANDLE;
      }

      if (s_welcome_buffer) {
        kfree(s_welcome_buffer);
        s_welcome_buffer = 0;
      }

      display_print("[BOOT_EXP] BootExperience Destroy\n");
    }
  }

  // 1. Draw Selection Box
  if (s_desktop_selecting) {
    int32_t x1 = s_select_start_x < s_select_current_x ? s_select_start_x
                                                       : s_select_current_x;
    int32_t y1 = s_select_start_y < s_select_current_y ? s_select_start_y
                                                       : s_select_current_y;
    int32_t w = s_select_start_x > s_select_current_x ? s_select_start_x - x1
                                                      : x1 - s_select_start_x;
    int32_t h = s_select_start_y > s_select_current_y ? s_select_start_y - y1
                                                      : y1 - s_select_start_y;
    if (w < 0)
      w = -w;
    if (h < 0)
      h = -h;

    BWE_FillRect(fb, x1, y1, w, h, 0x333B82F6); // 20% alpha blue
    BWE_DrawRect(fb, x1, y1, w, h, 0xFF3B82F6, 1);
  }

  // 2. Draw Active Notifications
  render_notifications(fb);

  // 3. Update HUD Telemetry
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

  // Update active cursor location
  extern int32_t g_bwe_mouse_x;
  extern int32_t g_bwe_mouse_y;
  g_hud_hovered_control = 0;
  for (int32_t i = (int32_t)BWE_MAX_WINDOWS - 1; i >= 0; i--) {
    if (g_windows[i].state != BWE_STATE_DESTROYED &&
        g_windows[i].id != BWE_DESKTOP_ID) {
      BWE_HitZone hit =
          BWE_HitTest(g_windows[i].id, g_bwe_mouse_x, g_bwe_mouse_y);
      if (hit != BWE_HIT_NONE) {
        g_hud_hovered_control = g_windows[i].id;
        break;
      }
    }
  }
}

extern void display_print(const char *s);

static bwe_error_t demo_app_launch_wrapper(uint32_t *out_win) {
  extern void BWE_DemoApp_Initialize(void);
  BWE_DemoApp_Initialize();
  if (out_win)
    *out_win = 1; // Standard demo window ID is 1
  return BWE_SUCCESS;
}

// Desktop Shell Main Initializer
bwe_error_t Desktop_Shell_Initialize(void) {
  display_print("[SHELL] Starting ATOMS OS Native Workspace Shell...\n");

  memset(s_notifications, 0, sizeof(s_notifications));

  // Hook Desktop Window render and event callbacks
  BWE_Window *desktop = BWE_GetWindow(BWE_DESKTOP_ID);
  if (desktop) {
    desktop->on_render = desktop_paint_handler;
    desktop->on_event = desktop_event_handler;
  }

  horse_init();
  wallpaper_manager_init();

  // Initialize BOASSET engine & preload desktop icon assets into master texture
  // atlas
  BOAsset_Initialize();
  BOAsset_PreloadCritical();

  // Initialize BOMATRIX Layout Authority Engine
  bomatrix_init();

  g_hud_desktop_icons = 0;

  // Create Desktop Icons via BOMATRIX Matrix Layout Engine
  create_desktop_icon("File Explorer", APP_ID_EXPLORER, -1, -1);
  create_desktop_icon("Terminal", APP_ID_TERMINAL, -1, -1);
  create_desktop_icon("Settings", APP_ID_SETTINGS, -1, -1);
  create_desktop_icon("Calculator", APP_ID_CALCULATOR, -1, -1);
  create_desktop_icon("Stress Test", APP_ID_STRESS_TEST, -1, -1);
  create_desktop_icon("Music Player", APP_ID_MUSIC, -1, -1);
  create_desktop_icon("DOOM", APP_ID_DOOM, -1, -1);
  create_desktop_icon("Input Lab", APP_ID_INPUT_LAB, -1, -1);
  create_desktop_icon("ATRIX Browser", APP_ID_ATRIX, -1, -1);
  create_desktop_icon("3D Benchmark", APP_ID_GRAPH_3D, -1, -1);
  create_desktop_icon("Task Manager", APP_ID_TMH, -1, -1);

  // Initialize UI
  TaskPanel_Initialize();
  StartMenu_Initialize();
  SystemHub_Initialize();

  Shell_ShowNotification("Welcome", "ATOMS OS Workspace V2.0 Ready!", 5000);
  // horse_launch(APP_ID_DOOM);
  return BWE_SUCCESS;
}
