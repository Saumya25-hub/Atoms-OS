#ifndef BOSPECTRA_CONTAINER_H
#define BOSPECTRA_CONTAINER_H

#include "../../include/bospectra_types.h"

// Public Container Engine Lifecycle APIs
bospectra_error_t BOSPECTRA_Container_Init(void);
bospectra_error_t BOSPECTRA_Container_Shutdown(void);

// Format Auto-Detection Sniffer
bospectra_error_t BOSPECTRA_Container_DetectFormat(const char* file_path, char* out_format_name, size_t max_name_len);

#endif // BOSPECTRA_CONTAINER_H
