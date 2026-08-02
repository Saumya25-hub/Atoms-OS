#ifndef HORSE_ENGINE_H
#define HORSE_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

/* App IDs for Foundation v2 */
#define APP_ID_ATOMS        1
#define APP_ID_EXPLORER     2
#define APP_ID_TERMINAL     3
#define APP_ID_CALCULATOR   4
#define APP_ID_SETTINGS     5
#define APP_ID_SANDBOX      6
#define APP_ID_STRESS_TEST  7
#define APP_ID_MUSIC        8
#define APP_ID_IMAGE_VIEWER 9
#define APP_ID_DOOM         10
#define APP_ID_INPUT_LAB    11
#define APP_ID_ATRIX        12
#define APP_ID_GRAPH_3D     13
#define APP_ID_TMH          14
#define APP_ID_FORGE_APP    15
#define APP_ID_CONTROLPANEL 16


// Required API Definition
void horse_init(void);
void horse_dispatch(void);

// App Registry & Launch API
typedef struct {
    uint32_t app_id;
    const char* display_name;
    uint32_t icon_id;
    int (*launch_callback)(uint32_t* out_win_id);
} HorseAppEntry;

void horse_register(uint32_t app_id, const char* name, int (*launch_cb)(uint32_t*), uint32_t icon_id);
void horse_launch(uint32_t app_id);
HorseAppEntry* horse_get_running(uint32_t* out_count);
void horse_focus(uint32_t app_id);

// Power API
void horse_shutdown(void);
void horse_restart(void);

#endif // HORSE_ENGINE_H
