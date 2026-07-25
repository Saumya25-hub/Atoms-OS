#include "html_debug.h"
#include "kernel/drivers/display/display.h"

void html_debug_log(const char* level, const char* message) {
    if (!level || !message) return;
    display_print("[HTML_DEBUG ");
    display_print(level);
    display_print("] ");
    display_print(message);
    display_print("\n");
}

void html_debug_dump_node(const bos_node_t* node, int depth) {
    if (!node) return;
    for (int i = 0; i < depth; i++) {
        display_print("  ");
    }
    display_print("<");
    display_print(node->name);
    if (node->value[0] != '\0') {
        display_print(" val=\"");
        display_print(node->value);
        display_print("\"");
    }
    display_print(">\n");

    bos_node_t* child = node->first_child;
    while (child) {
        html_debug_dump_node(child, depth + 1);
        child = child->next_sibling;
    }
}
