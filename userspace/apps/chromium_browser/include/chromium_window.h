/*
 * =====================================================================
 * ATOMS OS — CHROMIUM DESKTOP WINDOW & CHROME VIEWS COMPOSITOR
 * =====================================================================
 * Handles Titlebar, Native TabStrip, Omnibox, Navigation Buttons,
 * Bookmark Bar, and Blink WebContents viewport rendering.
 * =====================================================================
 */

#ifndef USERSPACE_APPS_CHROMIUM_BROWSER_CHROMIUM_WINDOW_H_
#define USERSPACE_APPS_CHROMIUM_BROWSER_CHROMIUM_WINDOW_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "atoms/userspace/apal/include/apal.h"

namespace chromium {

struct Rect {
    int32_t x, y, w, h;
    bool Contains(int32_t px, int32_t py) const {
        return px >= x && px < (x + w) && py >= y && py < (y + h);
    }
};

class ChromiumWindow {
public:
    ChromiumWindow(int width = 1200, int height = 800);
    ~ChromiumWindow();

    bool Create();
    void Destroy();
    void Invalidate();
    void Present();

    // Layout Calculations
    void UpdateLayout(int width, int height);

    // Painting Pipeline (Aura / Views Style)
    void Paint();
    void PaintTitleBar(uint32_t* fb, uint32_t stride);
    void PaintTabStrip(uint32_t* fb, uint32_t stride);
    void PaintToolbar(uint32_t* fb, uint32_t stride);
    void PaintOmnibox(uint32_t* fb, uint32_t stride);
    void PaintBookmarkBar(uint32_t* fb, uint32_t stride);
    void PaintWebContents(uint32_t* fb, uint32_t stride);
    void PaintDownloadShelf(uint32_t* fb, uint32_t stride);

    // User Input
    bool HandleMouseClick(int32_t x, int32_t y, uint32_t button);
    bool HandleKeyInput(uint32_t key_code, char ascii_char, uint32_t modifiers);

    // Active Tab & URL Bindings
    void SetURL(const char* url);
    void SetPageTitle(const char* title);
    void SetLoadingProgress(float progress);
    void SetSecurityState(bool is_https);

    uint32_t GetWindowId() const { return surface_.window_id; }
    uint32_t* GetSurfaceBuffer() const { return surface_.pixel_buffer; }

private:
    int width_;
    int height_;
    apal_surface_t surface_;

    // Layout Regions (Aura Views Geometry)
    Rect rect_titlebar_;
    Rect rect_tabstrip_;
    Rect rect_toolbar_;
    Rect rect_btn_back_;
    Rect rect_btn_forward_;
    Rect rect_btn_reload_;
    Rect rect_btn_home_;
    Rect rect_omnibox_;
    Rect rect_bookmarks_;
    Rect rect_web_contents_;
    Rect rect_download_shelf_;

    // State
    char current_url_[256];
    char page_title_[256];
    char omnibox_input_[256];
    int omnibox_len_;
    bool omnibox_focused_;
    bool is_loading_;
    bool is_secure_https_;
    float load_progress_;
};

} // namespace chromium

#endif // USERSPACE_APPS_CHROMIUM_BROWSER_CHROMIUM_WINDOW_H_
