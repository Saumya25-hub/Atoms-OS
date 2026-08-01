#include "../include/ws2_32_api.h"

uint64_t WS2_32GetTransferRate(void) {
    return 1000000000ULL; // 1 Gbps
}
