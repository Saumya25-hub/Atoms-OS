#include "dispatcher_priority.h"

bool dispatcher_priority_is_valid(DispatcherPriorityTier tier) {
    return (int)tier >= 0 && (int)tier < DISPATCH_TIER_COUNT;
}

const char* dispatcher_priority_get_name(DispatcherPriorityTier tier) {
    switch (tier) {
        case DISPATCH_TIER_0_POINTER_ENGINE: return "Tier 0 (Pointer Engine)";
        case DISPATCH_TIER_1_CURSOR_ENGINE:  return "Tier 1 (Cursor Engine)";
        case DISPATCH_TIER_2_DESKTOP:        return "Tier 2 (Desktop / Login)";
        case DISPATCH_TIER_3_WINDOW_MANAGER: return "Tier 3 (Window Manager)";
        case DISPATCH_TIER_4_WIDGETS:        return "Tier 4 (Widgets / Controls)";
        case DISPATCH_TIER_5_APPLICATIONS:   return "Tier 5 (Applications)";
        default:                             return "Unknown Tier";
    }
}
