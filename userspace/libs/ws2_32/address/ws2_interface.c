#include "../include/ws2_32_api.h"

int gethostname(char* name, int namelen) {
    if (name && namelen >= 7) {
        name[0] = 'A'; name[1] = 'T'; name[2] = 'O'; name[3] = 'M'; name[4] = 'S'; name[5] = '1'; name[6] = '\0';
    }
    return 0;
}
