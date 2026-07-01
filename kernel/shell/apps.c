#include "desktop_shell.h"
#include "../lib/include/string.h"
#include "../memory/heap/include/heap.h"
#include "../vfs/include/vfs.h"

// HUD variables
extern bool g_hud_visible;

// Settings structure
typedef struct {
    uint32_t win_id;
    uint32_t right_panel_id;
} SettingsCtx;
static SettingsCtx s_settings;

// Explorer structure
typedef struct {
    uint32_t win_id;
    uint32_t toolbar_id;
    uint32_t pathbar_id;
    uint32_t list_panel_id;
    uint32_t status_id;
    char current_path[256];
} ExplorerCtx;
static ExplorerCtx s_explorer;

// Terminal structure
typedef struct {
    uint32_t win_id;
    uint32_t canvas_id;
    uint32_t textbox_id;
} TerminalCtx;
static TerminalCtx s_terminal;

#define MAX_TERM_LINES 15
static char s_terminal_lines[MAX_TERM_LINES][64];
static uint32_t s_terminal_line_count = 0;

// Calculator structure
typedef struct {
    uint32_t win_id;
    uint32_t display_id;
    char display_text[32];
    int32_t value1;
    char op;
    bool new_input;
} CalculatorCtx;
static CalculatorCtx s_calculator;

// ============================================================
// File Explorer Implementation
// ============================================================
static void explorer_load_directory(const char* path);

static void btn_up_clicked(uint32_t btn_id) {
    (void)btn_id;
    if (strcmp(s_explorer.current_path, "/") == 0) return;
    
    int last_slash = -1;
    for (int i = 0; s_explorer.current_path[i] != '\0'; i++) {
        if (s_explorer.current_path[i] == '/') last_slash = i;
    }
    
    if (last_slash > 0) {
        s_explorer.current_path[last_slash] = '\0';
    } else if (last_slash == 0) {
        s_explorer.current_path[1] = '\0'; // root
    }
    explorer_load_directory(s_explorer.current_path);
}

static void btn_refresh_clicked(uint32_t btn_id) {
    (void)btn_id;
    explorer_load_directory(s_explorer.current_path);
}

static void file_item_clicked(uint32_t btn_id) {
    BWE_Window* btn = BWE_GetWindow(btn_id);
    if (!btn) return;
    
    const char* filename = btn->control_data.button.text;
    uint32_t bg = btn->control_data.button.bg_color;
    
    char full_path[256];
    strcpy(full_path, s_explorer.current_path);
    int len = strlen(full_path);
    if (full_path[len - 1] != '/') {
        full_path[len] = '/';
        full_path[len + 1] = '\0';
    }
    strcat(full_path, filename);
    
    if (bg == 0xFFDBEAFE) { // Folder color indicator
        explorer_load_directory(full_path);
    } else {
        // Mock file execution / opening
        char msg[256];
        strcpy(msg, "Open file: ");
        strcat(msg, filename);
        Shell_ShowNotification("File Manager", msg, 4000);
    }
}

static void strcat_itoa(uint32_t val, char* buf) {
    char temp[16];
    int i = 0;
    if (val == 0) { strcat(buf, "0"); return; }
    while (val > 0) {
        temp[i++] = (val % 10) + '0';
        val /= 10;
    }
    char rev[16];
    int r = 0;
    while (i > 0) rev[r++] = temp[--i];
    rev[r] = '\0';
    strcat(buf, rev);
}

static void explorer_load_directory(const char* path) {
    strcpy(s_explorer.current_path, path);
    
    // Update pathbar textbox value
    BWE_Window* tb = BWE_GetWindow(s_explorer.pathbar_id);
    if (tb) {
        strcpy(tb->control_data.textbox.text, path);
        BWE_InvalidateWindow(s_explorer.pathbar_id);
    }
    
    // Clear items in middle panel by destroying and recreating it
    BOS_DestroySurface(s_explorer.list_panel_id);
    BOS_CreatePanel(s_explorer.win_id, 0, 70, 600, 310, 0xFFFFFFFF, &s_explorer.list_panel_id);
    
    int index = 0;
    vfs_dirent_t entry;
    
    uint32_t x_offset = 10;
    uint32_t y_offset = 10;
    uint32_t item_count = 0;
    
    extern int vfs_readdir(const char* path, int index, vfs_dirent_t* entry);
    while (vfs_readdir(path, index, &entry) == 0) {
        if (strlen(entry.name) > 0) {
            uint32_t item_id;
            uint32_t bg_color = entry.is_directory ? 0xFFDBEAFE : 0xFFF1F5F9; // folders blue, files gray
            
            BOS_CreateButton(s_explorer.list_panel_id, x_offset, y_offset, 100, 40, entry.name, file_item_clicked, &item_id);
            BWE_Window* btn = BWE_GetWindow(item_id);
            if (btn) {
                btn->control_data.button.bg_color = bg_color;
                btn->control_data.button.text_color = 0xFF0F172A; // Dark text
            }
            
            x_offset += 115;
            if (x_offset > 480) {
                x_offset = 10;
                y_offset += 50;
            }
            item_count++;
        }
        index++;
    }
    
    BWE_Window* status_lbl = BWE_GetWindow(s_explorer.status_id);
    if (status_lbl) {
        char status[64];
        strcpy(status, "Items: ");
        strcat_itoa(item_count, status);
        strcpy(status_lbl->control_data.label.text, status);
        BWE_InvalidateWindow(s_explorer.status_id);
    }
    
    BWE_InvalidateWindow(s_explorer.win_id);
}

bwe_error_t explorer_init_v2(uint32_t* out_win) {
    bwe_error_t err = BOS_CreateWindow(100, 100, 600, 400, "File Explorer", &s_explorer.win_id);
    if (err != BWE_SUCCESS) return err;
    
    // Toolbar Panel (Top)
    BOS_CreatePanel(s_explorer.win_id, 0, 30, 600, 40, 0xFFF8FAFC, &s_explorer.toolbar_id);
    
    // Navigation Buttons
    uint32_t btn_up, btn_ref;
    BOS_CreateButton(s_explorer.toolbar_id, 10, 5, 40, 30, "Up", btn_up_clicked, &btn_up);
    BOS_CreateButton(s_explorer.toolbar_id, 60, 5, 80, 30, "Refresh", btn_refresh_clicked, &btn_ref);
    
    // Path Textbox
    BOS_CreateTextbox(s_explorer.toolbar_id, 150, 5, 430, 30, "/", &s_explorer.pathbar_id);
    
    // Status Bar Panel (Bottom)
    uint32_t status_panel;
    BOS_CreatePanel(s_explorer.win_id, 0, 380, 600, 20, 0xFFE2E8F0, &status_panel);
    BOS_CreateLabel(status_panel, 10, 2, "Items: 0", 0xFF334155, &s_explorer.status_id);
    
    // Middle panel placeholder
    BOS_CreatePanel(s_explorer.win_id, 0, 70, 600, 310, 0xFFFFFFFF, &s_explorer.list_panel_id);
    
    explorer_load_directory("/");
    
    if (out_win) *out_win = s_explorer.win_id;
    return BWE_SUCCESS;
}

// ============================================================
// Terminal Implementation
// ============================================================
static void terminal_add_line(const char* line) {
    if (s_terminal_line_count < MAX_TERM_LINES) {
        strcpy(s_terminal_lines[s_terminal_line_count++], line);
    } else {
        for (int i = 0; i < MAX_TERM_LINES - 1; i++) {
            strcpy(s_terminal_lines[i], s_terminal_lines[i + 1]);
        }
        strcpy(s_terminal_lines[MAX_TERM_LINES - 1], line);
    }
}

static void terminal_paint_callback(uint32_t canvas_id, const BVFramebuffer* fb, const BWE_Rect* clip) {
    (void)clip;
    BWE_Window* self = BWE_GetWindow(canvas_id);
    if (!self) return;
    
    BWE_Rect b = self->screen_bounds;
    BWE_FillRect(fb, b.x, b.y, b.width, b.height, 0xFF000000); // Black CLI
    
    for (uint32_t i = 0; i < s_terminal_line_count; i++) {
        BWE_DrawText(fb, s_terminal_lines[i], b.x + 10, b.y + 10 + i * 18, 0xFF10B981, 0); // Green monospace-styled text
    }
}

static void (*s_original_textbox_on_event)(uint32_t, const BWE_Event*) = 0;

static void terminal_textbox_event_callback(uint32_t window_id, const BWE_Event* event) {
    BWE_Window* self = BWE_GetWindow(window_id);
    if (!self) return;
    
    // Call default textbox event handler first to capture keystrokes
    if (s_original_textbox_on_event) {
        s_original_textbox_on_event(window_id, event);
    }
    
    if (event->type == BWE_EVENT_KEY_DOWN) {
        uint32_t kc = event->data.key.key_code;
        if (kc == 0x0A || kc == 0x0D) { // Enter Key pressed
            char cmd[128];
            strcpy(cmd, self->control_data.textbox.text);
            
            // Clean command input
            self->control_data.textbox.text[0] = '\0';
            BWE_InvalidateWindow(window_id);
            
            char echo[140];
            strcpy(echo, "root@atoms:~# ");
            strcat(echo, cmd);
            terminal_add_line(echo);
            
            // Command Routing
            if (strcmp(cmd, "help") == 0) {
                terminal_add_line("Available commands: help, ls, neofetch, clear, exit");
            } else if (strcmp(cmd, "ls") == 0) {
                terminal_add_line("Listing directory / :");
                vfs_dirent_t entry;
                int index = 0;
                while (vfs_readdir("/", index++, &entry) == 0) {
                    if (strlen(entry.name) > 0) {
                        char dir_line[128];
                        strcpy(dir_line, entry.is_directory ? "[DIR]  " : "[FILE] ");
                        strcat(dir_line, entry.name);
                        terminal_add_line(dir_line);
                    }
                }
            } else if (strcmp(cmd, "neofetch") == 0) {
                terminal_add_line("      __ _|_  _  ._ _   _ ");
                terminal_add_line("     (_|  |_ (_) | | | _> ");
                terminal_add_line("---------------------------");
                terminal_add_line("OS: ATOMS OS v2.0");
                terminal_add_line("Kernel: BWE Compositor Core");
                terminal_add_line("Memory: 512 MB physical");
                terminal_add_line("Display: 1280x720 VBE v3.0");
            } else if (strcmp(cmd, "clear") == 0) {
                s_terminal_line_count = 0;
            } else if (strcmp(cmd, "exit") == 0) {
                BOS_DestroySurface(s_terminal.win_id);
                extern void taskbar_update_windows_list(void);
                taskbar_update_windows_list();
                return;
            } else {
                char err_line[128];
                strcpy(err_line, "Terminal: Command not found: ");
                strcat(err_line, cmd);
                terminal_add_line(err_line);
            }
            BWE_InvalidateWindow(s_terminal.canvas_id);
        }
    }
}

bwe_error_t terminal_init_v2(uint32_t* out_win) {
    bwe_error_t err = BOS_CreateWindow(150, 120, 500, 360, "Interactive Terminal", &s_terminal.win_id);
    if (err != BWE_SUCCESS) return err;
    
    // Text output canvas
    BOS_CreateCanvas(s_terminal.win_id, 0, 30, 500, 300, terminal_paint_callback, &s_terminal.canvas_id);
    
    // Command input textbox
    BOS_CreateTextbox(s_terminal.win_id, 0, 330, 500, 30, "Type help for list of commands...", &s_terminal.textbox_id);
    
    // Override event callback of the textbox to intercept Enter
    BWE_Window* tb = BWE_GetWindow(s_terminal.textbox_id);
    if (tb) {
        s_original_textbox_on_event = tb->on_event;
        tb->on_event = terminal_textbox_event_callback;
    }
    
    // Initial welcome lines
    s_terminal_line_count = 0;
    terminal_add_line("ATOMS OS Terminal session active");
    terminal_add_line("Type 'help' for shell command reference.");
    terminal_add_line("");
    
    if (out_win) *out_win = s_terminal.win_id;
    return BWE_SUCCESS;
}

// ============================================================
// Settings Panel Implementation
// ============================================================
static void load_settings_tab(const char* category);

static void btn_category_display_clicked(uint32_t btn_id) {
    (void)btn_id;
    load_settings_tab("Display");
}
static void btn_category_theme_clicked(uint32_t btn_id) {
    (void)btn_id;
    load_settings_tab("Theme");
}
static void btn_category_system_clicked(uint32_t btn_id) {
    (void)btn_id;
    load_settings_tab("System Info");
}

static void chk_hud_toggled(uint32_t chk_id, bool is_checked) {
    (void)chk_id;
    g_hud_visible = is_checked;
    BWE_InvalidateWindow(BWE_DESKTOP_ID);
}

static void btn_theme_toggle_clicked(uint32_t btn_id) {
    (void)btn_id;
    static bool is_dark = true;
    is_dark = !is_dark;
    
    BWE_ThemeSetDark(is_dark);
    
    // Invalidate all windows to redraw
    for (uint32_t i = 0; i < BWE_MAX_WINDOWS; i++) {
        BWE_InvalidateWindow(i);
    }
    Shell_ShowNotification("Theme Engine", is_dark ? "Switched to Dark Mode" : "Switched to Light Mode", 3000);
}

static void load_settings_tab(const char* category) {
    BOS_DestroySurface(s_settings.right_panel_id);
    BOS_CreatePanel(s_settings.win_id, 140, 30, 380, 330, 0xFFF1F5F9, &s_settings.right_panel_id);
    
    BWE_Window* panel = BWE_GetWindow(s_settings.right_panel_id);
    if (panel) {
        panel->padding.left = 15;
        panel->padding.top = 15;
    }
    
    uint32_t dummy;
    if (strcmp(category, "Display") == 0) {
        BOS_CreateLabel(s_settings.right_panel_id, 15, 15, "Display Driver Configuration", 0xFF0F172A, &dummy);
        BOS_CreateLabel(s_settings.right_panel_id, 15, 45, "Active Resolution: 1280x720", 0xFF475569, &dummy);
        BOS_CreateLabel(s_settings.right_panel_id, 15, 70, "Color Format: 32-bit ARGB", 0xFF475569, &dummy);
        
        uint32_t chk_id;
        BOS_CreateCheckbox(s_settings.right_panel_id, 15, 110, 200, 30, "Show Performance HUD", chk_hud_toggled, &chk_id);
        BWE_Window* chk = BWE_GetWindow(chk_id);
        if (chk) chk->control_data.checkbox.checked = g_hud_visible;
        
    } else if (strcmp(category, "Theme") == 0) {
        BOS_CreateLabel(s_settings.right_panel_id, 15, 15, "Workspace Customization", 0xFF0F172A, &dummy);
        BOS_CreateLabel(s_settings.right_panel_id, 15, 45, "Change standard UI window coloring theme:", 0xFF475569, &dummy);
        
        BOS_CreateButton(s_settings.right_panel_id, 15, 80, 180, 35, "Toggle Dark/Light Mode", btn_theme_toggle_clicked, &dummy);
        
    } else if (strcmp(category, "System Info") == 0) {
        BOS_CreateLabel(s_settings.right_panel_id, 15, 15, "ATOMS OS System Specifications", 0xFF0F172A, &dummy);
        BOS_CreateLabel(s_settings.right_panel_id, 15, 45, "Processor: x86_64 Core Preemptive", 0xFF475569, &dummy);
        BOS_CreateLabel(s_settings.right_panel_id, 15, 70, "Memory RAM: 512 Megabytes", 0xFF475569, &dummy);
        BOS_CreateLabel(s_settings.right_panel_id, 15, 95, "Version: BWE V2.0 Shell Phase 4", 0xFF475569, &dummy);
        
        BOS_CreateLabel(s_settings.right_panel_id, 15, 140, "Diagnostic telemetry statistics are", 0xFF94A3B8, &dummy);
        BOS_CreateLabel(s_settings.right_panel_id, 15, 160, "live on the performance HUD overlays.", 0xFF94A3B8, &dummy);
    }
    
    BWE_InvalidateWindow(s_settings.win_id);
}

bwe_error_t settings_init_v2(uint32_t* out_win) {
    bwe_error_t err = BOS_CreateWindow(200, 150, 520, 360, "Settings Control", &s_settings.win_id);
    if (err != BWE_SUCCESS) return err;
    
    // Left sidebar categories panel
    uint32_t sidebar_id;
    BOS_CreatePanel(s_settings.win_id, 0, 30, 140, 330, 0xFFE2E8F0, &sidebar_id);
    
    uint32_t dummy;
    BOS_CreateButton(sidebar_id, 10, 10, 120, 35, "Display Settings", btn_category_display_clicked, &dummy);
    BOS_CreateButton(sidebar_id, 10, 55, 120, 35, "Theme Style", btn_category_theme_clicked, &dummy);
    BOS_CreateButton(sidebar_id, 10, 100, 120, 35, "System Info", btn_category_system_clicked, &dummy);
    
    // Right panel content space
    BOS_CreatePanel(s_settings.win_id, 140, 30, 380, 330, 0xFFF1F5F9, &s_settings.right_panel_id);
    
    load_settings_tab("Display");
    
    if (out_win) *out_win = s_settings.win_id;
    return BWE_SUCCESS;
}

static int shell_atoi(const char* s) {
    int res = 0;
    int sign = 1;
    int i = 0;
    if (s[0] == '-') {
        sign = -1;
        i++;
    }
    for (; s[i] != '\0'; i++) {
        if (s[i] >= '0' && s[i] <= '9') {
            res = res * 10 + (s[i] - '0');
        } else {
            break;
        }
    }
    return sign * res;
}

// ============================================================
// Calculator Implementation
// ============================================================
static void update_calc_display(void) {
    BWE_Window* lbl = BWE_GetWindow(s_calculator.display_id);
    if (lbl) {
        strcpy(lbl->control_data.label.text, s_calculator.display_text);
        BWE_InvalidateWindow(s_calculator.display_id);
    }
}

static void calc_btn_clicked(uint32_t btn_id) {
    BWE_Window* btn = BWE_GetWindow(btn_id);
    if (!btn) return;
    
    char key = btn->control_data.button.text[0];
    
    // If it's a number key
    if (key >= '0' && key <= '9') {
        if (s_calculator.new_input) {
            s_calculator.display_text[0] = '\0';
            s_calculator.new_input = false;
        }
        int len = strlen(s_calculator.display_text);
        if (len < 15) {
            s_calculator.display_text[len] = key;
            s_calculator.display_text[len + 1] = '\0';
        }
        update_calc_display();
    } else if (key == 'C') {
        s_calculator.display_text[0] = '0';
        s_calculator.display_text[1] = '\0';
        s_calculator.value1 = 0;
        s_calculator.op = '\0';
        s_calculator.new_input = true;
        update_calc_display();
    } else if (key == '+' || key == '-' || key == '*' || key == '/') {
        // Parse current value
        s_calculator.value1 = shell_atoi(s_calculator.display_text);
        s_calculator.op = key;
        s_calculator.new_input = true;
    } else if (key == '=') {
        if (s_calculator.op != '\0') {
            int32_t val2 = shell_atoi(s_calculator.display_text);
            int32_t result = 0;
            if (s_calculator.op == '+') result = s_calculator.value1 + val2;
            else if (s_calculator.op == '-') result = s_calculator.value1 - val2;
            else if (s_calculator.op == '*') result = s_calculator.value1 * val2;
            else if (s_calculator.op == '/') {
                if (val2 != 0) result = s_calculator.value1 / val2;
                else result = 0;
            }
            
            // Format result back to text
            s_calculator.display_text[0] = '\0';
            strcat_itoa(result, s_calculator.display_text);
            update_calc_display();
            s_calculator.op = '\0';
            s_calculator.new_input = true;
        }
    }
}

bwe_error_t calculator_init_v2(uint32_t* out_win) {
    bwe_error_t err = BOS_CreateWindow(300, 100, 240, 320, "Calculator Grid", &s_calculator.win_id);
    if (err != BWE_SUCCESS) return err;
    
    // Display screen Panel
    uint32_t scr_panel;
    BOS_CreatePanel(s_calculator.win_id, 10, 40, 220, 40, 0xFFE2E8F0, &scr_panel);
    BOS_CreateLabel(scr_panel, 10, 12, "0", 0xFF0F172A, &s_calculator.display_id);
    
    strcpy(s_calculator.display_text, "0");
    s_calculator.value1 = 0;
    s_calculator.op = '\0';
    s_calculator.new_input = true;
    
    // Buttons grid
    const char* keys[16] = {
        "7", "8", "9", "/",
        "4", "5", "6", "*",
        "1", "2", "3", "-",
        "C", "0", "=", "+"
    };
    
    uint32_t dummy;
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            BOS_CreateButton(s_calculator.win_id, 10 + c * 55, 90 + r * 55, 50, 50, keys[r * 4 + c], calc_btn_clicked, &dummy);
            BWE_Window* btn = BWE_GetWindow(dummy);
            if (btn) {
                btn->control_data.button.bg_color = 0xFFF1F5F9;
                btn->control_data.button.text_color = 0xFF0F172A;
            }
        }
    }
    
    if (out_win) *out_win = s_calculator.win_id;
    return BWE_SUCCESS;
}
