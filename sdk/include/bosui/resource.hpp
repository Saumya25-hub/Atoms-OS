#ifndef BOSUI_RESOURCE_HPP
#define BOSUI_RESOURCE_HPP

#include "sdk/include/bos/bos_res.h"

namespace bosui {

class ResourceManager {
public:
    static BOS_Result Load(const char* res_id, BOS_ResourceType type, BOS_Resource** out_res) {
        return BOS_Resource_Load(res_id, type, out_res);
    }
    static void Free(BOS_Resource* res) {
        BOS_Resource_Free(res);
    }
};

} // namespace bosui

#endif /* BOSUI_RESOURCE_HPP */
