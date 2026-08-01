#include "../include/bar_api.h"

static BARMessage g_msg_queue[BAR_MAX_MESSAGES];
static uint32_t   g_msg_head = 0;
static uint32_t   g_msg_tail = 0;
static uint32_t   g_msg_count = 0;

int32_t BAR_PostMessage(BARWindowID win_id, BARMessageType type, uint32_t p1, uint32_t p2) {
    if (g_msg_count >= BAR_MAX_MESSAGES) return -1;
    
    g_msg_queue[g_msg_tail].window_id = win_id;
    g_msg_queue[g_msg_tail].type = type;
    g_msg_queue[g_msg_tail].param1 = p1;
    g_msg_queue[g_msg_tail].param2 = p2;
    g_msg_queue[g_msg_tail].timestamp = 1000;
    
    g_msg_tail = (g_msg_tail + 1) % BAR_MAX_MESSAGES;
    g_msg_count++;
    return 0;
}

int32_t BAR_SendMessage(BARWindowID win_id, BARMessageType type, uint32_t p1, uint32_t p2) {
    BARMessage msg = {win_id, type, p1, p2, 1000};
    return BAR_DispatchMessage(&msg);
}

bool BAR_PeekMessage(BARWindowID win_id, BARMessage* out_msg) {
    (void)win_id;
    if (g_msg_count == 0 || !out_msg) return false;
    
    *out_msg = g_msg_queue[g_msg_head];
    g_msg_head = (g_msg_head + 1) % BAR_MAX_MESSAGES;
    g_msg_count--;
    return true;
}
