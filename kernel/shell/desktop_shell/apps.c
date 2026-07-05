#include "desktop_shell.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/audio/audio_player.h"

// HUD variables
extern bool g_hud_visible;

// Helper to find the top-level parent window's user_data context
static void* get_top_parent_ctx(uint32_t win_id) {
    BWE_Window* curr = BWE_GetWindow(win_id);
    while (curr && curr->parent_id != BWE_DESKTOP_ID && curr->parent_id != curr->id) {
        curr = BWE_GetWindow(curr->parent_id);
    }
    return curr ? curr->user_data : 0;
}

// Helper to format integers to buffer
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

// Helper to parse integers
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

// Removed old explorer

// ============================================================
// Interactive Terminal Implementation
// ============================================================
#define MAX_TERM_LINES 15
typedef struct {
    uint32_t win_id;
    uint32_t canvas_id;
    uint32_t textbox_id;
    void (*original_textbox_on_event)(uint32_t, const BWE_Event*);
    char lines[MAX_TERM_LINES][64];
    uint32_t line_count;
} TerminalCtx;

static void terminal_add_line(TerminalCtx* ctx, const char* line) {
    if (ctx->line_count < MAX_TERM_LINES) {
        strcpy(ctx->lines[ctx->line_count++], line);
    } else {
        for (int i = 0; i < MAX_TERM_LINES - 1; i++) {
            strcpy(ctx->lines[i], ctx->lines[i + 1]);
        }
        strcpy(ctx->lines[MAX_TERM_LINES - 1], line);
    }
}

static void terminal_paint_callback(uint32_t canvas_id, const BVFramebuffer* fb, const BWE_Rect* clip) {
    (void)clip;
    BWE_Window* self = BWE_GetWindow(canvas_id);
    if (!self) return;
    
    TerminalCtx* ctx = (TerminalCtx*)get_top_parent_ctx(canvas_id);
    if (!ctx) return;
    
    BWE_Rect b = self->screen_bounds;
    BWE_FillRect(fb, b.x, b.y, b.width, b.height, 0xFF000000); // Black CLI
    
    for (uint32_t i = 0; i < ctx->line_count; i++) {
        BWE_DrawText(fb, ctx->lines[i], b.x + 10, b.y + 10 + i * 18, 0xFF10B981, 0); // Green monospace-styled text
    }
}

static void terminal_textbox_event_callback(uint32_t window_id, const BWE_Event* event) {
    BWE_Window* self = BWE_GetWindow(window_id);
    if (!self) return;
    
    TerminalCtx* ctx = (TerminalCtx*)get_top_parent_ctx(window_id);
    if (!ctx) return;
    
    // Call default textbox event handler first to capture keystrokes
    if (ctx->original_textbox_on_event) {
        ctx->original_textbox_on_event(window_id, event);
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
            terminal_add_line(ctx, echo);
            
            // Command Routing
            if (strcmp(cmd, "help") == 0) {
                terminal_add_line(ctx, "Available commands: help, ls, neofetch, clear, exit");
            } else if (strcmp(cmd, "ls") == 0) {
                terminal_add_line(ctx, "Listing directory / :");
                vfs_dirent_t entry;
                int index = 0;
                while (vfs_readdir("/", index++, &entry) == 0) {
                    if (strlen(entry.name) > 0) {
                        char dir_line[128];
                        strcpy(dir_line, entry.is_directory ? "[DIR]  " : "[FILE] ");
                        strcat(dir_line, entry.name);
                        terminal_add_line(ctx, dir_line);
                    }
                }
            } else if (strcmp(cmd, "neofetch") == 0) {
                terminal_add_line(ctx, "      __ _|_  _  ._ _   _ ");
                terminal_add_line(ctx, "     (_|  |_ (_) | | | _> ");
                terminal_add_line(ctx, "---------------------------");
                terminal_add_line(ctx, "OS: ATOMS OS v2.0");
                terminal_add_line(ctx, "Kernel: BWE Compositor Core");
                terminal_add_line(ctx, "Memory: 512 MB physical");
                terminal_add_line(ctx, "Display: 1280x720 VBE v3.0");
            } else if (strcmp(cmd, "clear") == 0) {
                ctx->line_count = 0;
            } else if (strcmp(cmd, "exit") == 0) {
                BOS_DestroySurface(ctx->win_id);
                extern void TaskPanel_Update(void);
                TaskPanel_Update();
                return;
            } else {
                char err_line[128];
                strcpy(err_line, "Terminal: Command not found: ");
                strcat(err_line, cmd);
                terminal_add_line(ctx, err_line);
            }
            if (ctx->canvas_id != 0) {
                BWE_InvalidateWindow(ctx->canvas_id);
            }
        }
    }
}

bwe_error_t terminal_init_v2(uint32_t* out_win) {
    uint32_t win_id = 0;
    bwe_error_t err = BOS_CreateWindow(150, 120, 500, 360, "Interactive Terminal", &win_id);
    if (err != BWE_SUCCESS) return err;
    
    BWE_Window* win = BWE_GetWindow(win_id);
    if (!win) return BWE0002;
    
    extern void* kcalloc(size_t num, size_t size);
    TerminalCtx* ctx = (TerminalCtx*)kcalloc(1, sizeof(TerminalCtx));
    ctx->win_id = win_id;
    ctx->line_count = 0;
    win->user_data = ctx;
    
    // Text output canvas
    BOS_CreateCanvas(win_id, 0, 30, 500, 300, terminal_paint_callback, &ctx->canvas_id);
    
    // Command input textbox
    BOS_CreateTextbox(win_id, 0, 330, 500, 30, "Type help for list of commands...", &ctx->textbox_id);
    
    // Override event callback of the textbox to intercept Enter
    if (ctx->textbox_id != 0) {
        BWE_Window* tb = BWE_GetWindow(ctx->textbox_id);
        if (tb) {
            ctx->original_textbox_on_event = tb->on_event;
            tb->on_event = terminal_textbox_event_callback;
        }
    }
    
    // Initial welcome lines
    terminal_add_line(ctx, "ATOMS OS Terminal session active");
    terminal_add_line(ctx, "Type 'help' for shell command reference.");
    terminal_add_line(ctx, "");
    
    if (out_win) *out_win = win_id;
    return BWE_SUCCESS;
}

// ============================================================
// Settings Panel Implementation
// ============================================================
typedef struct {
    uint32_t win_id;
    uint32_t right_panel_id;
} SettingsCtx;

static void load_settings_tab(SettingsCtx* ctx, const char* category);

#include "kernel/media/bopawn/wallpaper/wallpaper_settings.h"

static void btn_category_display_clicked(uint32_t btn_id) {
    SettingsCtx* ctx = (SettingsCtx*)get_top_parent_ctx(btn_id);
    if (ctx) load_settings_tab(ctx, "Display");
}
static void btn_category_wallpaper_clicked(uint32_t btn_id) {
    SettingsCtx* ctx = (SettingsCtx*)get_top_parent_ctx(btn_id);
    if (ctx) load_settings_tab(ctx, "Wallpaper");
}
static void btn_category_theme_clicked(uint32_t btn_id) {
    SettingsCtx* ctx = (SettingsCtx*)get_top_parent_ctx(btn_id);
    if (ctx) load_settings_tab(ctx, "Theme");
}
static void btn_category_system_clicked(uint32_t btn_id) {
    SettingsCtx* ctx = (SettingsCtx*)get_top_parent_ctx(btn_id);
    if (ctx) load_settings_tab(ctx, "System Info");
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

static void load_settings_tab(SettingsCtx* ctx, const char* category) {
    if (ctx->right_panel_id != 0) {
        BOS_DestroySurface(ctx->right_panel_id);
    }
    ctx->right_panel_id = 0;
    
    BOS_CreatePanel(ctx->win_id, 140, 30, 380, 330, 0xFFF1F5F9, &ctx->right_panel_id);
    
    if (ctx->right_panel_id != 0) {
        BWE_Window* panel = BWE_GetWindow(ctx->right_panel_id);
        if (panel) {
            panel->padding.left = 15;
            panel->padding.top = 15;
        }
        
        uint32_t dummy = 0;
        if (strcmp(category, "Display") == 0) {
            BOS_CreateLabel(ctx->right_panel_id, 15, 15, "Display Driver Configuration", 0xFF0F172A, &dummy);
            BOS_CreateLabel(ctx->right_panel_id, 15, 45, "Active Resolution: 1280x720", 0xFF475569, &dummy);
            BOS_CreateLabel(ctx->right_panel_id, 15, 70, "Color Format: 32-bit ARGB", 0xFF475569, &dummy);
            
            uint32_t chk_id = 0;
            BOS_CreateCheckbox(ctx->right_panel_id, 15, 110, 200, 30, "Show Performance HUD", chk_hud_toggled, &chk_id);
            if (chk_id != 0) {
                BWE_Window* chk = BWE_GetWindow(chk_id);
                if (chk) chk->control_data.checkbox.checked = g_hud_visible;
            }
            
        } else if (strcmp(category, "Theme") == 0) {
            BOS_CreateLabel(ctx->right_panel_id, 15, 15, "Workspace Customization", 0xFF0F172A, &dummy);
            BOS_CreateLabel(ctx->right_panel_id, 15, 45, "Change standard UI window coloring theme:", 0xFF475569, &dummy);
            
            BOS_CreateButton(ctx->right_panel_id, 15, 80, 180, 35, "Toggle Dark/Light Mode", btn_theme_toggle_clicked, &dummy);
            
        } else if (strcmp(category, "System Info") == 0) {
            BOS_CreateLabel(ctx->right_panel_id, 15, 15, "ATOMS OS System Specifications", 0xFF0F172A, &dummy);
            BOS_CreateLabel(ctx->right_panel_id, 15, 45, "Processor: x86_64 Core Preemptive", 0xFF475569, &dummy);
            BOS_CreateLabel(ctx->right_panel_id, 15, 70, "Memory RAM: 512 Megabytes", 0xFF475569, &dummy);
            BOS_CreateLabel(ctx->right_panel_id, 15, 95, "Version: BWE V2.1 Shell Phase 4", 0xFF475569, &dummy);
            
            BOS_CreateLabel(ctx->right_panel_id, 15, 140, "Diagnostic telemetry statistics are", 0xFF94A3B8, &dummy);
            BOS_CreateLabel(ctx->right_panel_id, 15, 160, "live on the performance HUD overlays.", 0xFF94A3B8, &dummy);
        } else if (strcmp(category, "Wallpaper") == 0) {
            wallpaper_settings_render(ctx->right_panel_id);
        }
    }
    
    BWE_InvalidateWindow(ctx->win_id);
}

bwe_error_t settings_init_v2(uint32_t* out_win) {
    uint32_t win_id = 0;
    bwe_error_t err = BOS_CreateWindow(200, 150, 520, 360, "Settings Control", &win_id);
    if (err != BWE_SUCCESS) return err;
    
    BWE_Window* win = BWE_GetWindow(win_id);
    if (!win) return BWE0002;
    
    extern void* kcalloc(size_t num, size_t size);
    SettingsCtx* ctx = (SettingsCtx*)kcalloc(1, sizeof(SettingsCtx));
    ctx->win_id = win_id;
    win->user_data = ctx;
    
    // Left sidebar categories panel
    uint32_t sidebar_id = 0;
    BOS_CreatePanel(win_id, 0, 0, 140, 320, 0xFFE2E8F0, &sidebar_id);
    
    if (sidebar_id != 0) {
        uint32_t dummy = 0;
        BOS_CreateButton(sidebar_id, 10, 10, 120, 35, "Display Settings", btn_category_display_clicked, &dummy);
        BOS_CreateButton(sidebar_id, 10, 55, 120, 35, "Wallpaper", btn_category_wallpaper_clicked, &dummy);
        BOS_CreateButton(sidebar_id, 10, 100, 120, 35, "Theme Style", btn_category_theme_clicked, &dummy);
        BOS_CreateButton(sidebar_id, 10, 145, 120, 35, "System Info", btn_category_system_clicked, &dummy);
    }
    
    // Right panel content space
    BOS_CreatePanel(win_id, 140, 0, 380, 320, 0xFFF1F5F9, &ctx->right_panel_id);
    
    load_settings_tab(ctx, "Display");
    
    if (out_win) *out_win = win_id;
    return BWE_SUCCESS;
}

// ============================================================
// Calculator Implementation
// ============================================================
typedef struct {
    uint32_t win_id;
    uint32_t display_id;
    char display_text[32];
    int32_t value1;
    char op;
    bool new_input;
} CalculatorCtx;

static void update_calc_display(CalculatorCtx* ctx) {
    if (ctx->display_id != 0) {
        BWE_Window* lbl = BWE_GetWindow(ctx->display_id);
        if (lbl) {
            strcpy(lbl->control_data.label.text, ctx->display_text);
            BWE_InvalidateWindow(ctx->display_id);
        }
    }
}

static void calc_btn_clicked(uint32_t btn_id) {
    BWE_Window* btn = BWE_GetWindow(btn_id);
    if (!btn) return;
    
    CalculatorCtx* ctx = (CalculatorCtx*)get_top_parent_ctx(btn_id);
    if (!ctx) return;
    
    char key = btn->control_data.button.text[0];
    
    // If it's a number key
    if (key >= '0' && key <= '9') {
        if (ctx->new_input) {
            ctx->display_text[0] = '\0';
            ctx->new_input = false;
        }
        int len = strlen(ctx->display_text);
        if (len < 15) {
            ctx->display_text[len] = key;
            ctx->display_text[len + 1] = '\0';
        }
        update_calc_display(ctx);
    } else if (key == 'C') {
        ctx->display_text[0] = '0';
        ctx->display_text[1] = '\0';
        ctx->value1 = 0;
        ctx->op = '\0';
        ctx->new_input = true;
        update_calc_display(ctx);
    } else if (key == '+' || key == '-' || key == '*' || key == '/') {
        // Parse current value
        ctx->value1 = shell_atoi(ctx->display_text);
        ctx->op = key;
        ctx->new_input = true;
    } else if (key == '=') {
        if (ctx->op != '\0') {
            int32_t val2 = shell_atoi(ctx->display_text);
            int32_t result = 0;
            if (ctx->op == '+') result = ctx->value1 + val2;
            else if (ctx->op == '-') result = ctx->value1 - val2;
            else if (ctx->op == '*') result = ctx->value1 * val2;
            else if (ctx->op == '/') {
                if (val2 != 0) result = ctx->value1 / val2;
                else result = 0;
            }
            
            // Format result back to text
            ctx->display_text[0] = '\0';
            strcat_itoa(result, ctx->display_text);
            update_calc_display(ctx);
            ctx->op = '\0';
            ctx->new_input = true;
        }
    }
}

bwe_error_t calculator_init_v2(uint32_t* out_win) {
    uint32_t win_id = 0;
    bwe_error_t err = BOS_CreateWindow(300, 100, 240, 320, "Calculator Grid", &win_id);
    if (err != BWE_SUCCESS) return err;
    
    BWE_Window* win = BWE_GetWindow(win_id);
    if (!win) return BWE0002;
    
    extern void* kcalloc(size_t num, size_t size);
    CalculatorCtx* ctx = (CalculatorCtx*)kcalloc(1, sizeof(CalculatorCtx));
    ctx->win_id = win_id;
    win->user_data = ctx;
    
    // Display screen Panel
    uint32_t scr_panel = 0;
    BOS_CreatePanel(win_id, 10, 10, 220, 40, 0xFFE2E8F0, &scr_panel);
    if (scr_panel != 0) {
        BOS_CreateLabel(scr_panel, 10, 12, "0", 0xFF0F172A, &ctx->display_id);
    }
    
    strcpy(ctx->display_text, "0");
    ctx->value1 = 0;
    ctx->op = '\0';
    ctx->new_input = true;
    
    // Buttons grid
    const char* keys[16] = {
        "7", "8", "9", "/",
        "4", "5", "6", "*",
        "1", "2", "3", "-",
        "C", "0", "=", "+"
    };
    
    uint32_t dummy = 0;
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            BOS_CreateButton(win_id, 10 + c * 55, 60 + r * 55, 50, 50, keys[r * 4 + c], calc_btn_clicked, &dummy);
            if (dummy != 0) {
                BWE_Window* btn = BWE_GetWindow(dummy);
                if (btn) {
                    btn->control_data.button.bg_color = 0xFFF1F5F9;
                    btn->control_data.button.text_color = 0xFF0F172A;
                }
            }
        }
    }
    
    if (out_win) *out_win = win_id;
    return BWE_SUCCESS;
}

// ============================================================
// BWE Stress Test Subsystem
// ============================================================
typedef struct {
    uint32_t win_id;
    uint32_t status_lbl_id;
    uint32_t count_lbl_id;
    uint32_t progress_id;
    uint32_t step_counter;
    bool active;
    uint32_t stress_wins[40];
    uint32_t stress_win_count;
} StressCtx;

static void run_stress_step(StressCtx* ctx) {
    ctx->step_counter++;
    
    // Update progress bar
    if (ctx->progress_id != 0) {
        BWE_Window* pb = BWE_GetWindow(ctx->progress_id);
        if (pb) {
            pb->control_data.progressbar.value = (ctx->step_counter % 100);
            BWE_InvalidateWindow(ctx->progress_id);
        }
    }
    
    // Update status label
    if (ctx->status_lbl_id != 0) {
        BWE_Window* lbl = BWE_GetWindow(ctx->status_lbl_id);
        if (lbl) {
            char buf[128];
            strcpy(buf, "Step: ");
            strcat_itoa(ctx->step_counter, buf);
            if (ctx->stress_win_count < 40) {
                strcat(buf, " - Spawning Window");
            } else {
                strcat(buf, " - Random Ops");
            }
            strcpy(lbl->control_data.label.text, buf);
            BWE_InvalidateWindow(ctx->status_lbl_id);
        }
    }

    if (ctx->stress_win_count < 40) {
        // Spawn a new dummy window
        uint32_t new_win_id = 0;
        int32_t rx = 50 + (ctx->stress_win_count * 10) % 600;
        int32_t ry = 80 + (ctx->stress_win_count * 15) % 300;
        char name[32];
        strcpy(name, "Stress #");
        strcat_itoa(ctx->stress_win_count + 1, name);
        
        bwe_error_t err = BOS_CreateWindow(rx, ry, 280, 200, name, &new_win_id);
        if (err == BWE_SUCCESS && new_win_id != 0) {
            ctx->stress_wins[ctx->stress_win_count++] = new_win_id;
            
            // Mark shown explicitly
            BOS_Show(new_win_id);
            
            // Add some controls to it!
            uint32_t dummy = 0;
            BOS_CreateLabel(new_win_id, 10, 10, "BWE V2.1 Stress Test Client", 0xFF0F172A, &dummy);
            BOS_CreateButton(new_win_id, 10, 40, 100, 30, "Action", 0, &dummy);
            BOS_CreateCheckbox(new_win_id, 10, 80, 120, 25, "Option 1", 0, &dummy);
            
            uint32_t pb_id = 0;
            BOS_CreateProgressBar(new_win_id, 10, 120, 240, 20, 0, 100, &pb_id);
            if (pb_id != 0) {
                BWE_Window* pb_inner = BWE_GetWindow(pb_id);
                if (pb_inner) pb_inner->control_data.progressbar.value = 45;
            }
            
            uint32_t lv_id = 0;
            BOS_CreateListView(new_win_id, 140, 40, 120, 70, &lv_id);
            if (lv_id != 0) {
                BOS_ListView_AddItem(lv_id, "Item 1");
                BOS_ListView_AddItem(lv_id, "Item 2");
            }
        }
    } else {
        // We have 40 windows! Randomize ops!
        // Pick a random window
        uint32_t idx = ctx->step_counter % 40;
        uint32_t target_win = ctx->stress_wins[idx];
        if (target_win != 0) {
            BWE_Window* w = BWE_GetWindow(target_win);
            if (w) {
                uint32_t op = (ctx->step_counter / 40) % 5;
                if (op == 0) {
                    // Move it
                    int32_t nx = 50 + (ctx->step_counter * 25) % 600;
                    int32_t ny = 80 + (ctx->step_counter * 17) % 300;
                    BOS_SetBounds(target_win, nx, ny, w->screen_bounds.width, w->screen_bounds.height);
                } else if (op == 1) {
                    // Focus it
                    BOS_SetFocus(target_win);
                } else if (op == 2) {
                    // Minimize/Hide
                    BOS_Hide(target_win);
                } else if (op == 3) {
                    // Restore/Show
                    BOS_Show(target_win);
                } else {
                    // Close and rebuild it!
                    BOS_DestroySurface(target_win);
                    ctx->stress_wins[idx] = 0;
                    
                    // Spawn a new one at a different spot
                    uint32_t new_win_id = 0;
                    int32_t rx = 50 + (ctx->step_counter * 31) % 600;
                    int32_t ry = 80 + (ctx->step_counter * 23) % 300;
                    char name[32];
                    strcpy(name, "Stress #");
                    strcat_itoa(idx + 1, name);
                    
                    bwe_error_t err = BOS_CreateWindow(rx, ry, 280, 200, name, &new_win_id);
                    if (err == BWE_SUCCESS && new_win_id != 0) {
                        ctx->stress_wins[idx] = new_win_id;
                        BOS_Show(new_win_id);
                        
                        uint32_t dummy = 0;
                        BOS_CreateLabel(new_win_id, 10, 10, "BWE V2.1 Stress Test Client", 0xFF0F172A, &dummy);
                        BOS_CreateButton(new_win_id, 10, 40, 100, 30, "Action", 0, &dummy);
                        BOS_CreateCheckbox(new_win_id, 10, 80, 120, 25, "Option 1", 0, &dummy);
                        
                        uint32_t pb_id = 0;
                        BOS_CreateProgressBar(new_win_id, 10, 120, 240, 20, 0, 100, &pb_id);
                        if (pb_id != 0) {
                            BWE_Window* pb_inner = BWE_GetWindow(pb_id);
                            if (pb_inner) pb_inner->control_data.progressbar.value = 65;
                        }
                    }
                }
            }
        }
    }
    
    // Update count label
    if (ctx->count_lbl_id != 0) {
        BWE_Window* clbl = BWE_GetWindow(ctx->count_lbl_id);
        if (clbl) {
            char buf[64];
            strcpy(buf, "Active Windows: ");
            strcat_itoa(ctx->stress_win_count, buf);
            strcpy(clbl->control_data.label.text, buf);
            BWE_InvalidateWindow(ctx->count_lbl_id);
        }
    }
}

static void btn_stress_toggle_clicked(uint32_t btn_id) {
    StressCtx* ctx = (StressCtx*)get_top_parent_ctx(btn_id);
    if (!ctx) return;
    
    ctx->active = !ctx->active;
    
    BWE_Window* btn = BWE_GetWindow(btn_id);
    if (btn) {
        if (ctx->active) {
            strcpy(btn->control_data.button.text, "Stop Stress");
        } else {
            strcpy(btn->control_data.button.text, "Start Stress");
        }
        BWE_InvalidateWindow(btn_id);
    }
}

static void stress_test_paint_handler(BWE_Window* self) {
    StressCtx* ctx = (StressCtx*)self->user_data;
    if (!ctx) return;
    
    // Render default panel background
    extern void BWE_FillRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    extern const BVFramebuffer* BWE_GetRenderTarget(void);
    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (fb) {
        BWE_FillRect(fb, self->screen_bounds.x, self->screen_bounds.y, self->screen_bounds.width, self->screen_bounds.height, 0xFFF1F5F9);
    }
    
    // If active, run step every 15 frames (~250ms)
    static uint32_t frame_tick = 0;
    frame_tick++;
    if (ctx->active && (frame_tick % 15 == 0)) {
        run_stress_step(ctx);
    }
    
    // Invalidate main window to keep composer ticking while active
    if (ctx->active) {
        BWE_InvalidateWindow(self->id);
    }
}

bwe_error_t stress_test_init(uint32_t* out_win) {
    uint32_t win_id = 0;
    bwe_error_t err = BOS_CreateWindow(100, 80, 400, 300, "BWE Stress Test Mode", &win_id);
    if (err != BWE_SUCCESS) return err;
    
    BWE_Window* win = BWE_GetWindow(win_id);
    if (!win) return BWE0002;
    
    extern void* kcalloc(size_t num, size_t size);
    StressCtx* ctx = (StressCtx*)kcalloc(1, sizeof(StressCtx));
    ctx->win_id = win_id;
    ctx->step_counter = 0;
    ctx->active = false;
    ctx->stress_win_count = 0;
    win->user_data = ctx;
    win->on_render = stress_test_paint_handler;
    
    uint32_t dummy = 0;
    BOS_CreateLabel(win_id, 20, 20, "BWE V2.1 Stabilization Stress Tester", 0xFF0F172A, &dummy);
    BOS_CreateButton(win_id, 20, 60, 140, 40, "Start Stress", btn_stress_toggle_clicked, &dummy);
    
    BOS_CreateLabel(win_id, 20, 120, "Step: 0", 0xFF475569, &ctx->status_lbl_id);
    BOS_CreateLabel(win_id, 20, 150, "Active Windows: 0", 0xFF475569, &ctx->count_lbl_id);
    
    BOS_CreateProgressBar(win_id, 20, 190, 360, 25, 0, 100, &ctx->progress_id);
    
    if (out_win) *out_win = win_id;
    return BWE_SUCCESS;
}

// ============================================================
// Music Player Implementation
// ============================================================
static void music_canvas_paint(uint32_t canvas_id, const BVFramebuffer* fb, const BWE_Rect* clip) {
    (void)clip;
    BWE_Window* self = BWE_GetWindow(canvas_id);
    if (!self) return;

    BWE_Rect b = self->screen_bounds;
    BWE_FillRect(fb, b.x, b.y, b.width, b.height, 0xFF0F172A);

    BWE_DrawText(fb, "ATOMS Music Player", b.x + 20, b.y + 30, 0xFF3B82F6, 0);
    BWE_DrawText(fb, "Now Playing: DEMO1.wav", b.x + 20, b.y + 70, 0xFFF1F5F9, 0);
    BWE_DrawText(fb, "Status: Streaming via AC97 DMA", b.x + 20, b.y + 100, 0xFF94A3B8, 0);
}

static void btn_play_clicked(uint32_t btn_id) {
    (void)btn_id;
    audio_player_open("/DEMO1.WAV");
    audio_player_play();
}

static void btn_pause_clicked(uint32_t btn_id) {
    (void)btn_id;
    if (audio_player_is_playing()) {
        audio_player_pause();
    } else {
        audio_player_resume();
    }
}

static void btn_stop_clicked(uint32_t btn_id) {
    (void)btn_id;
    audio_player_stop();
}

bwe_error_t music_init_v2(uint32_t* out_win) {
    uint32_t win_id = 0;
    bwe_error_t err = BOS_CreateWindow(200, 120, 420, 250, "ATOMS Music", &win_id);
    if (err != BWE_SUCCESS) return err;

    BWE_Window* win = BWE_GetWindow(win_id);
    if (!win) return BWE0002;

    // Canvas for player visuals
    uint32_t canvas_id = 0;
    BOS_CreateCanvas(win_id, 0, 0, 420, 140, music_canvas_paint, &canvas_id);

    // Play button
    uint32_t dummy = 0;
    BOS_CreateButton(win_id, 20, 150, 100, 40, "Play", btn_play_clicked, &dummy);
    if (dummy != 0) {
        BWE_Window* btn = BWE_GetWindow(dummy);
        if (btn) {
            btn->control_data.button.bg_color = 0xFF2563EB;
            btn->control_data.button.text_color = 0xFFFFFFFF;
        }
    }

    // Pause button
    BOS_CreateButton(win_id, 140, 150, 100, 40, "Pause", btn_pause_clicked, &dummy);
    if (dummy != 0) {
        BWE_Window* btn = BWE_GetWindow(dummy);
        if (btn) {
            btn->control_data.button.bg_color = 0xFFEAB308;
            btn->control_data.button.text_color = 0xFF000000;
        }
    }

    // Stop button
    BOS_CreateButton(win_id, 260, 150, 100, 40, "Stop", btn_stop_clicked, &dummy);
    if (dummy != 0) {
        BWE_Window* btn = BWE_GetWindow(dummy);
        if (btn) {
            btn->control_data.button.bg_color = 0xFFEF4444;
            btn->control_data.button.text_color = 0xFFFFFFFF;
        }
    }

    if (out_win) *out_win = win_id;
    return BWE_SUCCESS;
}
