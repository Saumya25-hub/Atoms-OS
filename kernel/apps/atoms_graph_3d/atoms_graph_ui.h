#ifndef ATOMS_GRAPH_UI_H
#define ATOMS_GRAPH_UI_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/wm/bwe/include/bwe.h"
#include "atoms_graph_metrics.h"

void atoms_graph_ui_render_panel(const BVFramebuffer* fb, BWE_Rect win_bounds,
                                  const AtomsGraphMetrics* m, bool is_finished, uint32_t score);

void atoms_graph_ui_render_results_screen(const BVFramebuffer* fb, BWE_Rect win_bounds,
                                           const AtomsGraphMetrics* m, uint32_t score);

#endif // ATOMS_GRAPH_UI_H
