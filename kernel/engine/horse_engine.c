#include "horse_engine.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/apps/atrix/atrix_browser.h"

extern void display_print(const char* s);

#define MAX_APPS 16
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

    extern void BOVISUAL_Graphics_SwapFull(const void* hw_fb);
    extern void* vbe_get_back_page_ptr(void);
    // Force immediate presentation to visually register the click before we freeze
    BOVISUAL_Graphics_SwapFull(vbe_get_back_page_ptr());

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

void horse_init(void) {
    display_print("[Horse Engine] Initializing and Registering BOSX Primary Applications...\n");
    s_app_count = 0;
    
    horse_register(APP_ID_EXPLORER,     "File Explorer.BOSX", bosx_fileexplorer_launch, 1);
    horse_register(APP_ID_TERMINAL,     "Terminal.BOSX",     bosx_terminal_launch, 2);
    horse_register(APP_ID_SETTINGS,     "Settings.BOSX",     bosx_settings_launch, 3);
    horse_register(APP_ID_CALCULATOR,   "Calculator",        calculator_init_v2, 4);
    horse_register(APP_ID_SANDBOX,      "Sandbox",           demo_app_launch_wrapper, 5);
    horse_register(APP_ID_STRESS_TEST,  "Stress Test",       stress_test_init, 6);
    horse_register(APP_ID_MUSIC,        "Music",             music_init_v2, 7);
    horse_register(APP_ID_DOOM,         "DOOM 1",            doom_launch_wrapper, 8);
    horse_register(APP_ID_INPUT_LAB,    "Input Lab",         input_lab_init, 9);
    horse_register(APP_ID_ATRIX,        "ATRIX Browser",     (int (*)(uint32_t*))atrix_browser_launch, 10);
    horse_register(APP_ID_GRAPH_3D,     "ATOMS 3D Benchmark",atoms_graph_3d_launch, 11);
    horse_register(APP_ID_TMH,          "Task Manager.BOSX", bosx_taskmanager_launch, 6);
    horse_register(APP_ID_CONTROLPANEL, "ControlPanel.BOSX", bosx_controlpanel_launch, 3);
    horse_register(APP_ID_FORGE_APP,    "Forge App",         forge_app_launch_wrapper, 12);
}


void horse_dispatch(void) {
    // Stub for future task scheduling
}

void horse_launch(uint32_t app_id) {
    for (uint32_t i = 0; i < s_app_count; i++) {
        if (s_app_registry[i].app_id == app_id) {
            if (s_app_registry[i].launch_callback) {
                // Force immediate presentation to visually register the click before we freeze loading the app
                extern void BOVISUAL_Graphics_SwapFull(const void* hw_fb);
                extern void* vbe_get_back_page_ptr(void);
                BOVISUAL_Graphics_SwapFull(vbe_get_back_page_ptr());

                uint32_t win_id = 0;
                int err = s_app_registry[i].launch_callback(&win_id);
                if (err == 0 && win_id != 0) {
                    BWE_Window* win = BWE_GetWindow(win_id);
                    if (win && !win->user_data) {
                        win->user_data = (void*)(uintptr_t)app_id;
                    }
                    BOS_Show(win_id);
                    BOS_SetFocus(win_id);
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
    extern void io_out16(uint16_t port, uint16_t data);
    io_out16(0x604, 0x2000); // QEMU ACPI shutdown
    // Bochs/older QEMU
    io_out16(0xB004, 0x2000);
}

void horse_restart(void) {
    display_print("[Horse] Restarting OS...\n");
    extern void io_out8(uint16_t port, uint8_t data);
    io_out8(0x64, 0xFE); // Pulse reset line via keyboard controller
}
