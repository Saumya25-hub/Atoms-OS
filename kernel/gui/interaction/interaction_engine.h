#ifndef GUI_INTERACTION_ENGINE_H
#define GUI_INTERACTION_ENGINE_H

#include "kernel/drivers/input/input_abstraction.h"
#include "kernel/gui/window/window.h"
#include <stdint.h>

// Initializes the interaction engine
void interaction_engine_init(void);

// Called per frame to process input and route events to windows
void interaction_engine_update(InputState state);

// Reset capture states (e.g., if drag is aborted)
void interaction_engine_release_capture(void);

#endif // GUI_INTERACTION_ENGINE_H
