/*
 * BOSPECTRA V3 — Ownership Tree Dump Implementation
 * kernel/media/bospectra/diagnostics/ownership_dump.c
 */

#include "ownership_dump.h"
#include "../resource/resource_graph.h"
#include "../debug/bospectra_debug.h"

extern void display_print(const char* str);

void bospectra_dump_ownership_tree(void) {
    display_print("\n============= RESOURCE OWNERSHIP TREE =============\n");
    display_print("Playback Session #1\n");
    display_print("├── File Handle (VFS)\n");
    display_print("├── Container Context (AVI/MP4/MKV)\n");
    display_print("├── Packet Queue (Lock-Free Ring Buffer)\n");
    display_print("├── Decoder Context (H264 / MJPEG)\n");
    display_print("├── Color Engine (BT.601 ARGB32 Converter)\n");
    display_print("├── Frame Queue (FramePool Recycling)\n");
    display_print("├── Renderer Session (Software / OpenGL)\n");
    display_print("└── Display Scheduler (BWE Presentation Surface)\n");
    display_print("===================================================\n\n");
    bospectra_resource_graph_dump();
}
