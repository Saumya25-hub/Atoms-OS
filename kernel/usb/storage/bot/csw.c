#include "../include/usb_csw.h"

bool usb_csw_validate(const usb_csw_t* csw, uint32_t expected_tag) {
    if (!csw) return false;
    if (csw->dCSWSignature != USB_CSW_SIGNATURE) return false;
    if (csw->dCSWTag != expected_tag) return false;
    return (csw->bCSWStatus == CSW_STATUS_PASSED || csw->bCSWStatus == CSW_STATUS_FAILED || csw->bCSWStatus == CSW_STATUS_PHASE_ERROR);
}
