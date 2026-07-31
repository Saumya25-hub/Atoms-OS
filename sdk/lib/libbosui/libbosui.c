#include "sdk/include/bos/bos.h"

BOS_Result BOS_SDK_Init(const char* app_name, const char* version) {
    BOS_AppConfig cfg;
    cfg.app_name = app_name ? app_name : "BOSApp";
    cfg.version = version ? version : "1.0.0";
    cfg.on_startup = NULL;
    cfg.on_shutdown = NULL;
    cfg.on_event = NULL;

    BOS_App* app = NULL;
    return BOS_InitApplication(&cfg, &app);
}

int BOS_SDK_Run(void) {
    BOS_App* app = BOS_GetCurrentApplication();
    if (!app) return -1;
    BOS_Result res = BOS_RunApplication(app);
    return (res == BOS_SUCCESS) ? app->exit_code : -1;
}

void BOS_SDK_Quit(int exit_code) {
    BOS_QuitApplication(NULL, exit_code);
}

BOS_Window* BOS_SDK_CreateWindow(int32_t x, int32_t y, uint32_t w, uint32_t h, const char* title) {
    return BOS_Window_Create(x, y, w, h, title);
}

void BOS_SDK_ShowWindow(BOS_Window* win) {
    BOS_Window_Show(win);
}
