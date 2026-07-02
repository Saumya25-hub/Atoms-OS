#ifndef BOS_TEXT_VIEWER_H
#define BOS_TEXT_VIEWER_H

#include <stdint.h>
#include "kernel/wm/surface/surface.h"

// Open a text file in a new Text Viewer window
void text_viewer_open(const char* filepath);

#endif // BOS_TEXT_VIEWER_H
