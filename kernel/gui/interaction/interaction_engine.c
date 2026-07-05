#include "interaction_engine.h"
#include "kernel/gui/shell/desktop_shell.h"
#include <stddef.h>

static InputState last_state;
static struct BOSWindow* dragged_window = NULL;
static struct BOSWindow* resizing_window = NULL;
static struct BOSWindow* hovered_window = NULL;
static WindowHitTestResult resize_hit_mode = HIT_OUTSIDE;

static int drag_offset_x = 0;
static int drag_offset_y = 0;

void interaction_engine_init(void) {
    last_state.mouse_x = 0;
    last_state.mouse_y = 0;
    last_state.buttons = 0;
    last_state.scroll = 0;
}

static void handle_mouse_down(InputState state) {
    if (desktop_shell_hit_test(state.mouse_x, state.mouse_y, state.buttons)) {
        return; // Handled by shell
    }

    struct BOSWindow* win = window_get_at_point(state.mouse_x, state.mouse_y);
    if (!win) return;
    
    // Bring to front and focus
    window_focus(win);
    
    WindowHitTestResult hit = window_hit_test(win, state.mouse_x, state.mouse_y);
    
    if (hit == HIT_TITLEBAR) {
        dragged_window = win;
        drag_offset_x = state.mouse_x - win->surface->x;
        drag_offset_y = state.mouse_y - win->surface->y;
    } 
    else if (hit == HIT_CLOSE_BUTTON) {
        // Handled on mouse up, but we could add visual pressed state
    }
    else if (hit >= HIT_BORDER_LEFT && hit <= HIT_BORDER_BOTTOMRIGHT) {
        resizing_window = win;
        resize_hit_mode = hit;
        // In a real implementation, calculate resize offsets here
    }
    else if (hit == HIT_CLIENT) {
        GUIEvent ev;
        ev.type = GUI_EVENT_MOUSE_DOWN;
        ev.target_surface_id = win->surface->unique_id;
        ev.data.mouse.x = state.mouse_x - win->surface->x;
        ev.data.mouse.y = state.mouse_y - win->surface->y;
        ev.data.mouse.buttons = state.buttons;
        window_post_event(win, &ev);
    }
}

static void handle_mouse_up(InputState state) {
    if (desktop_shell_hit_test(state.mouse_x, state.mouse_y, state.buttons)) {
        // Technically mouse up handling should route too if captured, but we'll re-hit test for now
    }

    if (dragged_window) {
        dragged_window = NULL;
    }
    if (resizing_window) {
        resizing_window = NULL;
    }
    
    struct BOSWindow* win = window_get_at_point(state.mouse_x, state.mouse_y);
    if (win) {
        WindowHitTestResult hit = window_hit_test(win, state.mouse_x, state.mouse_y);
        if (hit == HIT_CLOSE_BUTTON) {
            window_close(win);
        } else if (hit == HIT_CLIENT) {
            GUIEvent ev;
            ev.type = GUI_EVENT_MOUSE_UP;
            ev.target_surface_id = win->surface->unique_id;
            ev.data.mouse.x = state.mouse_x - win->surface->x;
            ev.data.mouse.y = state.mouse_y - win->surface->y;
            ev.data.mouse.buttons = state.buttons;
            window_post_event(win, &ev);
        }
    }
}

static void handle_mouse_move(InputState state) {
    if (desktop_shell_hit_test(state.mouse_x, state.mouse_y, state.buttons)) {
        // Shell handles hover updates during hit test implicitly for now
    }

    if (dragged_window) {
        // Dragging mode
        int new_x = state.mouse_x - drag_offset_x;
        int new_y = state.mouse_y - drag_offset_y;
        window_move(dragged_window, new_x, new_y);
        return;
    }
    
    if (resizing_window) {
        // Simplified resize logic (only bottom-right for this example)
        if (resize_hit_mode == HIT_BORDER_BOTTOMRIGHT) {
            int new_w = state.mouse_x - resizing_window->surface->x;
            int new_h = state.mouse_y - resizing_window->surface->y;
            window_resize(resizing_window, new_w, new_h);
        }
        return;
    }

    struct BOSWindow* win = window_get_at_point(state.mouse_x, state.mouse_y);
    
    if (hovered_window != win) {
        if (hovered_window) {
            GUIEvent ev;
            ev.type = GUI_EVENT_MOUSE_LEAVE;
            ev.target_surface_id = hovered_window->surface->unique_id;
            window_post_event(hovered_window, &ev);
        }
        if (win) {
            GUIEvent ev;
            ev.type = GUI_EVENT_MOUSE_ENTER;
            ev.target_surface_id = win->surface->unique_id;
            window_post_event(win, &ev);
        }
        hovered_window = win;
    }

    if (win) {
        WindowHitTestResult hit = window_hit_test(win, state.mouse_x, state.mouse_y);
        if (hit == HIT_CLIENT) {
            GUIEvent ev;
            ev.type = GUI_EVENT_MOUSE_MOVE;
            ev.target_surface_id = win->surface->unique_id;
            ev.data.mouse.x = state.mouse_x - win->surface->x;
            ev.data.mouse.y = state.mouse_y - win->surface->y;
            ev.data.mouse.buttons = state.buttons;
            window_post_event(win, &ev);
        }
    }
}

void interaction_engine_update(InputState state) {
    bool left_pressed = (state.buttons & 1) && !(last_state.buttons & 1);
    bool left_released = !(state.buttons & 1) && (last_state.buttons & 1);
    bool moved = (state.mouse_x != last_state.mouse_x || state.mouse_y != last_state.mouse_y);

    if (left_pressed) {
        handle_mouse_down(state);
    } else if (left_released) {
        handle_mouse_up(state);
    }
    
    if (moved) {
        handle_mouse_move(state);
    }

    last_state = state;
}

void interaction_engine_release_capture(void) {
    dragged_window = NULL;
    resizing_window = NULL;
}
