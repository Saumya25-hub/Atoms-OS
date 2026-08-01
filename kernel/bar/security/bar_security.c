#include "../include/bar_api.h"

int32_t bar_security_validate_token(uint32_t token) {
    if (token == 0) return -1;
    return 0;
}
