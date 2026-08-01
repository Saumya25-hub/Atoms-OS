#include "../include/usb_hub_topology.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

#define MAX_TOPOLOGY_NODES 256
static usb_topology_node_t g_nodes[MAX_TOPOLOGY_NODES];
static uint32_t g_node_count = 0;
static atoms_spinlock_t g_topo_lock;

void usb_topology_init(void) {
    atoms_spinlock_init(&g_topo_lock, 0);
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_topo_lock);
    memset(g_nodes, 0, sizeof(g_nodes));
    g_node_count = 0;
    atoms_spin_unlock_irqrestore(&g_topo_lock, state);
    display_print("[UHE TOPOLOGY] Topology Engine Initialized.\n");
}

bool usb_topology_validate_depth(uint8_t depth) {
    return (depth <= USB_MAX_HUB_DEPTH);
}

usb_topology_node_t* usb_topology_create_node(uint32_t node_id, bool is_hub, uint32_t parent_hub_id, uint8_t parent_port_num, uint32_t controller_id, usb_speed_t speed, uint8_t depth) {
    if (!usb_topology_validate_depth(depth)) {
        display_print("[UHE TOPOLOGY] Error: Hub depth limit exceeded (");
        display_print_dec(depth);
        display_print(" > 7)\n");
        return NULL;
    }
    
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_topo_lock);
    if (g_node_count >= MAX_TOPOLOGY_NODES) {
        atoms_spin_unlock_irqrestore(&g_topo_lock, state);
        return NULL;
    }
    
    usb_topology_node_t* node = &g_nodes[g_node_count++];
    memset(node, 0, sizeof(usb_topology_node_t));
    node->node_id = node_id;
    node->is_hub = is_hub;
    node->parent_hub_id = parent_hub_id;
    node->parent_port_num = parent_port_num;
    node->controller_id = controller_id;
    node->speed = speed;
    node->depth = depth;
    
    // Build path string: e.g. "1.2.4"
    if (parent_hub_id == 0) {
        node->path[0] = '1';
        node->path[1] = '\0';
    } else {
        usb_topology_node_t* parent = NULL;
        for (uint32_t i = 0; i < g_node_count - 1; i++) {
            if (g_nodes[i].node_id == parent_hub_id) {
                parent = &g_nodes[i];
                break;
            }
        }
        if (parent) {
            uint32_t len = strlen(parent->path);
            memcpy(node->path, parent->path, len);
            node->path[len] = '.';
            node->path[len + 1] = '0' + parent_port_num;
            node->path[len + 2] = '\0';
        } else {
            node->path[0] = '1';
            node->path[1] = '\0';
        }
    }
    
    atoms_spin_unlock_irqrestore(&g_topo_lock, state);
    return node;
}

bool usb_topology_attach_child(uint32_t parent_hub_id, usb_topology_node_t* child) {
    if (!child) return false;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_topo_lock);
    for (uint32_t i = 0; i < g_node_count; i++) {
        if (g_nodes[i].node_id == parent_hub_id && g_nodes[i].is_hub) {
            if (g_nodes[i].child_count < 16) {
                g_nodes[i].children[g_nodes[i].child_count++] = child;
                atoms_spin_unlock_irqrestore(&g_topo_lock, state);
                return true;
            }
        }
    }
    atoms_spin_unlock_irqrestore(&g_topo_lock, state);
    return false;
}

bool usb_topology_detach_child(uint32_t parent_hub_id, uint32_t child_id) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_topo_lock);
    for (uint32_t i = 0; i < g_node_count; i++) {
        if (g_nodes[i].node_id == parent_hub_id) {
            for (uint32_t j = 0; j < g_nodes[i].child_count; j++) {
                if (g_nodes[i].children[j] && g_nodes[i].children[j]->node_id == child_id) {
                    // Shift left
                    for (uint32_t k = j; k < g_nodes[i].child_count - 1; k++) {
                        g_nodes[i].children[k] = g_nodes[i].children[k + 1];
                    }
                    g_nodes[i].children[g_nodes[i].child_count - 1] = NULL;
                    g_nodes[i].child_count--;
                    atoms_spin_unlock_irqrestore(&g_topo_lock, state);
                    return true;
                }
            }
        }
    }
    atoms_spin_unlock_irqrestore(&g_topo_lock, state);
    return false;
}

usb_topology_node_t* usb_topology_find_node(uint32_t node_id) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_topo_lock);
    for (uint32_t i = 0; i < g_node_count; i++) {
        if (g_nodes[i].node_id == node_id) {
            atoms_spin_unlock_irqrestore(&g_topo_lock, state);
            return &g_nodes[i];
        }
    }
    atoms_spin_unlock_irqrestore(&g_topo_lock, state);
    return NULL;
}
