/*
 * =====================================================================
 * ATOMS OS — FULL CHROMIUM DESKTOP GUI APPLICATION
 * =====================================================================
 * Native Desktop Window, TabStrip, Omnibox, and WebContents Host
 * Powered by Google Chromium (net/, base/, mojo/) + Blink + Skia + APAL
 * Copyright (C) 2026 ATOMS OS Project / Chromium Authors
 * =====================================================================
 */

#ifndef USERSPACE_APPS_CHROMIUM_BROWSER_BROWSER_APP_H_
#define USERSPACE_APPS_CHROMIUM_BROWSER_BROWSER_APP_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Public C ABI for OS / Shell launch
int chromium_browser_main(int argc, char** argv);
bool chromium_browser_launch_desktop(const char* initial_url);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <string>
#include <vector>
#include <memory>

namespace chromium {

enum class BrowserCommand {
    BACK,
    FORWARD,
    RELOAD,
    STOP,
    HOME,
    NEW_TAB,
    CLOSE_TAB,
    BOOKMARKS,
    SETTINGS,
    DEVTOOLS
};

struct TabInfo {
    uint32_t id;
    std::string title;
    std::string url;
    bool is_loading;
    bool is_audio_playing;
    bool is_secure;
};

class BrowserApp {
public:
    static BrowserApp& GetInstance();

    bool Initialize(int width, int height);
    void Run();
    void Shutdown();

    // Navigation & Tab Control
    void Navigate(const std::string& url);
    void CreateNewTab(const std::string& url = "chrome://newtab");
    void CloseTab(uint32_t tab_id);
    void SelectTab(uint32_t tab_id);

    // Command Dispatch
    void ExecuteCommand(BrowserCommand cmd);

    // Event Handling
    void OnMouseMove(int32_t x, int32_t y);
    void OnMouseDown(int32_t x, int32_t y, uint32_t button);
    void OnMouseUp(int32_t x, int32_t y, uint32_t button);
    void OnKeyDown(uint32_t key_code, char ascii_char, uint32_t modifiers);

    bool IsRunning() const { return is_running_; }

private:
    BrowserApp();
    ~BrowserApp();

    bool is_running_;
    int window_width_;
    int window_height_;
    uint32_t active_tab_id_;
    std::vector<TabInfo> tabs_;
};

} // namespace chromium
#endif // __cplusplus

#endif // USERSPACE_APPS_CHROMIUM_BROWSER_BROWSER_APP_H_
