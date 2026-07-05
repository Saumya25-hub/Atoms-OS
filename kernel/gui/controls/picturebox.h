#ifndef GUI_CONTROLS_PICTUREBOX_H
#define GUI_CONTROLS_PICTUREBOX_H

#include "kernel/gui/controls/control.h"

typedef struct {
    BOSControl base;
    uint32_t* bitmap_data;
    int bmp_width;
    int bmp_height;
    bool stretch;
} BOSPictureBox;

BOSPictureBox* picturebox_create();
void picturebox_set_image(BOSPictureBox* pb, uint32_t* data, int width, int height);

#endif // GUI_CONTROLS_PICTUREBOX_H
