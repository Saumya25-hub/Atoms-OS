#ifndef BOVISUAL_INPUT_H
#define BOVISUAL_INPUT_H

#include "events.h"
#include "controls.h"

// BOVISUAL Input Processor
// Takes raw BVEvents and routes them to controls.

void BV_Input_ProcessEvent(const BVEvent* event, BOVISUAL_Control_Button* buttons, int button_count);

#endif // BOVISUAL_INPUT_H
