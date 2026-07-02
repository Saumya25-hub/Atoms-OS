#ifndef HORSE_ENGINE_H
#define HORSE_ENGINE_H

#include <stdint.h>

/* App IDs for Foundation v2 */
#define APP_ID_ATOMS     1
#define APP_ID_SEARCH    2
#define APP_ID_FILES     3
#define APP_ID_NOTES     4
#define APP_ID_TERMINAL  5
#define APP_ID_SETTINGS  6

/* Initializes the Horse Engine */
void horse_init(void);

/* Main dispatch routine for engine tasks */
void horse_dispatch(void);

/* Initiates a high-speed search lookup */
void horse_search(const char* query);

/* Safely launches an application, handling resources and process spawning */
void horse_launch(uint32_t app_id);

#endif // HORSE_ENGINE_H
