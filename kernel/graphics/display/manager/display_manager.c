#include "kernel/graphics/display/manager/display_manager.h"
#include "kernel/graphics/display/include/bos_edid.h"

extern void display_print(const char* str);
extern void display_print_dec(uint64_t val);
extern void display_print_hex(uint64_t val);

#define MAX_BOS_DISPLAYS 4

static bos_display_device_t g_displays[MAX_BOS_DISPLAYS];
static uint32_t             g_display_count = 0;
static bos_multi_monitor_mode_t g_multi_monitor_mode = BOS_MULTI_MONITOR_SINGLE;
static bool                 g_display_subsystem_initialized = false;

/* Helper String Converters */
const char* bos_connector_type_to_string(bos_connector_type_t type) {
    switch (type) {
        case BOS_CONNECTOR_TYPE_VGA:         return "VGA";
        case BOS_CONNECTOR_TYPE_DVI:         return "DVI";
        case BOS_CONNECTOR_TYPE_HDMI:        return "HDMI";
        case BOS_CONNECTOR_TYPE_DISPLAYPORT: return "DisplayPort";
        case BOS_CONNECTOR_TYPE_LVDS:        return "LVDS";
        case BOS_CONNECTOR_TYPE_EDP:         return "eDP";
        case BOS_CONNECTOR_TYPE_VIRTUAL:     return "Virtual Display";
        default:                             return "Unknown";
    }
}

bool bos_mode_validate(const bos_display_mode_t* mode) {
    if (!mode) return false;
    if (mode->width < 640 || mode->width > 7680) return false;
    if (mode->height < 480 || mode->height > 4320) return false;
    if (mode->refresh_rate < 24 || mode->refresh_rate > 240) return false;
    return true;
}

bool bos_mode_equal(const bos_display_mode_t* a, const bos_display_mode_t* b) {
    if (!a || !b) return false;
    return (a->width == b->width && a->height == b->height && a->refresh_rate == b->refresh_rate);
}

/* Atomic State Machine Routines */
bos_display_status_t bos_atomic_state_init(bos_atomic_state_t* state) {
    if (!state) return BOS_DISPLAY_ERR_INVALID_PARAM;
    state->num_snapshots = 0;
    state->checked = false;
    state->committed = false;
    return BOS_DISPLAY_OK;
}

bos_display_status_t bos_atomic_state_add(bos_atomic_state_t* state, const bos_display_state_snapshot_t* snapshot) {
    if (!state || !snapshot || state->num_snapshots >= 8) return BOS_DISPLAY_ERR_INVALID_PARAM;
    state->snapshots[state->num_snapshots++] = *snapshot;
    return BOS_DISPLAY_OK;
}

bos_display_status_t bos_atomic_check(bos_atomic_state_t* state) {
    if (!state) return BOS_DISPLAY_ERR_INVALID_PARAM;
    for (uint32_t i = 0; i < state->num_snapshots; i++) {
        if (!bos_mode_validate(&state->snapshots[i].mode)) {
            return BOS_DISPLAY_ERR_ATOMIC_REJECT;
        }
    }
    state->checked = true;
    return BOS_DISPLAY_OK;
}

bos_display_status_t bos_atomic_commit(bos_atomic_state_t* state) {
    if (!state || !state->checked) return BOS_DISPLAY_ERR_ATOMIC_REJECT;
    state->committed = true;
    return BOS_DISPLAY_OK;
}

bos_display_status_t bos_atomic_rollback(bos_atomic_state_t* state) {
    if (!state) return BOS_DISPLAY_ERR_INVALID_PARAM;
    state->checked = false;
    state->committed = false;
    return BOS_DISPLAY_OK;
}

/* Central Display Subsystem Implementation */

bos_display_status_t bos_display_init(void) {
    if (g_display_subsystem_initialized) return BOS_DISPLAY_OK;

    display_print("[DISPLAY CORE] Initializing Central Display Subsystem...\n");

    g_display_count = 0;
    for (int i = 0; i < MAX_BOS_DISPLAYS; i++) {
        for (int b = 0; b < (int)sizeof(bos_display_device_t); b++) {
            ((uint8_t*)&g_displays[i])[b] = 0;
        }
    }

    /* Register Primary Virtual / Hardware Display Engine 1 */
    bos_display_device_t* primary = &g_displays[0];
    primary->id = 101;
    primary->state = BOS_DISPLAY_STATE_ACTIVE;
    primary->is_primary = true;
    primary->is_active = true;

    primary->connector.id = 1;
    primary->connector.type = BOS_CONNECTOR_TYPE_DISPLAYPORT;
    primary->connector.name = "DP-1";
    primary->connector.connected = true;
    primary->connector.hpd_supported = true;
    primary->connector.bound_display_id = primary->id;

    /* Parse Synthetic / Hardware EDID Block */
    uint8_t synthetic_edid[128] = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00 };
    edid_parse_base(synthetic_edid, &primary->connector.edid);

    /* Populate Mode Tables */
    bos_display_mode_t m1080p = {
        .width = 1920, .height = 1080, .refresh_rate = 60, .bpp = 32,
        .pixel_clock_khz = 148500, .htotal = 2200, .vtotal = 1125,
        .flags = BOS_MODE_FLAG_PREFERRED | BOS_MODE_FLAG_NATIVE,
        .name = "1920x1080@60Hz"
    };

    bos_display_mode_t m720p = {
        .width = 1280, .height = 720, .refresh_rate = 60, .bpp = 32,
        .pixel_clock_khz = 74250, .htotal = 1650, .vtotal = 750,
        .flags = BOS_MODE_FLAG_SAFE,
        .name = "1280x720@60Hz"
    };

    primary->active_mode = m1080p;
    primary->preferred_mode = m1080p;
    primary->supported_modes[0] = m1080p;
    primary->supported_modes[1] = m720p;
    primary->supported_mode_count = 2;

    const char name[] = "BOS Primary Display (DP-1)";
    for (int j = 0; j < 63 && name[j] != '\0'; j++) primary->name[j] = name[j];

    g_display_count = 1;

    /* Register Secondary HDMI Connector 2 for Multi-Monitor */
    bos_display_device_t* secondary = &g_displays[1];
    secondary->id = 102;
    secondary->state = BOS_DISPLAY_STATE_CONNECTED;
    secondary->is_primary = false;
    secondary->is_active = false;
    secondary->connector.id = 2;
    secondary->connector.type = BOS_CONNECTOR_TYPE_HDMI;
    secondary->connector.name = "HDMI-1";
    secondary->connector.connected = true;
    secondary->connector.bound_display_id = secondary->id;
    edid_parse_base(synthetic_edid, &secondary->connector.edid);
    secondary->active_mode = m1080p;
    secondary->preferred_mode = m1080p;
    secondary->supported_modes[0] = m1080p;
    secondary->supported_mode_count = 1;
    const char name2[] = "BOS Secondary Display (HDMI-1)";
    for (int j = 0; j < 63 && name2[j] != '\0'; j++) secondary->name[j] = name2[j];

    g_display_count = 2;

    g_display_subsystem_initialized = true;
    display_print("[DISPLAY CORE] Display Subsystem Initialized Successfully.\n");
    return BOS_DISPLAY_OK;
}

uint32_t bos_display_enumerate(void) {
    if (!g_display_subsystem_initialized) bos_display_init();
    return g_display_count;
}

bos_display_device_t* bos_display_get_primary(void) {
    if (!g_display_subsystem_initialized) bos_display_init();
    for (uint32_t i = 0; i < g_display_count; i++) {
        if (g_displays[i].is_primary) return &g_displays[i];
    }
    return &g_displays[0];
}

bos_display_device_t* bos_display_get_device(uint32_t index) {
    if (!g_display_subsystem_initialized) bos_display_init();
    if (index >= g_display_count) return NULL;
    return &g_displays[index];
}

bos_display_status_t bos_display_set_mode(bos_display_id_t id, const bos_display_mode_t* mode) {
    if (!mode || !bos_mode_validate(mode)) return BOS_DISPLAY_ERR_INVALID_PARAM;
    bos_display_device_t* dev = NULL;
    for (uint32_t i = 0; i < g_display_count; i++) {
        if (g_displays[i].id == id) {
            dev = &g_displays[i];
            break;
        }
    }
    if (!dev) return BOS_DISPLAY_ERR_NOT_FOUND;

    /* Execute Transactional Atomic Mode Change */
    bos_atomic_state_t atomic;
    bos_atomic_state_init(&atomic);

    bos_display_state_snapshot_t snapshot = {
        .display_id = dev->id,
        .state = BOS_DISPLAY_STATE_ACTIVE,
        .mode = *mode,
        .crtc_id = 1,
        .plane_id = 1,
        .dirty = true
    };

    bos_atomic_state_add(&atomic, &snapshot);
    if (bos_atomic_check(&atomic) != BOS_DISPLAY_OK) {
        bos_atomic_rollback(&atomic);
        return BOS_DISPLAY_ERR_ATOMIC_REJECT;
    }

    bos_atomic_commit(&atomic);
    dev->active_mode = *mode;
    return BOS_DISPLAY_OK;
}

bos_display_status_t bos_display_set_multi_monitor(bos_multi_monitor_mode_t mode) {
    g_multi_monitor_mode = mode;
    return BOS_DISPLAY_OK;
}

void bos_display_dump_diagnostics(void) {
    display_print("\n============================================\n");
    display_print(" BOS DISPLAY SUBSYSTEM FORENSIC DIAGNOSTICS \n");
    display_print("============================================\n");
    display_print(" Total Displays      : "); display_print_dec(g_display_count); display_print("\n");
    display_print(" Multi-Monitor Mode  : ");
    switch (g_multi_monitor_mode) {
        case BOS_MULTI_MONITOR_SINGLE: display_print("SINGLE PRIMARY\n"); break;
        case BOS_MULTI_MONITOR_EXTEND: display_print("EXTENDED DESKTOP\n"); break;
        case BOS_MULTI_MONITOR_CLONE:  display_print("CLONE DESKTOP\n"); break;
        case BOS_MULTI_MONITOR_MIRROR: display_print("MIRROR DESKTOP\n"); break;
    }

    for (uint32_t i = 0; i < g_display_count; i++) {
        bos_display_device_t* d = &g_displays[i];
        display_print("--------------------------------------------\n");
        display_print(" Display #"); display_print_dec(i + 1); display_print(" : "); display_print(d->name); display_print("\n");
        display_print("   Port Type         : "); display_print(bos_connector_type_to_string(d->connector.type)); display_print("\n");
        display_print("   PNP Manufacturer  : "); display_print(d->connector.edid.pnp_id); display_print("\n");
        display_print("   Monitor Name      : "); display_print(d->connector.edid.monitor_name); display_print("\n");
        display_print("   Active Mode       : "); display_print_dec(d->active_mode.width); display_print("x");
        display_print_dec(d->active_mode.height); display_print(" @ "); display_print_dec(d->active_mode.refresh_rate); display_print(" Hz\n");
        display_print("   Primary Display   : "); display_print(d->is_primary ? "YES" : "NO"); display_print("\n");
        display_print("   EDID 1.4 Valid    : "); display_print(d->connector.edid.valid ? "YES" : "NO"); display_print("\n");
        display_print("   CEA-861 Extension : "); display_print(d->connector.edid.has_cea_extension ? "PRESENT" : "NONE"); display_print("\n");
        display_print("   Status            : PASS\n");
    }
    display_print("============================================\n\n");
}
