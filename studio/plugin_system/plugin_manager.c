#include "platform/include/bos_types.h"

typedef struct {
    char plugin_name[64];
    char version[16];
    void (*on_init)(void);
    void (*on_shutdown)(void);
} BOS_StudioPlugin;

#define BOS_MAX_STUDIO_PLUGINS 16U
static BOS_StudioPlugin g_studio_plugins[BOS_MAX_STUDIO_PLUGINS];
static uint32_t g_plugin_count = 0;

void BOS_Studio_RegisterPlugin(const BOS_StudioPlugin* plugin) {
    if (!plugin || g_plugin_count >= BOS_MAX_STUDIO_PLUGINS) return;
    g_studio_plugins[g_plugin_count++] = *plugin;
    if (plugin->on_init) {
        plugin->on_init();
    }
}
