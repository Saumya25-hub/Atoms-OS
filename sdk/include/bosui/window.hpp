#ifndef BOSUI_WINDOW_HPP
#define BOSUI_WINDOW_HPP

#include "framework/include/bos_ui_controls.h"

namespace bosui {

class Window {
public:
    Window(const char* title, uint32_t width = 640, uint32_t height = 480, int32_t x = 100, int32_t y = 100) {
        m_win = BOS_Window_Create(x, y, width, height, title);
    }

    void SetContent(BOS_UIElement* element) {
        if (m_win) {
            BOS_Window_SetContent(m_win, element);
        }
    }

    void Show() {
        if (m_win) {
            BOS_Window_Show(m_win);
        }
    }

    BOS_Window* GetNativeWindow() const { return m_win; }

private:
    BOS_Window* m_win{nullptr};
};

} // namespace bosui

#endif /* BOSUI_WINDOW_HPP */
