#ifndef ATOMS_GRAPH_3D_H
#define ATOMS_GRAPH_3D_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/wm/bwe/include/bwe.h"

int  atoms_graph_3d_launch(uint32_t* out_win_id);
void atoms_graph_3d_close(void);
void atoms_graph_3d_pump_frame(float delta_ms);
bool atoms_graph_3d_is_active(void);

#endif // ATOMS_GRAPH_3D_H
