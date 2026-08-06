#ifndef BOS_CONNECTOR_H
#define BOS_CONNECTOR_H

#include "kernel/graphics/display/include/bos_display.h"
#include "kernel/graphics/display/include/bos_edid.h"

typedef enum {
    BOS_CONNECTOR_TYPE_VGA = 1,
    BOS_CONNECTOR_TYPE_DVI,
    BOS_CONNECTOR_TYPE_HDMI,
    BOS_CONNECTOR_TYPE_DISPLAYPORT,
    BOS_CONNECTOR_TYPE_LVDS,
    BOS_CONNECTOR_TYPE_EDP,
    BOS_CONNECTOR_TYPE_VIRTUAL
} bos_connector_type_t;

typedef struct {
    bos_connector_id_t   id;
    bos_connector_type_t type;
    const char*          name;
    bool                 connected;
    bool                 hpd_supported;
    edid_info_t          edid;
    bos_display_id_t     bound_display_id;
} bos_connector_t;

const char* bos_connector_type_to_string(bos_connector_type_t type);

#endif /* BOS_CONNECTOR_H */
