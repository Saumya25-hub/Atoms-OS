#include "../include/ole32_api.h"

int OLE32GetCurrentApartmentType(void) {
    return COINIT_APARTMENTTHREADED;
}
