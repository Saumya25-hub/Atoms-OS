#include "horse_engine.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* s);

#define MAX_APPS 32
static HorseAppEntry s_app_registry[MAX_APPS];
static uint32_t s_app_count = 0;

// App init callbacks from other modules
extern int explorer_init(uint32_t* out_win);
extern int terminal_init_v2(uint32_t* out_win);
extern int calculator_init_v2(uint32_t* out_win);
extern int settings_init_v2(uint32_t* out_win);
extern int stress_test_init(uint32_t* out_win);
extern int music_init_v2(uint32_t* out_win);
extern int sandbox_init(uint32_t* out_win);
extern int app_image_viewer_init(uint32_t* out_win);
extern int doom_bwe_init(uint32_t* out_win);
extern int input_lab_init(uint32_t* out_win);
extern int atoms_graph_3d_launch(uint32_t* out_win);

static int demo_app_launch_wrapper(uint32_t* out_win) {
    extern void BWE_DemoApp_Initialize(void);
    BWE_DemoApp_Initialize();
    if (out_win) *out_win = 1; // Standard demo window ID is 1
    return 0;
}

#include "kernel/core/process/include/process_image.h"
#include "kernel/core/process/include/enter_usermode.h"

static int doom_launch_wrapper(uint32_t* out_win) {
    extern void* vmm_create_address_space(void);
    extern ProcessImage* elf_load_image(void* pml4, const char* path);
    extern bool process_build_user_stack(ProcessImage* image, void* pml4);
    
    void* new_pml4 = vmm_create_address_space();
    ProcessImage* new_image = elf_load_image(new_pml4, "DOOM.ELF");
    if (!new_image) {
        return -1;
    }

    if (!process_build_user_stack(new_image, new_pml4)) {
        return -1;
    }

    extern void BCM_RequestFullRepaint(void);
    BCM_RequestFullRepaint();

    extern void* process_spawn(ProcessImage* image, const char* name);
    process_spawn(new_image, "DOOM.ELF");
    
    if (out_win) *out_win = 0; // It spawns a new task, no single window ID to return synchronously
    return 0;
}

void horse_register(uint32_t app_id, const char* name, int (*launch_cb)(uint32_t*), uint32_t icon_id) {
    if (s_app_count >= MAX_APPS) return;
    s_app_registry[s_app_count].app_id = app_id;
    s_app_registry[s_app_count].display_name = name;
    s_app_registry[s_app_count].launch_callback = launch_cb;
    s_app_registry[s_app_count].icon_id = icon_id;
    s_app_count++;
}

// Phase 29 — Production BOSX Application Entry Point Declarations
extern int32_t FileExplorerInitialize(void);
extern int32_t TerminalInitialize(void);
extern int32_t SettingsInitialize(void);
extern int32_t TaskManagerInitialize(void);
extern int32_t ControlPanelInitialize(void);

extern int explorer_init(uint32_t* out_win);
extern int terminal_init_v2(uint32_t* out_win);
extern int settings_init_v2(uint32_t* out_win);
extern int tmh_app_init(uint32_t* out_win);

static int bosx_fileexplorer_launch(uint32_t* out_win) {
    display_print("[BOSX LAUNCHER] Invoking FileExplorer.BOSX V1.0...\n");
    FileExplorerInitialize();
    return explorer_init(out_win);
}

static int bosx_terminal_launch(uint32_t* out_win) {
    display_print("[BOSX LAUNCHER] Invoking Terminal.BOSX V1.0...\n");
    TerminalInitialize();
    return terminal_init_v2(out_win);
}

static int bosx_settings_launch(uint32_t* out_win) {
    display_print("[BOSX LAUNCHER] Invoking Settings.BOSX V1.0...\n");
    SettingsInitialize();
    return settings_init_v2(out_win);
}

static int bosx_taskmanager_launch(uint32_t* out_win) {
    display_print("[BOSX LAUNCHER] Invoking TaskManager.BOSX V1.0...\n");
    TaskManagerInitialize();
    return tmh_app_init(out_win);
}

static int bosx_controlpanel_launch(uint32_t* out_win) {
    display_print("[BOSX LAUNCHER] Invoking ControlPanel.BOSX V1.0...\n");
    ControlPanelInitialize();
    return settings_init_v2(out_win);
}

#include "../runtime/installer/include/bos_installer.h"

static int forge_app_launch_wrapper(uint32_t *out_win) {
    display_print("[BOS NATIVE LAUNCHER] Spawning Native SDK Explorer Executable...\n");
    extern void* vmm_create_address_space(void);
    extern ProcessImage* elf_load_image(void* pml4, const char* path);
    extern bool process_build_user_stack(ProcessImage* image, void* pml4);
    
    void* new_pml4 = vmm_create_address_space();
    ProcessImage* new_image = elf_load_image(new_pml4, "CALC.ELF");
    if (!new_image) {
        return -1;
    }

    if (!process_build_user_stack(new_image, new_pml4)) {
        return -1;
    }

    extern void* process_spawn(ProcessImage* image, const char* name);
    process_spawn(new_image, "CALC.ELF");
    
    if (out_win) *out_win = 0;
    return 0;
}

static int chromium_browser_launch(uint32_t *out_win) {
    display_print("[CHROMIUM] Spawning REAL Chromium Browser (chromium_browser.elf)...\n");
    extern void* vmm_create_address_space(void);
    extern ProcessImage* elf_load_image(void* pml4, const char* path);
    extern ProcessImage* elf_load_image_from_buffer(void* pml4, const void* buf, uint64_t size);
    extern bool process_build_user_stack(ProcessImage* image, void* pml4);
    extern void* process_spawn(ProcessImage* image, const char* name);
    extern void BCM_RequestFullRepaint(void);

    extern const uint8_t g_embedded_chromium_elf[];
    extern const uint64_t g_embedded_chromium_elf_len;
    extern uint64_t get_embedded_chromium_elf_len(void);

    void* new_pml4 = vmm_create_address_space();
    if (!new_pml4) {
        display_print("[CHROMIUM] ERROR: Failed to allocate address space (PML4)!\n");
        return -1;
    }

    ProcessImage* new_image = elf_load_image(new_pml4, "chromium_browser.elf");
    if (!new_image) new_image = elf_load_image(new_pml4, "/chromium_browser.elf");
    if (!new_image) {
        uint64_t elf_len = get_embedded_chromium_elf_len();
        if (elf_len == 0) elf_len = g_embedded_chromium_elf_len;
        if (elf_len > 0) {
            display_print("[CHROMIUM] Loading Chromium from embedded ELF payload...\n");
            new_image = elf_load_image_from_buffer(new_pml4, g_embedded_chromium_elf, elf_len);
        }
    }

    if (!new_image) {
        display_print("[CHROMIUM] ERROR: Could not load chromium_browser.elf\n");
        return -1;
    }
    display_print("[CHROMIUM] ELF loaded successfully.\n");

    if (!process_build_user_stack(new_image, new_pml4)) {
        display_print("[CHROMIUM] ERROR: Could not build user stack\n");
        return -1;
    }

    BCM_RequestFullRepaint();

    void* proc = process_spawn(new_image, "chromium_browser.elf");
    if (!proc) {
        display_print("[CHROMIUM] ERROR: process_spawn failed for chromium_browser.elf!\n");
        return -1;
    }

    display_print("[CHROMIUM] SUCCESS: Real Chromium Browser spawned into Ring 3 (PID assigned)!\n");

    if (out_win) *out_win = 0;
    return 0;
}

#include "kernel/shell/apps/bos_media_player/include/bos_media_player.h"
#include "../shell/apps/notes_app.h"

void horse_init(void) {
    display_print("[Horse Engine] Initializing and Registering BOSX Primary Applications...\n");
    s_app_count = 0;
    
    horse_register(APP_ID_EXPLORER,     "File Explorer",     bosx_fileexplorer_launch, 1);
    horse_register(APP_ID_NOTES,        "Notes",             notes_app_launch, 2);
    horse_register(APP_ID_CALCULATOR,   "Calculator",        calculator_init_v2, 4);
    horse_register(APP_ID_TERMINAL,     "Terminal",          bosx_terminal_launch, 2);
    horse_register(APP_ID_SETTINGS,     "Settings",          bosx_settings_launch, 3);
    horse_register(APP_ID_MUSIC,        "Media Player",      (int (*)(uint32_t*))bos_media_player_launch, 7);
    horse_register(APP_ID_CHROMIUM,     "Chromium",          chromium_browser_launch, 10);
    horse_register(APP_ID_TMH,          "Task Manager",      bosx_taskmanager_launch, 6);
    horse_register(APP_ID_CONTROLPANEL, "Control Panel",     bosx_controlpanel_launch, 3);
    horse_register(APP_ID_DOOM,         "DOOM",              doom_launch_wrapper, 8);
    horse_register(APP_ID_GRAPH_3D,     "3D Benchmark",      atoms_graph_3d_launch, 11);
    horse_register(APP_ID_SANDBOX,      "Sandbox",           demo_app_launch_wrapper, 5);
    horse_register(APP_ID_STRESS_TEST,  "Stress Test",       stress_test_init, 6);
    horse_register(APP_ID_INPUT_LAB,    "Input Lab",         input_lab_init, 9);
    horse_register(APP_ID_FORGE_APP,    "Forge App",         forge_app_launch_wrapper, 12);
}


void horse_dispatch(void) {
    // Stub for future task scheduling
}

void horse_launch(uint32_t app_id) {
    for (uint32_t i = 0; i < s_app_count; i++) {
        if (s_app_registry[i].app_id == app_id) {
            if (s_app_registry[i].launch_callback) {
                /* Set AppStarting animated cursor (appstarting.ani spinner) */
                typedef uint32_t bce_error_t;
                extern bce_error_t bos_cursor_set_active_type(uint32_t type);
                bos_cursor_set_active_type(4 /* BCE_CURSOR_APPSTARTING */);

                extern void BCM_RequestFullRepaint(void);
                BCM_RequestFullRepaint();

                uint32_t win_id = 0;
                int err = s_app_registry[i].launch_callback(&win_id);
                if (err == 0 && win_id != 0) {
                    BWE_Window* win = BWE_GetWindow(win_id);
                    if (win && !win->user_data) {
                        win->user_data = (void*)(uintptr_t)app_id;
                    }
                    BOS_Show(win_id);
                    BOS_SetFocus(win_id);
                    BWE_BringToFront(win_id);
                    extern void TaskPanel_Update(void);
                    TaskPanel_Update();
                    extern bool audio_player_is_playing(void);
                    if (!audio_player_is_playing()) {
                        display_print("[Horse] Launched App.\n");
                    }
                }
            }
            return;
        }
    }
}

HorseAppEntry* horse_get_running(uint32_t* out_count) {
    // For now, we return the entire registry. 
    // In the future, this would return actually running processes.
    // For the task panel, we can just display the registered apps as shortcuts,
    // or filter by active windows if needed. 
    // Actually, "Show running applications" in task panel means we should check window count.
    if (out_count) *out_count = s_app_count;
    return s_app_registry;
}

void horse_focus(uint32_t app_id) {
    // To properly focus, we need the window ID.
    // Since we don't track process->window mappings yet, we do a naive lookup 
    // or we can just launch it if it's not focused.
    // In this basic version, we can just call launch again (most apps re-focus or spawn new).
    horse_launch(app_id);
}

void horse_shutdown(void) {
    display_print("[Horse] Shutting down OS...\n");
    extern void atoms_power_shutdown(void);
    atoms_power_shutdown();
}

void horse_restart(void) {
    display_print("[Horse] Restarting OS...\n");
    extern void atoms_power_reboot(void);
    atoms_power_reboot();
}
